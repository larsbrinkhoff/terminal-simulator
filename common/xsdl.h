#include <SDL.h>

struct draw {
  int odd;
  int scroll;
  int line;
  int columns;
  int reverse;
  int underline;
  int brightness;
  void *pixels;
  int pitch;
  SDL_Renderer *renderer;
};

extern void sdl_init (const char *title, int scale, int full);
extern void sdl_loop (void);
extern void sdl_refresh (struct draw *data);
extern void sdl_render (int brightness, int columns);
extern void draw_line (int scroll, int attr, int y, uint8_t *data);
extern void sdl_capslock (uint8_t code);
extern void sdl_sound (uint8_t *data, int size);
extern void sdl_present (struct draw *data);

extern uint8_t keymap (SDL_Scancode key);
extern uint8_t capslock;
extern void key_up (uint8_t code);
extern void key_down (uint8_t code);
extern void reset_render (void *);
extern uint8_t *render_video (uint8_t *dest, int c, int wide, int scroll, void *);
extern int quick;
