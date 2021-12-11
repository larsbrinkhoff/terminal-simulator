#include "xsdl.h"
#include <SDL_image.h>
#include "opengl.h"
#include "term.h"
#include "log.h"

#define USEREVENT_DRAW     (userevent + 0)
#define USEREVENT_RENDER   (userevent + 1)
#define USEREVENT_SOUND    (userevent + 2)
#define USEREVENT_PRESENT  (userevent + 3)

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *screentex;
static uint32_t userevent;
static uint8_t fb[TERMHEIGHT + 1][137];
static uint8_t a[TERMHEIGHT + 1];
uint8_t capslock;
static void (*meta) (void);

static SDL_Window *sound_window = NULL;
static SDL_Renderer *sound_renderer;
static uint8_t sound_buffer[800];
static int sound_pointer = 0;

void sdl_sound (uint8_t *data, int size)
{
  SDL_Event ev;
  SDL_zero (ev);
  ev.type = USEREVENT_SOUND;
  ev.user.data1 = malloc (size);
  ev.user.data2 = malloc (sizeof size);
  memcpy (ev.user.data1, data, size);
  *(int *)ev.user.data2 = size;
  SDL_PushEvent (&ev);
}

static void draw_sound (uint8_t *data, int *size)
{
  int i;

  if (sound_window == NULL) {
    SDL_CreateWindowAndRenderer (sizeof sound_buffer, 256, 0,
				 &sound_window, &sound_renderer);
    SDL_SetWindowTitle (sound_window, "VT100 Sound");
  }

  SDL_SetRenderDrawColor (sound_renderer, 0, 0, 0, 255);
  SDL_RenderClear (sound_renderer);
  SDL_SetRenderDrawColor (sound_renderer, 0, 255, 0, 255);
  for (i = 0; i < *size; i++) {
    sound_buffer[sound_pointer] = data[i];
    if (sound_buffer[sound_pointer] > 3)
      sound_buffer[sound_pointer] -= rand() & 3;
    sound_pointer++;
    sound_pointer %= sizeof sound_buffer;
  }
  for (i = 0; i < sizeof sound_buffer; i++)
    SDL_RenderDrawPoint (sound_renderer, i, 255 - sound_buffer[i]);
  SDL_SetRenderDrawColor (sound_renderer, 100, 100, 100, 255);
  for (i = 0; i < sizeof sound_buffer; i += 48)
    SDL_RenderDrawLine (sound_renderer, i, 0, i, 255);
  for (i = 0; i < 256; i += 48)
    SDL_RenderDrawLine (sound_renderer, 0, i, sizeof sound_buffer - 1, i);
  SDL_RenderPresent (sound_renderer);
  free (data);
  free (size);
}

void draw_line (int scroll, int attr, int y, uint8_t *data)
{
  int i;
  if ((attr | scroll) != a[y] || memcmp (data, fb[y], 80) != 0) {
    a[y] = attr | scroll;
    for (i = 0; i < 132; i++)
      fb[y][i] = data[i];
  }
}

void sdl_refresh (struct draw *data)
{
  SDL_Event ev;
  SDL_zero (ev);
  ev.type = USEREVENT_DRAW;
  ev.user.data1 = malloc (sizeof (struct draw));
  memcpy (ev.user.data1, data, sizeof (struct draw));
  SDL_PushEvent (&ev);
}

void sdl_render (int brightness, int columns)
{
  struct draw *data;
  SDL_Event ev;
  data = malloc (sizeof (struct draw));
  data->columns = columns;
  data->brightness = brightness;
  data->renderer = renderer;
  SDL_zero (ev);
  ev.type = USEREVENT_RENDER;
  ev.user.data1 = data;
  SDL_PushEvent (&ev);
}

void sdl_present (struct draw *data)
{
  struct draw *copy;
  SDL_Event ev;
  SDL_zero (ev);
  copy = malloc (sizeof (struct draw));
  memcpy (copy, data, sizeof (struct draw));
  ev.type = USEREVENT_PRESENT;
  ev.user.data1 = copy;
  SDL_PushEvent (&ev);
}

static void draw_present (struct draw *data)
{
  void *pixels;
  uint8_t *rgb, *source;
  int pitch;
  int i;

  if (SDL_LockTexture (screentex, NULL, &pixels, &pitch) != 0) {
    LOG (SDL, "LockTexture error: %s", SDL_GetError ());
    exit (1);
  }

  for (i = 0; i < 240; i++) {
    rgb = pixels;
    rgb += pitch * BORDER;
    rgb += pitch * i * 2;
    rgb += sizeof (uint32_t) * BORDER;
    source = data->pixels;
    source += sizeof (uint32_t) * 900 * (i + 17);
    source += sizeof (uint32_t) * 135;
    memcpy (rgb, source, 720 * sizeof (uint32_t));
  }

  SDL_UnlockTexture (screentex);
  free (data);

  SDL_RenderCopy (renderer, screentex, NULL, NULL);
  SDL_RenderPresent (renderer);
}

static void draw (struct draw *data)
{
  int scroll;
  int x, y, yy;
  void *pixels;
  uint8_t *dest;
  int pitch;
  int w, h;

  if (SDL_LockTexture (screentex, NULL, &pixels, &pitch) != 0) {
    LOG (SDL, "LockTexture error: %s", SDL_GetError ());
    exit (1);
  }

  dest = pixels + BORDER * pitch;
  dest += BORDER * sizeof (uint32_t);
  scroll = data->scroll;
  if (data->odd) {
    memset (dest, 0, pitch);
    dest += pitch;
  }
  for (y = yy = 0; y < 240; y++) {
    w = ((a[yy] & 0x60) != 0x60) ? 2 : 1;
    for (x = 0; x < data->columns / w; x++)
      dest = render_video (dest, fb[yy][x], (a[yy] & 0x60) != 0x60, scroll, data);
    dest += pitch - sizeof (uint32_t) * 800;
    memset (dest, 0, pitch);
    dest += pitch;
    if (data->columns == 132)
      dest += (800 - 132 * 6) * sizeof (uint32_t);
    if ((a[yy] & 0x40) == 0x40 || (y & 1)) {
      scroll++;
      if (scroll == ((a[yy] & 0x60) == 0x20 ? 5 : 10)) {
	yy++;
	if ((a[yy] & 0x60) == 0)
	  scroll = 5;
	else
	  scroll = 0;
      }
    }
  }
  if (!data->odd)
    memset (dest, 0, pitch);
  SDL_UnlockTexture (screentex);
  if (quick) {
    SDL_RenderCopy (renderer, screentex, NULL, NULL);
    SDL_RenderPresent (renderer);
  } else {
    SDL_GetRendererOutputSize (renderer, &w, &h);
    opengl_present (screentex, w, h);
    SDL_GL_SwapWindow (window);
  }
  free (data);
}

static void toggle_fullscreen (void)
{
  Uint32 flags = SDL_GetWindowFlags (window);
  flags ^= SDL_WINDOW_FULLSCREEN_DESKTOP;
  SDL_SetWindowFullscreen (window, flags);
  if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
    SDL_ShowCursor (SDL_DISABLE);
  else
    SDL_ShowCursor (SDL_ENABLE);
}

void sdl_capslock (uint8_t code)
{
  capslock = code;
}

static void altmode (void)
{
  key_down (0x2A);
  SDL_Delay (50);
  key_up (0x2A);
}

static void nothing (void)
{
}

void sdl_init (const char *title, int scale, int full)
{
  SDL_RendererInfo info;
  SDL_DisplayMode mode;
  struct draw *data;
  Uint32 flags;
  int w, h;

  flags = SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO;
  if (!quick)
    flags |= SDL_VIDEO_OPENGL;
  SDL_Init (flags);

  LOG (SDL, "Video displays: %d", SDL_GetNumVideoDisplays ());
  SDL_GetCurrentDisplayMode (0, &mode);
  LOG (SDL, "Refresh rate: %d Hz", mode.refresh_rate);

  window = SDL_CreateWindow (title,
			     SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			     WIDTH*scale, HEIGHT*scale, 0);
  if (window == NULL)
    //panic("SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError ());
    ;
  if (!quick)
    SDL_SetHint (SDL_HINT_RENDER_DRIVER, "opengl");
  renderer = SDL_CreateRenderer (window, -1,
				 SDL_RENDERER_ACCELERATED |
				 SDL_RENDERER_TARGETTEXTURE |
				 SDL_RENDERER_PRESENTVSYNC);
  SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  SDL_SetHint ("SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS", "0");

  SDL_GetRendererInfo (renderer, &info);
  SDL_GetRendererOutputSize (renderer, &w, &h);
  LOG (SDL, "Renderer: %s, %dx%d", info.name, w, h);

  SDL_Surface *icon = IMG_Load ("icon.jpg");
  if (icon != NULL) {
    SDL_SetWindowIcon (window, icon);
    SDL_FreeSurface (icon);
  }

  if (full)
    toggle_fullscreen ();
  screentex = SDL_CreateTexture (renderer, SDL_PIXELFORMAT_RGB888,
                                 SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
  userevent = SDL_RegisterEvents (3);
  memset (a, 0, sizeof a);

  meta = nothing;

  data = malloc (sizeof (struct draw));
  data->brightness = 0x10;
  data->columns = 80;
  data->renderer = renderer;
  reset_render (data);
}

static int special_key (SDL_KeyboardEvent *ev)
{
  switch (ev->keysym.scancode) {
  case SDL_SCANCODE_F11:
    if (ev->state != SDL_PRESSED)
      return 1;
    if (SDL_GetModState () & KMOD_CTRL)
      exit (0);
    else
      toggle_fullscreen ();
    return 1;
  case SDL_SCANCODE_LALT:
  case SDL_SCANCODE_RALT:
  case SDL_SCANCODE_LGUI:
  case SDL_SCANCODE_RGUI:
    switch (ev->state) {
    case SDL_PRESSED: meta = altmode; break;
    case SDL_RELEASED: meta = nothing; break;
    }
    return 1;
  default:
    return 0;
  }  
}

void sdl_loop (void)
{
  SDL_Event ev;
  uint8_t code;

  while (SDL_WaitEvent(&ev) >= 0) {
    switch (ev.type) {
    case SDL_QUIT:
      exit (0);

    case SDL_KEYDOWN:
      if (ev.key.repeat)
        break;
      if (special_key (&ev.key))
        break;
      code = keymap (ev.key.keysym.scancode);
      if (code == 0)
        break;
      meta ();
      key_down (code);
      break;
    case SDL_KEYUP:
      if (special_key (&ev.key))
        break;
      code = keymap (ev.key.keysym.scancode);
      if (code == 0)
        break;
      key_up (code);
      break;

    default:
      if (ev.type == USEREVENT_DRAW)
	draw (ev.user.data1);
      else if (ev.type == USEREVENT_RENDER)
	reset_render (ev.user.data1);
      else if (ev.type == USEREVENT_SOUND)
	draw_sound (ev.user.data1, ev.user.data2);
      else if (ev.type == USEREVENT_PRESENT)
	draw_present (ev.user.data1);
      break;
    }
  }
}
