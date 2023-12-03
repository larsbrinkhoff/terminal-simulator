#include <stdio.h>
#include <stdint.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define RST0 0xC7
#define RST1 0xCF
#define RST2 0xD7
#define RST3 0xDF
#define RST4 0xE7
#define RST5 0xEF
#define RST6 0xF7
#define RST7 0xFF
#define foreach(item, array) \
    for(int keep = 1, \
            count = 0,\
            size = sizeof (array) / sizeof *(array); \
        keep && count != size; \
        keep = !keep, count++) \
      for (item = (array) + count; keep; keep = !keep)

extern u8 memory[0x10000];
extern u16 starta;
extern unsigned long long get_cycles (void);
extern u8 vt100_flags;
extern int pty;
extern int sound_scope;
extern int field_rate;

extern int quick;
extern int pixcolor;

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

extern void nvr_clock (void);
extern void key_down (u8 code);
extern void key_up (u8 code);
