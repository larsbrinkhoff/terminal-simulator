#include <stdio.h>
#include <stdint.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define TERMWIDTH 80
#define TERMHEIGHT 24

#define CWIDTH 10
#define CHEIGHT 10

#define FBWIDTH (TERMWIDTH*CWIDTH)
#define FBHEIGHT (TERMHEIGHT*CHEIGHT)

#define BORDER 5
#define WIDTH  (FBWIDTH + 2*BORDER)
#define HEIGHT (2*FBHEIGHT + 2*BORDER)

#define RST0 0xC7
#define RST1 0xCF
#define RST2 0xD7
#define RST3 0xDF
#define RST4 0xE7
#define RST5 0xEF
#define RST6 0xF7
#define RST7 0xFF

struct event {
  unsigned cycles;
  char *name;
  void (*callback) (void);
  struct event *next;
};

#define EVENT(NAME, CALLBACK) \
  struct event NAME = { 0, #NAME, (CALLBACK), NULL }

extern u8 memory[0x10000];
extern u16 starta;
extern unsigned long long get_cycles (void);
extern u8 vt100_flags;
extern int pty;
extern int sound_scope;
extern int quick;
extern int field_rate;

extern u8 vt100rom[];
extern u8 vt100font[];

extern void cpu_state (u16 *sp, u8 *regs);
extern void cpu_reset (void);
extern void jump (u16 addr);
extern u16 execute (void);
extern void ddt (void);
extern void (*halt) (u16 addr);
extern u8 mtype[];

extern u8 flags_in (u8 port);
extern void bell (void);
extern void reset_keyboard (void);
extern void reset (void);
extern void (*device_out[256]) (u8 port, u8 data);
extern u8 (*device_in[256]) (u8 port);
extern void register_port (u8 port,
                             u8 (*in) (u8),
                             void (*out) (u8, u8));
extern void interrupt (int rst);
extern void raise_interrupt (int mask);
extern void clear_interrupt (int mask);

extern void reset_pusart (void);
extern void pusart_rx (u8 data);

extern void reset_brightness (void);
extern void reset_nvr (void);
extern void reset_video (void);
extern int event (void *arg);
extern int timer (void *arg);
extern void reset_sound (void);
extern void reset_render (void *);
extern u8 *render_video (u8 *dest, int c, int wide, int scroll, void *);

extern void nvr_clock (void);
extern void key_down (u8 code);
extern void key_up (u8 code);

#ifdef DEBUG
#define LOG(COMPONENT, MESSAGE, ...) \
  logger (#COMPONENT, MESSAGE, ##__VA_ARGS__)
#else
#define LOG(COMPONENT, MESSAGE, ...) do {} while (0)
#endif
extern FILE *log_file;
extern void logger (const char *device, const char *format, ...);
extern void events (unsigned cycles);
extern void add_event (unsigned cycles, struct event *event);
extern void print_events (FILE *);
extern void mkpty (char **cmd, int th, int tw, int fw, int fh);
extern void send_break (void);
extern void send_character (u8 data);
extern u8 receive_character (void);
extern void reset_pty (char **cmd, int th, int tw, int fw, int fh);
