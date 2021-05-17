#ifndef TERMINAL_H
#define TERMINAL_H

#include <sys/ioctl.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
#define nil NULL

typedef struct Col Col;
struct Col
{
	u8 a, b, g, r;
};

extern Col phos1, phos2;
extern float Gamma;
extern int altesc;

void panic(char *fmt, ...);

#define BLURRADIUS 4
Col getblur(Col *src, int width, int height, int x, int y);
void initblur(float sig);

void keyup(SDL_Keysym keysym);
void keydown(SDL_Keysym keysym, int repeat);
void draw(void);
void set_dtr(SDL_bool dtr);
void recvchar(int c);
void sendchar(char c);
void shell(void);
void spawn(void);
void mkpty(struct winsize *ws, int th, int tw, int fw, int fh);
void mkwindow(SDL_Window **window, SDL_Renderer **renderer,
	      char *title, int width, int height);
void toggle_fullscreen(void);

extern int baud;
extern int rerun;
extern char **scancodemap;
extern char *scancodemap_both[];
extern char *scancodemap_upper[];
extern const u8 *keystate;
extern u32 userevent;
extern int updatebuf;
extern int updatescreen;
extern int pty;
extern SDL_Window *window;
extern char *name;

#endif /* TERMINAL_H */
