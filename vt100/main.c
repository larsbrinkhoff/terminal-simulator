#include <SDL.h>
#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <pwd.h>
#include <poll.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

#include "terminal.h"
#include "vt100.h"

Col phos1 = { 0xFF, 0xFF, 0xFF, 0xF0 };
Col phos2 = { 0xFF, 0xC0, 0xA0, 0x70 };
float Gamma = 1.0/2.2f;

#define TERMWIDTH 80
#define TERMHEIGHT 24

#define CWIDTH 10
#define CHEIGHT 10

#define FBWIDTH (TERMWIDTH*CWIDTH+2)
#define FBHEIGHT (TERMHEIGHT*CHEIGHT+2)

#define WIDTH  (FBWIDTH)
#define HEIGHT (2*FBHEIGHT)

SDL_SpinLock lock_update;
SDL_Surface *screen;
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *screentex;
const u8 *keystate;
u8 fb[TERMHEIGHT + 1][137];
u8 a[TERMHEIGHT + 1];
int curx, cury;
u32 userevent;
int updatebuf = 1;
int updaterender = 1;
int blink;
int arrows = 0;
int rerun = 0;
int scale = 1;
int full = 0;

SDL_Texture *fonttex[256];

int pty;

#define TEXW ((CWIDTH + 2*BLURRADIUS))
#define TEXH ((CHEIGHT*2 + 2*BLURRADIUS*2))

void
createchar(u32 *raster, int c)
{
	int i, j;
	u8 bg = 0x00, fg = 0xFF;
	u8 *chr;

	if (c < 128)
		chr = &vt100font[c*16];
	else {
		chr = &vt100font[(c-128)*16];
		bg = 0xFF;
		fg = 0x00;
	}

	memset(raster, 0, TEXW*TEXH*sizeof(u32));
	raster = &raster[BLURRADIUS*TEXW + BLURRADIUS];

	for(i = 0; i < CHEIGHT; i++){
		for(j = 0; j <= 8; j++){
			if(chr[i]&(0600>>j))
				raster[(i*2+0)*TEXW + j] = fg;
			else
				raster[(i*2+0)*TEXW + j] = bg;
		}
		for(; j < CWIDTH; j++){
			raster[(i*2+0)*TEXW + j] = raster[(i*2+0)*TEXW + 8];
		}
	}
}

void
blurchar(u32 *dst, u32 *src)
{
	Col *s, *d, c;
	int x, y;

	s = (Col*)src;
	d = (Col*)dst;
	for(y = 0; y < TEXH; y++){
		for(x = 0; x < TEXW; x++){
			c = getblur(s, TEXW, TEXH, x, y);
			c.a = 255;
			d[y*TEXW + x] = c;
		}
	}
}

void
createfont(void)
{
	int i;
	int w, h;
	u32 *ras1, *ras2;
	w = TEXW;
	h = TEXH;
	ras1 = malloc(w*h*sizeof(u32));
	ras2 = malloc(w*h*sizeof(u32));
	for(i = 0; i < 256; i++){
		createchar(ras1, i);
		blurchar(ras2, ras1);


		fonttex[i] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
			SDL_TEXTUREACCESS_STREAMING, w, h);
		SDL_SetTextureBlendMode(fonttex[i], SDL_BLENDMODE_ADD);
		SDL_UpdateTexture(fonttex[i], nil, ras2, w*sizeof(u32));
	}
}

void
draw_line (int scroll, int attr, int y, u8 *data)
{
  int i;
  if ((attr | scroll) != a[y] || memcmp (data, fb[y], 80) != 0) {
    SDL_AtomicLock(&lock_update);
    a[y] = attr | scroll;
    for (i = 0; i < 132; i++)
      fb[y][i] = data[i];
    updatebuf = 1;
    SDL_AtomicUnlock(&lock_update);
  }
}

void
checkupdate (void)
{
  SDL_Event ev;
  int flag;
  SDL_AtomicLock(&lock_update);
  flag = updatebuf;
  SDL_AtomicUnlock(&lock_update);
  if (!flag)
    return;
  memset(&ev, 0, sizeof(ev));
  ev.type = userevent;
  SDL_PushEvent(&ev);
}

void
draw(void)
{
  extern int line_scroll;
  int scroll;
  int x, xx, y, yy;

  SDL_AtomicLock(&lock_update);
  if (updaterender) {
    updaterender = 0;
    reset_render ();
  }

  if(updatebuf) {
    updatebuf = 0;

    SDL_SetRenderTarget(renderer, screentex);
    SDL_RenderClear(renderer);
    scroll = line_scroll;
    for(y = yy = 0; y < 240; y++) {
      for(x = xx = 0; xx < 792; x++)
	xx = render_video (xx, y, fb[yy][x], (a[yy] & 0x60) != 0x60, scroll);
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
    SDL_SetRenderTarget(renderer, nil);
    SDL_RenderCopy(renderer, screentex, nil, nil);
    SDL_RenderPresent(renderer);
  }
  SDL_AtomicUnlock(&lock_update);
}

void
recvchar(int c)
{
	pusart_rx (c);
}

char **cmd;

void
shell(void)
{
	setenv("TERM", "vt100", 1);
	execvp(cmd[0], cmd);
	exit(1);
}


static void bdos_print (u8 *s)
{
  while (*s != '$')
    putchar (*s++);
}

void bdos_out (u8 port, u8 data)
{
  u16 reg_SP;
  u8 reg[10];
  cpu_state (&reg_SP, reg);
  switch (reg[3]) {
  case 0x01:
    break;
  case 0x02:
  case 0x05:
    putchar (reg[5]);
    fflush (stdout);
    break;
  case 0x09:
    bdos_print (&memory[reg[4] << 8 | reg[5]]);
    fflush (stdout);
    break;
  default:
    fprintf (stderr, "Unknown BDOS: C=%d\n", reg[3]);
    exit (1);
  }
}

Uint32 ticks;
unsigned long long previous;

static void throttle (void)
{
	ticks = SDL_GetTicks () - ticks;
	previous = get_cycles () - previous;
	ticks *= 2765;
	if (previous > 1000000)
		LOG (CPU, "Large number of cycles: %llu", previous);
	else if (previous > ticks)
		SDL_Delay ((previous - ticks) / 2765);
	ticks = SDL_GetTicks ();
	previous = get_cycles ();
}

void
run(char *file)
{
	FILE *f = fopen (file, "rb");
	if (f == NULL)
		panic("Couldn't open %s: %s", file, strerror (errno));
	if (fread (memory + 0x100, 1, 0x10000 - 0x100, f) <= 0)
		panic("Error reading %s");

	memset (mtype, 0, 64);
	memory[0] = 0x76;  //HLT
	memory[5] = 0xD3;  //OUT FF
	memory[6] = 0xFF;
	memory[7] = 0xC9;  //RET

	//stderr = fopen ("/dev/null", "w");

	cpu_reset ();
	register_port (0xFF, NULL, bdos_out);
	starta = 0x100;
	jump (starta);
	for (;;)
		execute ();
}

static int cputhread (void *arg)
{
  unsigned long long cycles, previous = get_cycles ();
  memcpy (memory, vt100rom, 0x2000);
  reset ();
  for (;;) {
    execute ();
    cycles = get_cycles ();
    events (cycles - previous);
    previous = cycles;
  }
  return 0;
}


char *argv0;
char *name;

void
usage(void)
{
	panic("usage: %s [-2] [-B] [-f] [-b baudrate] command...", argv0);
}

static void end (u16 addr)
{
#if 0
  fprintf (stderr, "\nExecuted %llu instructions, %llu cycles.\n",
	   instructions, get_cycles ());
#endif
  exit (0);
}

static int special_key (SDL_Scancode key)
{
  switch (key) {
  case SDL_SCANCODE_F11: toggle_fullscreen (); return 1;
  default: return 0;
  }  
}

static u8 keymap (SDL_Scancode key) {
  switch (key) {
  case SDL_SCANCODE_DELETE: return 0x03; // DELETE
  case SDL_SCANCODE_P: return 0x05;
  case SDL_SCANCODE_O: return 0x06;
  case SDL_SCANCODE_Y: return 0x07;
  case SDL_SCANCODE_T: return 0x08;
  case SDL_SCANCODE_W: return 0x09;
  case SDL_SCANCODE_Q: return 0x0A;
  case SDL_SCANCODE_RIGHT: return 0x10;
  case SDL_SCANCODE_RIGHTBRACKET: return 0x14;
  case SDL_SCANCODE_LEFTBRACKET: return 0x15;
  case SDL_SCANCODE_I: return 0x16;
  case SDL_SCANCODE_U: return 0x17;
  case SDL_SCANCODE_R: return 0x18;
  case SDL_SCANCODE_E: return 0x19;
  case SDL_SCANCODE_1: return 0x1A;
  case SDL_SCANCODE_LEFT: return 0x20;
  case SDL_SCANCODE_DOWN: return 0x22;
  case SDL_SCANCODE_PAUSE: return 0x23; // BREAK
  case SDL_SCANCODE_GRAVE: return 0x24;
  case SDL_SCANCODE_MINUS: return 0x25;
  case SDL_SCANCODE_9: return 0x26;
  case SDL_SCANCODE_7: return 0x27;
  case SDL_SCANCODE_4: return 0x28;
  case SDL_SCANCODE_3: return 0x29;
  case SDL_SCANCODE_ESCAPE: return 0x2A;
  case SDL_SCANCODE_UP: return 0x30;
  case SDL_SCANCODE_F3: return 0x31; // PF3
  case SDL_SCANCODE_F1: return 0x32; // PF1
  case SDL_SCANCODE_BACKSPACE: return 0x33; // BACKSPACE
  case SDL_SCANCODE_EQUALS: return 0x34;
  case SDL_SCANCODE_0: return 0x35;
  case SDL_SCANCODE_8: return 0x36;
  case SDL_SCANCODE_6: return 0x37;
  case SDL_SCANCODE_5: return 0x38;
  case SDL_SCANCODE_2: return 0x39;
  case SDL_SCANCODE_TAB: return 0x3A;
  case SDL_SCANCODE_KP_7: return 0x40;
  case SDL_SCANCODE_F4: return 0x41; // PF4
  case SDL_SCANCODE_F2: return 0x42; // PF2
  case SDL_SCANCODE_KP_0: return 0x43;
  case SDL_SCANCODE_F7: return 0x44; // LINE-FEED
  case SDL_SCANCODE_BACKSLASH: return 0x45;
  case SDL_SCANCODE_L: return 0x46;
  case SDL_SCANCODE_K: return 0x47;
  case SDL_SCANCODE_G: return 0x48;
  case SDL_SCANCODE_F: return 0x49;
  case SDL_SCANCODE_A: return 0x4A;
  case SDL_SCANCODE_KP_8: return 0x50;
  case SDL_SCANCODE_KP_ENTER: return 0x51;
  case SDL_SCANCODE_KP_2: return 0x52;
  case SDL_SCANCODE_KP_1: return 0x53;
  case SDL_SCANCODE_APOSTROPHE: return 0x55;
  case SDL_SCANCODE_SEMICOLON: return 0x56;
  case SDL_SCANCODE_J: return 0x57;
  case SDL_SCANCODE_H: return 0x58;
  case SDL_SCANCODE_D: return 0x59;
  case SDL_SCANCODE_S: return 0x5A;
  case SDL_SCANCODE_KP_PERIOD: return 0x60; // KEYPAD PERIOD
  case SDL_SCANCODE_KP_COMMA: return 0x61; // KEYPAD COMMA
  case SDL_SCANCODE_KP_5: return 0x62;
  case SDL_SCANCODE_KP_4: return 0x63;
  case SDL_SCANCODE_RETURN: return 0x64;
  case SDL_SCANCODE_PERIOD: return 0x65;
  case SDL_SCANCODE_COMMA: return 0x66;
  case SDL_SCANCODE_N: return 0x67;
  case SDL_SCANCODE_B: return 0x68;
  case SDL_SCANCODE_X: return 0x69;
  case SDL_SCANCODE_SCROLLLOCK: return 0x6A; // NO-SCROLL
  case SDL_SCANCODE_KP_9: return 0x70;
  case SDL_SCANCODE_KP_3: return 0x71;
  case SDL_SCANCODE_KP_6: return 0x72;
  case SDL_SCANCODE_KP_MINUS: return 0x73; // KEYPAD MINUS
  case SDL_SCANCODE_SLASH: return 0x75;
  case SDL_SCANCODE_M: return 0x76;
  case SDL_SCANCODE_SPACE: return 0x77;
  case SDL_SCANCODE_V: return 0x78;
  case SDL_SCANCODE_C: return 0x79;
  case SDL_SCANCODE_Z: return 0x7A;
  case SDL_SCANCODE_F9: return 0x7B; // SETUP
  case SDL_SCANCODE_RCTRL:
  case SDL_SCANCODE_LCTRL: return 0x7C;
  case SDL_SCANCODE_RSHIFT:
  case SDL_SCANCODE_LSHIFT: return 0x7D;
  case SDL_SCANCODE_CAPSLOCK: return 0x7E;
  default: return 0;
  }
}

int
main(int argc, char *argv[])
{
	SDL_Event ev;
	int x, y;
	struct winsize ws;
	int debug = 0;
	int opt;
	u8 code;

	scancodemap = scancodemap_both;
	halt = end;

	baud = 19200;
	argv0 = argv[0];
	while ((opt = getopt (argc, argv, "b:Br2fR:D")) != -1) {
	  switch (opt) {
	case 'b':
	  baud = atoi(optarg);
	  break;
	case 'B':
	  /* Backspace is Rubout. */
	  scancodemap[SDL_SCANCODE_BACKSPACE] = "\177\177";
	  break;
	case 'r':
	  rerun = 1;
	  break;
	  case '2':
	    scale++;
	    break;
	  case 'f':
	    full = 1;
	    break;
	  case 'R':
	    run(optarg);
	    break;
	  case 'D':
	    debug = 1;
	    break;
	  default:
	    usage();
	    break;
	  }
	}

	if(optind == argc && !debug)
		usage();

	cmd = &argv[optind];

	mkpty(&ws, TERMHEIGHT, TERMWIDTH, FBWIDTH, FBHEIGHT);
	spawn();

	mkwindow(&window, &renderer, "VT100", WIDTH*scale, HEIGHT*scale);

	screentex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
			SDL_TEXTUREACCESS_TARGET, WIDTH, HEIGHT);

	keystate = SDL_GetKeyboardState(nil);

	userevent = SDL_RegisterEvents(1);

	for(x = 0; x < TERMWIDTH; x++)
		for(y = 0; y < TERMHEIGHT; y++)
			fb[y][x] = ' ';
	memset (a, 0, sizeof a);
	initblur(1.5);
	createfont();

	SDL_AtomicUnlock(&lock_update);

	if (debug)
		ddt();
	else
		SDL_CreateThread (cputhread, "CPU",  NULL);

	if(full)
		toggle_fullscreen();

	while(SDL_WaitEvent(&ev) >= 0){
		switch(ev.type){
		case SDL_QUIT:
			goto out;

		case SDL_KEYDOWN:
			if (ev.key.repeat)
			  break;
			if (special_key (ev.key.keysym.scancode))
			  break;
			code = keymap (ev.key.keysym.scancode);
			if (code == 0)
			  break;
			key_down (code);
			break;
		case SDL_KEYUP:
			key_up (keymap (ev.key.keysym.scancode));
			break;

		case SDL_USEREVENT:
			/* got a new character */
			draw();
			break;
		case SDL_WINDOWEVENT:
			switch(ev.window.event){
			case SDL_WINDOWEVENT_MOVED:
			case SDL_WINDOWEVENT_ENTER:
			case SDL_WINDOWEVENT_LEAVE:
			case SDL_WINDOWEVENT_FOCUS_GAINED:
			case SDL_WINDOWEVENT_FOCUS_LOST:
#if (SDL_MAJOR_VERSION > 2) || (SDL_MAJOR_VERSION == 2 && \
    (SDL_MINOR_VERSION > 0) || (SDL_PATCHLEVEL > 4))
			case SDL_WINDOWEVENT_TAKE_FOCUS:
#endif
				break;
			default:
				break;
			}
			break;
		}
	}
out:
	SDL_Quit();
	return 0;
}
