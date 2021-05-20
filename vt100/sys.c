#include "vt100.h"

u8 memory[0x10000];
u16 starta = 0;
void (*halt) (u16);
u8 vt100_flags;
static int imask;

u8 (*device_in[256]) (u8 port);
void (*device_out[256]) (u8 port, u8 data);

void raise_interrupt (int mask)
{
  imask |= mask;
  if (imask)
    interrupt (RST0 | imask << 3);
  else
    interrupt (-1);
}

void clear_interrupt (int mask)
{
  imask &= ~mask;
  if (imask)
    interrupt (RST0 | imask << 3);
  else
    interrupt (-1);
}

void register_port (u8 port, u8 (*in) (u8), void (*out) (u8, u8))
{
  device_in[port] = in;
  device_out[port] = out;
}

static void no_out (u8 port, u8 data)
{
  LOG (SYS, "Write to unknown port %02X", port);
}

static u8 no_in (u8 port)
{
  LOG (SYS, "Read from unknown port %02X", port);
  return 0;
}

void reset (void)
{
  int i;

  log_file = stderr;

  cpu_reset ();

  for (i = 0; i < 256; i++) {
    device_out[i] = no_out;
    device_in[i] = no_in;
  }

  for (i = 0x2000; i < 0x2C00; i++)
    memory[i] = rand ();

  starta = 0;
  imask = 0;

  memset (mtype, 2, 64);
  mtype[0] = 1; //8K RROM
  mtype[1] = 1;
  mtype[2] = 1;
  mtype[3] = 1;
  mtype[4] = 1;
  mtype[5] = 1;
  mtype[6] = 1;
  mtype[7] = 1;
  mtype[8] = 0; //3K RAM
  mtype[9] = 0;
  mtype[10] = 0;

  reset_brightness ();
  reset_pusart ();
  reset_nvr ();
  //SDL_CreateThread (timer, "Time", NULL);
  reset_video ();
  reset_keyboard ();
  reset_sound ();
  reset_render ();
}
