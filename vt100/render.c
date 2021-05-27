#include <SDL.h>
#include "vt100.h"
#include "xsdl.h"

/* Inputs:
   fb[]
   a[]
   brightness
   columns;
   reverse_field;
   underline;
   wide
   scroll */

extern SDL_Renderer *renderer;

static u32 scanline[1024][10];

static void data (u32 *raster, int width, unsigned pixels, struct draw *data)
{
  int size = width * sizeof (u32);
  u32 lit;
  unsigned mask;
  int i;
  u32 x = data->brightness;
  x = 255 * (31 - (x & 0x1F)) / 31;
  lit = x << 16 | x << 8 | x;
  memset (raster, 0, size);
  for (i = 0, mask = (1 << (width - 1)); i < width; i++, mask >>= 1) {
    if (pixels & mask)
      raster[i] = lit;
   }
  if (data->columns == 132) {
    SDL_Texture *tex1 = SDL_CreateTexture (renderer,
                                           SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_STATIC,
                                           9, 1);
    SDL_Texture *tex2 = SDL_CreateTexture (renderer,
                                           SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_TARGET,
                                           6, 1);
    SDL_Rect r1, r2;
    r1.x = r1.y = 0;
    r1.w = 9;
    r1.h = 1;
    SDL_UpdateTexture (tex1, &r1, raster, 9 * sizeof (u32));
    SDL_SetRenderTarget (renderer, tex2);
    r2.x = r1.y = 0;
    r2.w = 6;
    r2.h = 1;
    SDL_RenderCopy (renderer, tex1, &r1, &r2);
    memset (raster, 0, 10 * sizeof (u32));
    SDL_RenderReadPixels (renderer, &r2, SDL_PIXELFORMAT_RGBA8888,
                          raster, 6 * sizeof (u32));
    SDL_SetRenderTarget (renderer, NULL);
    SDL_DestroyTexture (tex1);
    SDL_DestroyTexture (tex2);
  }
}

void render (struct draw *x)
{
  int width = x->columns == 80 ? 10 : 9;
  unsigned i;
  for (i = 0; i < (1 << width); i++)
    data (scanline[i], width, i, x);
}

static unsigned widen (unsigned pixels, unsigned mask1, unsigned mask2)
{
  int result = 0;
  while (mask1) {
    if (pixels & mask1)
      result |= mask2;
    mask1 >>= 1;
    mask2 >>= 2;
  }
  return result;
}

static u8 * render_80 (u8 *dest, int c, int wide, int scroll, struct draw *data)
{
  unsigned pixels, reverse = 0;
  pixels = vt100font[16 * (c & 0x7F) + scroll];
  if (c & 0x80) {
    if (data->underline)
      pixels = scroll == 8 ? 0xFF : pixels;
    else
      reverse = wide ? 0xFFFFF : 0x3FF;
  }
  if (data->reverse)
    reverse ^= wide ? 0xFFFFF : 0x3FF;
  pixels <<= 2;
  if (pixels & 4)
    pixels |= 3;
  pixels |= pixels >> 1;
  if (wide)
    pixels = widen (pixels, 0x200, 0xC0000);
  pixels ^= reverse;
  if (wide) {
    SDL_memcpy (dest, scanline[pixels >> 10], 10 * sizeof (u32));
    dest += 10 * sizeof (u32);
  }
  SDL_memcpy (dest, scanline[pixels & 0x3FF], 10 * sizeof (u32));
  dest += 10 * sizeof (u32);
  return dest;
}

static u8 * render_132 (u8 *dest, int c, int wide, int scroll, struct draw *data)
{
  unsigned pixels, reverse = 0;
  pixels = vt100font[16 * (c & 0x7F) + scroll];
  if (c & 0x80) {
    if (data->underline)
      pixels = scroll == 8 ? 0xFF : pixels;
    else
      reverse = wide ? 0x3FFFF : 0x1FF;
  }
  if (data->reverse)
    reverse ^= wide ? 0x3FFFF : 0x1FF;
  pixels <<= 1;
  if (pixels & 2)
    pixels |= 1;
  pixels |= pixels >> 1;
  if (wide)
    pixels = widen (pixels, 0x200, 0xC0000);
  pixels ^= reverse;
  if (wide) {
    SDL_memcpy (dest, scanline[pixels >> 9], 6 * sizeof (u32));
    dest += 6 * sizeof (u32);
  }
  SDL_memcpy (dest, scanline[pixels & 0x1FF], 6 * sizeof (u32));
  dest += 6 * sizeof (u32);
  return dest;
}

u8 * render_video (u8 *dest, int c, int wide, int scroll, void *data)
{
  if (((struct draw *)data)->columns == 80)
    return render_80 (dest, c, wide, scroll, data);
  else
    return render_132 (dest, c, wide, scroll, data);
}

void reset_render (void *data)
{
  struct draw x;
  if (data == NULL) {
    x.brightness = 0x10;
    x.columns = 80;
    data = &x;
  }
  render (data);
}
