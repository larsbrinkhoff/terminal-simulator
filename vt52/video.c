#include "defs.h"
#include "xsdl.h"
#include "vcd.h"

static int x, y;
static int vsr;
static int old_scan, old_vert;
static struct draw data;
static uint32_t rgb[900];

static void refresh (void)
{
  sdl_present ();
}

void video_shifter (int data)
{
  vsr = data | 0x80;
}

void video (void)
{
  long long t;

  if (old_scan && !scan) {
    if (y >= 0 && y <= 239) {
      data.pixels = &rgb[130];
      data.line = y;
      sdl_scanline (&data);
    }
    x = 0;
    y++;
  }
  old_scan = scan;

  if (!old_vert && vert) {
    refresh ();
    y = 0;
  }
  old_vert = vert;

  if (vsr & 0200) {
    rgb[x] = 0x000000;
  } else {
    rgb[x] = 0xFFFFFF;
  }

  vsr <<= 1;
  vsr |= 1;
  x++;

  t = 1e9 * cycles / 13.824e6;
  vcd_real (t, index_beam_x, x);
  vcd_real (t, index_beam_y, y);
  vcd_value (t, index_vsr, vsr);
}

void reset_video (void)
{
  memset (rgb, 0, sizeof rgb);
  refresh ();
}
