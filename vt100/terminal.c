#define _XOPEN_SOURCE 600

#include <SDL.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include "terminal.h"

/* Common code for all terminals */

void
panic(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(1);
}



#define MATSIZ (2*BLURRADIUS+1)
float blurmat[MATSIZ][MATSIZ];

Col
getblur(Col *src, int width, int height, int x, int y)
{
	int xx, yy;
	Col *p;
	int i, j;
	int r, g, b, a;
	Col c;

	r = g = b = a = 0;
	for(i = 0, yy = y-BLURRADIUS; yy <= y+BLURRADIUS; yy++, i++){
		if(yy < 0 || yy >= height)
			continue;
		for(j = 0, xx = x-BLURRADIUS; xx <= x+BLURRADIUS; xx++, j++){
			if(xx < 0 || xx >= width)
				continue;
			p = &src[yy*width + xx];
			r += p->r * blurmat[i][j];
			g += p->g * blurmat[i][j];
			b += p->b * blurmat[i][j];
			a += p->a * blurmat[i][j];
		}
	}
	c.r = pow(r/255.0f, Gamma)*255;
	c.g = pow(g/255.0f, Gamma)*255;
	c.b = pow(b/255.0f, Gamma)*255;
	c.a = pow(a/255.0f, Gamma)*255;

	p = &src[y*width + x];
	if(p->a > c.a){
		// full brightness
		c = phos1;
	}else{
		// glow
		c.r = phos2.r*c.a/255.0f;
		c.g = phos2.g*c.a/255.0f;
		c.b = phos2.b*c.a/255.0f;
		c.a = 255;
	}

	return c;
}

void
initblur(float sig)
{
	int i, j;
	float dx, dy;

	for(i = 0; i < MATSIZ; i++)
		for(j = 0; j < MATSIZ; j++){
			dx = i-BLURRADIUS;
			dy = j-BLURRADIUS;
			blurmat[i][j] = exp(-(dx*dx + dy*dy)/(2*sig*sig)) / (2*M_PI*sig*sig);
		}
}



char **scancodemap;

/* Map SDL scancodes to ASCII */
char *scancodemap_both[SDL_NUM_SCANCODES] = {
	[SDL_SCANCODE_ESCAPE] = "\033\033",

	[SDL_SCANCODE_GRAVE] = "`~",
	[SDL_SCANCODE_1] = "1!",
	[SDL_SCANCODE_2] = "2@",
	[SDL_SCANCODE_3] = "3#",
	[SDL_SCANCODE_4] = "4$",
	[SDL_SCANCODE_5] = "5%",
	[SDL_SCANCODE_6] = "6^",
	[SDL_SCANCODE_7] = "7&",
	[SDL_SCANCODE_8] = "8*",
	[SDL_SCANCODE_9] = "9(",
	[SDL_SCANCODE_0] = "0)",
	[SDL_SCANCODE_MINUS] = "-_",
	[SDL_SCANCODE_EQUALS] = "=+",
	[SDL_SCANCODE_BACKSPACE] = "\b\b",
	[SDL_SCANCODE_DELETE] = "\177\177",

	[SDL_SCANCODE_TAB] = "\011\011",
	[SDL_SCANCODE_Q] = "qQ",
	[SDL_SCANCODE_W] = "wW",
	[SDL_SCANCODE_E] = "eE",
	[SDL_SCANCODE_R] = "rR",
	[SDL_SCANCODE_T] = "tT",
	[SDL_SCANCODE_Y] = "yY",
	[SDL_SCANCODE_U] = "uU",
	[SDL_SCANCODE_I] = "iI",
	[SDL_SCANCODE_O] = "oO",
	[SDL_SCANCODE_P] = "pP",
	[SDL_SCANCODE_LEFTBRACKET] = "[{",
	[SDL_SCANCODE_RIGHTBRACKET] = "]}",
	[SDL_SCANCODE_BACKSLASH] = "\\|",

	[SDL_SCANCODE_A] = "aA",
	[SDL_SCANCODE_S] = "sS",
	[SDL_SCANCODE_D] = "dD",
	[SDL_SCANCODE_F] = "fF",
	[SDL_SCANCODE_G] = "gG",
	[SDL_SCANCODE_H] = "hH",
	[SDL_SCANCODE_J] = "jJ",
	[SDL_SCANCODE_K] = "kK",
	[SDL_SCANCODE_L] = "lL",
	[SDL_SCANCODE_SEMICOLON] = ";:",
	[SDL_SCANCODE_APOSTROPHE] = "'\"",
	[SDL_SCANCODE_RETURN] = "\015\015",

	[SDL_SCANCODE_Z] = "zZ",
	[SDL_SCANCODE_X] = "xX",
	[SDL_SCANCODE_C] = "cC",
	[SDL_SCANCODE_V] = "vV",
	[SDL_SCANCODE_B] = "bB",
	[SDL_SCANCODE_N] = "nN",
	[SDL_SCANCODE_M] = "mM",
	[SDL_SCANCODE_COMMA] = ",<",
	[SDL_SCANCODE_PERIOD] = ".>",
	[SDL_SCANCODE_SLASH] = "/?",
	[SDL_SCANCODE_SPACE] = "  ",
};

char *scancodemap_upper[SDL_NUM_SCANCODES] = {
	[SDL_SCANCODE_ESCAPE] = "\033\033",

	[SDL_SCANCODE_GRAVE] = "\033\033",
	[SDL_SCANCODE_1] = "1!",
	[SDL_SCANCODE_2] = "2@",
	[SDL_SCANCODE_3] = "3#",
	[SDL_SCANCODE_4] = "4$",
	[SDL_SCANCODE_5] = "5%",
	[SDL_SCANCODE_6] = "6^",
	[SDL_SCANCODE_7] = "7&",
	[SDL_SCANCODE_8] = "8*",
	[SDL_SCANCODE_9] = "9(",
	[SDL_SCANCODE_0] = "0)",
	[SDL_SCANCODE_MINUS] = "-_",
	[SDL_SCANCODE_EQUALS] = "=+",
	[SDL_SCANCODE_BACKSPACE] = "\b\b",
	[SDL_SCANCODE_DELETE] = "\177\177",

	[SDL_SCANCODE_TAB] = "\011\011",
	[SDL_SCANCODE_Q] = "QQ",
	[SDL_SCANCODE_W] = "WW",
	[SDL_SCANCODE_E] = "EE",
	[SDL_SCANCODE_R] = "RR",
	[SDL_SCANCODE_T] = "TT",
	[SDL_SCANCODE_Y] = "YY",
	[SDL_SCANCODE_U] = "UU",
	[SDL_SCANCODE_I] = "II",
	[SDL_SCANCODE_O] = "OO",
	[SDL_SCANCODE_P] = "PP",
	[SDL_SCANCODE_LEFTBRACKET] = "[[",
	[SDL_SCANCODE_RIGHTBRACKET] = "]]",
	[SDL_SCANCODE_BACKSLASH] = "\\\\",

	[SDL_SCANCODE_A] = "AA",
	[SDL_SCANCODE_S] = "SS",
	[SDL_SCANCODE_D] = "DD",
	[SDL_SCANCODE_F] = "FF",
	[SDL_SCANCODE_G] = "GG",
	[SDL_SCANCODE_H] = "HH",
	[SDL_SCANCODE_J] = "JJ",
	[SDL_SCANCODE_K] = "KK",
	[SDL_SCANCODE_L] = "LL",
	[SDL_SCANCODE_SEMICOLON] = ";:",
	[SDL_SCANCODE_APOSTROPHE] = "'\"",
	[SDL_SCANCODE_RETURN] = "\015\015",

	[SDL_SCANCODE_Z] = "ZZ",
	[SDL_SCANCODE_X] = "XX",
	[SDL_SCANCODE_C] = "CC",
	[SDL_SCANCODE_V] = "VV",
	[SDL_SCANCODE_B] = "BB",
	[SDL_SCANCODE_N] = "NN",
	[SDL_SCANCODE_M] = "MM",
	[SDL_SCANCODE_COMMA] = ",<",
	[SDL_SCANCODE_PERIOD] = ".>",
	[SDL_SCANCODE_SLASH] = "/?",
	[SDL_SCANCODE_SPACE] = "  ",
};

int ctrl;
int shift;
int alt;

void
toggle_fullscreen(void)
{
	u32 f = SDL_GetWindowFlags(window) &
		SDL_WINDOW_FULLSCREEN_DESKTOP;
	SDL_SetWindowFullscreen(window,
		f ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void
sendchar(char c)
{
	write(pty, &c, 1);
}

void
keydown(SDL_Keysym keysym, int repeat)
{
	char *keys;
	int key;

	switch(keysym.scancode){
	case SDL_SCANCODE_LSHIFT:
	case SDL_SCANCODE_RSHIFT: shift = 1; return;
	case SDL_SCANCODE_CAPSLOCK:
	case SDL_SCANCODE_LCTRL:
	case SDL_SCANCODE_RCTRL: ctrl = 1; return;
	default: break;
	}
	if(keystate[SDL_SCANCODE_LGUI] || keystate[SDL_SCANCODE_RGUI])
		return;
	if(keysym.scancode == SDL_SCANCODE_LALT || keysym.scancode == SDL_SCANCODE_RALT) {
		alt = 1;
		return;
	}

	if(keysym.scancode == SDL_SCANCODE_F11 && !repeat)
		toggle_fullscreen();

	keys = scancodemap[keysym.scancode];
	/* TODO: implement keypad */
	if(keys == nil)
		return;
	key = keys[shift];
	if(ctrl)
		key &= 037;
//	printf("%o(%d %d) %c\n", key, shift, ctrl, key);

	if(alt){
		sendchar('\033');
		sendchar(key);
	}else{
		sendchar(key);
	}

/*	// local echo for testing
	recvchar(c);
	updatebuf = 1;
	draw();
*/
}

void
keyup(SDL_Keysym keysym)
{
	switch(keysym.scancode){
	case SDL_SCANCODE_LSHIFT:
	case SDL_SCANCODE_RSHIFT: shift = 0; return;
	case SDL_SCANCODE_CAPSLOCK:
	case SDL_SCANCODE_LCTRL:
	case SDL_SCANCODE_RCTRL: ctrl = 0; return;
	case SDL_SCANCODE_LALT:
	case SDL_SCANCODE_RALT: alt = 0; return;
	default: break;
	}
}

int baud;

void spawn(void)
{
	switch(fork()){
	case -1:
		panic("fork failed");

	case 0:
		close(pty);
		close(0);
		close(1);
		close(2);

		setsid();

		if(open(name, O_RDWR) != 0)
			exit(1);
		dup(0);
		dup(1);

		shell();
	}
}

void mkpty(struct winsize *ws, int th, int tw, int fw, int fh)
{
	pty = posix_openpt(O_RDWR);
	if(pty < 0 ||
	   grantpt(pty) < 0 ||
	   unlockpt(pty) < 0)
		panic("Couldn't get pty");

	name = ptsname(pty);

	ws->ws_row = th;
	ws->ws_col = tw;
	ws->ws_xpixel = fw;
	ws->ws_ypixel = fh;
	ioctl(pty, TIOCSWINSZ, ws);
}

void mkwindow(SDL_Window **window, SDL_Renderer **renderer,
	      char *title, int width, int height)
{
	SDL_Init(SDL_INIT_EVERYTHING);
	if(SDL_CreateWindowAndRenderer(width, height, 0, window, renderer) < 0)
		;
		//panic("SDL_CreateWindowAndRenderer() failed: %s\n", SDL_GetError());
	SDL_SetWindowTitle(*window, title);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_SetHint("SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS", "0");
}
