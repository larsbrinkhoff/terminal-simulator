#include <SDL.h>

struct draw {
  int odd;
  int scroll;
  int columns;
  int reverse;
  int underline;
  int brightness;
  void *pixels;
  int pitch;
  SDL_Renderer *renderer;
};

extern void sdl_init (int scale, int full);
extern void sdl_loop (void);
extern void sdl_refresh (struct draw *data);
extern void sdl_render (int brightness, int columns);
extern void draw_line (int scroll, int attr, int y, u8 *data);
extern void sdl_capslock (u8 code);
extern void sdl_sound (u8 *data, int size);
