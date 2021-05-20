#include "vt100.h"

extern SDL_Renderer *renderer;

SDL_Texture *scanline[1024];

extern u8 brightness;

static void data (u32 *raster, int width, unsigned pixels)
{
  int size = width * sizeof (u32);
  u32 lit;
  unsigned mask;
  int i;
  u32 x = brightness;
  x = 255 * (31 - (x & 0x1F)) / 31;
  lit = x << 24 | x << 16 | x << 8 | 0xFF;
  memset (raster, 0, size);
  for (i = 0, mask = (1 << (width - 1)); i < width; i++, mask >>= 1) {
    if (pixels & mask)
      raster[i] = lit;
  }
}

void render (int width, SDL_Renderer *renderer)
{
  int size = width * sizeof (u32);
  unsigned i;
  u32 *raster;
  for (i = 0; i < (1 << width); i++) {
    raster = malloc (size);
    data (raster, width, i);
    if (scanline[i] == NULL)
      scanline[i] = SDL_CreateTexture (renderer, SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_STATIC, width, 1);
    SDL_SetTextureBlendMode (scanline[i], SDL_BLENDMODE_ADD);
    SDL_UpdateTexture (scanline[i], NULL, raster, size);
    free (raster);
  }
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

extern int columns;
extern int reverse_field;
extern int underline;

int render_80 (int x, int y, int c, int wide, int scroll)
{
  SDL_Rect r;
  unsigned pixels, reverse = 0;
  r.x = x;
  r.y = 2 * y;
  r.w = 10;
  r.h = 1;
  pixels = vt100font[16 * (c & 0x7F) + scroll];
  if (c & 0x80) {
    if (underline)
      pixels = scroll == 8 ? 0xFF : pixels;
    else
      reverse = wide ? 0xFFFFF : 0x3FF;
  }
  if (reverse_field)
    reverse ^= wide ? 0xFFFFF : 0x3FF;
  pixels <<= 2;
  if (pixels & 4)
    pixels |= 3;
  pixels |= pixels >> 1;
  if (wide)
    pixels = widen (pixels, 0x200, 0xC0000);
  pixels ^= reverse;
  if (wide) {
    SDL_RenderCopy (renderer, scanline[pixels >> 10], NULL, &r);
    r.x += 10;
  }
  SDL_RenderCopy (renderer, scanline[pixels & 0x3FF], NULL, &r);
  r.x += 10;
  return r.x;
}

static SDL_Rect r132 = { 1, 0, 9, 1 };

int render_132 (int x, int y, int c, int wide, int scroll)
{
  SDL_Rect r;
  unsigned pixels, reverse = 0;
  r.x = x;
  r.y = 2 * y;
  r.w = 6;
  r.h = 1;
  pixels = vt100font[16 * (c & 0x7F) + scroll];
  if (c & 0x80) {
    if (underline)
      pixels = scroll == 8 ? 0xFF : pixels;
    else
      reverse = wide ? 0x3FFFF : 0x1FF;
  }
  if (reverse_field)
    reverse ^= wide ? 0x3FFFF : 0x1FF;
  pixels <<= 1;
  if (pixels & 2)
    pixels |= 1;
  pixels |= pixels >> 1;
  if (wide)
    pixels = widen (pixels, 0x100, 0x30000);
  pixels ^= reverse;
  if (wide) {
    SDL_RenderCopy (renderer, scanline[pixels >> 9], &r132, &r);
    r.x += 6;
  }
  SDL_RenderCopy (renderer, scanline[pixels & 0x1FF], &r132, &r);
  r.x += 6;
  return r.x;
}

int render_video (int x, int y, int c, int wide, int scroll)
{
  if (columns == 80)
    return render_80 (x, y, c, wide, scroll);
  else
    return render_132 (x, y, c, wide, scroll);
}

void reset_render (void)
{
  render (10, renderer);
}
