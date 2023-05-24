#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "xsdl.h"
#include "opengl.h"
#include "term.h"
#include "log.h"
#include "vcd.h"

float curvature = 0.1;
unsigned long long cycles;
int quick = 0;
int pixcolor = 0;

extern void reset_pty (char **cmd, int th, int tw, int fw, int fh);
static char *command[] = { "bash", NULL };

int index_e2_74164;
int index_e27_74161;
int index_e18_7490;
int index_e13_7493;
int index_e11_7493;
int index_e5_74161;
int index_e6_74161;
int index_e7_74161;
int index_horiz;
int index_scan;
int index_fly;
int index_video;
int index_vert;
int index_tos;
int index_vsr;
int index_vsr_ld;
int index_auto_inc;
int index_beam_x;
int index_beam_y;
int index_reg_a;
int index_reg_b;
int index_reg_x;
int index_reg_y;
int index_tosj;
int index_pc;
int index_rom;
int index_mem;
int index_rx;
int index_tx;

unsigned long long get_cycles (void)
{
  return cycles;
}

Uint32 ticks;
unsigned long long previous;

static void throttle (void)
{
  ticks = SDL_GetTicks () - ticks;
  previous = cycles - previous;
  ticks *= 13824;
  if (previous > ticks)
    SDL_Delay ((previous - ticks) / 13824);
  ticks = SDL_GetTicks ();
  previous = cycles;
}

static int cputhread (void *arg)
{
  (void)arg;

  for (cycles = 0; ; cycles++) {
    if ((cycles % 18) == 0)
      LOG (MAIN, "%7lld/%9.3fms:", cycles, 1e3 * cycles / 13.824e6);
    timing ();
    step ();
    video ();
    if ((cycles % 1000000) == 0)
      throttle ();
  }

  return 0;
}

int main (void)
{
  FILE *f;

  f = fopen ("rom/microcode", "rb");
  fread (ROM, 1, 1024, f);
  fclose (f);

  {
    extern unsigned char M[];
    int i;
    for (i = 0; i < 2048; i++)
      M[i] = rand ();
  }

#ifdef DEBUG_VCD
  index_e2_74164 = vcd_variable ("e2_74164", "reg", 8);
  index_e27_74161 = vcd_variable ("e27_74161", "reg", 4);
  index_e18_7490 = vcd_variable ("e18_7490", "reg", 4);
  index_e13_7493 = vcd_variable ("e13_7493", "reg", 4);
  index_e11_7493 = vcd_variable ("e11_7493", "reg", 4);
  index_e5_74161 = vcd_variable ("e5_74161", "reg", 4);
  index_e6_74161 = vcd_variable ("e6_74161", "reg", 4);
  index_e7_74161 = vcd_variable ("e7_74161", "reg", 4);
  index_horiz = vcd_variable ("horiz", "wire", 1);
  index_scan = vcd_variable ("scan", "wire", 1);
  index_fly = vcd_variable ("fly", "wire", 1);
  index_video = vcd_variable ("video", "wire", 1);
  index_vert = vcd_variable ("vert", "wire", 1);
  index_tos = vcd_variable ("tos", "wire", 1);
  index_vsr = vcd_variable ("vsr", "reg", 8);
  index_vsr_ld = vcd_variable ("vsr_ld", "wire", 1);
  index_auto_inc = vcd_variable ("auto_inc", "wire", 1);
  index_beam_x = vcd_variable ("beam_x", "real", 64);
  index_beam_y = vcd_variable ("beam_y", "real", 64);
  index_reg_a = vcd_variable ("a", "reg", 7);
  index_reg_b = vcd_variable ("b", "reg", 7);
  index_reg_x = vcd_variable ("x", "reg", 7);
  index_reg_y = vcd_variable ("y", "reg", 5);
  index_pc = vcd_variable ("pc", "reg", 10);
  index_rom = vcd_variable ("rom", "wire", 8);
  index_mem = vcd_variable ("mem", "reg", 7);
  index_rx = vcd_variable ("rx", "wire", 1);
  index_tx = vcd_variable ("tx", "wire", 1);
  vcd_start ("dump.vcd", "VT52 simulator", "1ns");
#endif

  sdl_init ("VT52", 1, 0);
  init_opengl ();

  setenv ("TERM", "vt52", 1);
  reset_keyboard ();
  reset_pty (command, TERMHEIGHT, TERMWIDTH, FBWIDTH, FBHEIGHT);
  reset_uart ();

  SDL_CreateThread (cputhread, "VT52: CPU",  NULL);

  sdl_loop ();

  return 0;
}
