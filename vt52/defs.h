#include <stdint.h>

#ifdef DEBUG
#define DEBUG_VCD 1
#endif

extern void step (void);
extern unsigned char ROM[1024];
extern unsigned char CHAR[1024];
extern void timing (void);
extern void video_shifter (int data);
extern void video (void);
extern void reset_video (void);

extern void uart_clock (void);
extern void reset_uart (void);
extern int uart_rx_flag (void);
extern uint8_t uart_rx_data (void);
extern int uart_tx_flag (void);
extern void uart_tx_data (uint8_t data);

extern void reset_keyboard (void);
extern int key_flag (uint8_t code);

extern int scan, fly, horiz, vert, tos;
extern unsigned long long cycles;

extern int index_e2_74164;
extern int index_e27_74161;
extern int index_e18_7490;
extern int index_e13_7493;
extern int index_e11_7493;
extern int index_e5_74161;
extern int index_e6_74161;
extern int index_e7_74161;
extern int index_horiz;
extern int index_scan;
extern int index_fly;
extern int index_video;
extern int index_vert;
extern int index_tos;
extern int index_vsr;
extern int index_vsr_ld;
extern int index_auto_inc;
extern int index_beam_x;
extern int index_beam_y;
extern int index_reg_a;
extern int index_reg_b;
extern int index_reg_x;
extern int index_reg_y;
extern int index_tosj;
extern int index_pc;
extern int index_rom;
extern int index_mem;
extern int index_rx;
extern int index_tx;
