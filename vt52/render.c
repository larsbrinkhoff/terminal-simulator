#include <stdlib.h>
#include "defs.h"

uint8_t * render_video (uint8_t *dest, int c, int wide, int scroll, void *data)
{
  (void)dest;
  (void)c;
  (void)wide;
  (void)scroll;
  (void)data;
  return NULL;
}

void reset_render (void *data)
{
  free (data);
}
