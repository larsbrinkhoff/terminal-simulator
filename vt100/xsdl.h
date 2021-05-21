#include <SDL.h>

struct draw {
  int scroll;
  int columns;
  int reverse;
  int underline;
  int brightness;
  void *pixels;
  int pitch;
};

extern void sdl_init (int scale, int full);
extern void sdl_loop (void);
extern void sdl_refresh (struct draw *data);
extern void sdl_render (int brightness, int columns);
extern void draw_line (int scroll, int attr, int y, u8 *data);
extern SDL_SpinLock lock_update;
