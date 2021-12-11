#include "defs.h"
#include "xsdl.h"
#include "vcd.h"

static int x, y;
static int vsr;
static int old_scan, old_vert;
static struct draw data;
static uint32_t rgb[900 * 400];

static void refresh (void)
{
  data.pixels = rgb;
  sdl_present (&data);
}

void video_shifter (int data)
{
  vsr = data | 0x80;
}

void video (void)
{
#ifdef DEBUG_VCD
  long long t;
#endif

  if (old_scan && !scan) {
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
    rgb[900 * y + x] = 0x000000;
  } else {
    rgb[900 * y + x] = 0xFFFFFF;
  }

  vsr <<= 1;
  vsr |= 1;
  x++;

#ifdef DEBUG_VCD
  t = 1e9 * cycles / 13.824e6;
  vcd_real (t, index_beam_x, x);
  vcd_real (t, index_beam_y, y);
  vcd_value (t, index_vsr, vsr);
#endif
}

void reset_video (void)
{
  memset (rgb, 0, sizeof rgb);
  refresh ();
}
