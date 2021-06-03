#include "vt100.h"
#include "xsdl.h"
#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

int full = 0;
char *argv0;
char **cmd;

void panic (char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  fprintf (stderr, "\n");
  va_end (ap);
  exit (1);
}

static void bdos_print (u8 *s)
{
  while (*s != '$')
    putchar (*s++);
}

void bdos_out (u8 port, u8 data)
{
  u16 reg_SP;
  u8 reg[10];
  cpu_state (&reg_SP, reg);
  switch (reg[3]) {
  case 0x01:
    break;
  case 0x02:
  case 0x05:
    putchar (reg[5]);
    fflush (stdout);
    break;
  case 0x09:
    bdos_print (&memory[reg[4] << 8 | reg[5]]);
    fflush (stdout);
    break;
  default:
    fprintf (stderr, "Unknown BDOS: C=%d\n", reg[3]);
    exit (1);
  }
}

Uint32 ticks;
unsigned long long previous;

static void throttle (void)
{
  ticks = SDL_GetTicks () - ticks;
  previous = get_cycles () - previous;
  ticks *= 2765;
  if (previous > 1000000)
    LOG (CPU, "Large number of cycles: %llu", previous);
  else if (previous > ticks)
    SDL_Delay ((previous - ticks) / 2765);
  ticks = SDL_GetTicks ();
  previous = get_cycles ();
}

static void run (char *file)
{
  FILE *f = fopen (file, "rb");
  if (f == NULL)
    panic("Couldn't open %s: %s", file, strerror (errno));
  if (fread (memory + 0x100, 1, 0x10000 - 0x100, f) <= 0)
    panic("Error reading %s");

  memset (mtype, 0, 64);
  memory[0] = 0x76;  //HLT
  memory[5] = 0xD3;  //OUT FF
  memory[6] = 0xFF;
  memory[7] = 0xC9;  //RET

  //stderr = fopen ("/dev/null", "w");

  cpu_reset ();
  register_port (0xFF, NULL, bdos_out);
  starta = 0x100;
  jump (starta);
  for (;;)
    execute ();
}

static int cputhread (void *arg)
{
  unsigned long long cycles, previous = get_cycles ();
  memcpy (memory, vt100rom, 0x2000);
  for (;;) {
    execute ();
    cycles = get_cycles ();
    events (cycles - previous);
    previous = cycles;
  }
  return 0;
}

void
usage(void)
{
  panic ("usage: %s [-2] [-B] [-f] command...", argv0);
}

static void end (u16 addr)
{
  exit (0);
}

int main (int argc, char **argv)
{
  int debug = 0;
  int scale = 1;
  int opt;

  halt = end;
  sdl_capslock (0x7E); //Default is capslock.

  argv0 = argv[0];
  while ((opt = getopt (argc, argv, "b:Br2fR:DC")) != -1) {
    switch (opt) {
    case 'B':
      /* Backspace is Rubout. */
      break;
    case '2':
      scale++;
      break;
    case 'f':
      full = 1;
      break;
    case 'R':
      run(optarg);
      break;
    case 'D':
      debug = 1;
      break;
    case 'C':
      sdl_capslock (0x7C); //Make capslock into control.
      break;
    default:
      usage();
      break;
    }
  }

  if (optind == argc && !debug)
    usage ();

  cmd = &argv[optind];

  log_file = stderr;

  reset_pty (cmd, TERMHEIGHT, TERMWIDTH, FBWIDTH, FBHEIGHT);
  sdl_init (scale, full);
  reset ();

  if (debug)
    ddt ();
  else
    SDL_CreateThread (cputhread, "vt100: CPU",  NULL);

  sdl_loop ();
  return 0;
}
