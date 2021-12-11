#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "log.h"
#include "vcd.h"

/* VT52 simulator. */

unsigned int PC;		/* 10 bits */
unsigned char IR;		/* 8 bits */
unsigned char A;		/* 7 bits */
unsigned char B;		/* */
unsigned char X;		/* 7 bits */
unsigned char X8;		/* 1 bit */
unsigned char Y;		/* 5 bits */
unsigned char R;		/* 2 bits */
unsigned char M[2048];		/* 2048x7 */
unsigned char ROM[1024];	/* 1024x8 */
unsigned char MB;		/* 7 bits */

/* Flip-flops. */
static int cursor = 0;
static int video_ff = 0;
static int hz60 = 1;
static int bell;
static int done = 0;
static int write;
static int vsr_ld = 0;
static int auto_inc = 0;

int horiz;
int fly;
int vert;
int tos;
int scan;

static unsigned int address (void);
static void TJ_cycle_mode0 (void);
static void TJ_cycle_mode1 (void);
static void (*TJ_cycle) (void);
#include "defs.h"

static void (*constant) (void);

#define ADDR address ()
#define RAM M[ADDR]

#define INCREMENT_PC                            \
  PC++;                                         \
  PC &= 01777;                                  \
  if ((PC & 0377) == 0)                         \
    R = (R + 1) & 3

#define JUMP(COND)                              \
  a = ROM[PC];                                  \
  INCREMENT_PC;                                 \
  if (COND)                                     \
    PC = a | (R << 8)

static void constant_mode0 (void)
{
  LOG(CPU, ".LD %03o ;mode 0", IR & 0177);
  A++;
  A &= 0177;
  if (A >= RAM) {
    uint8_t mask = 0177;
    if (cursor)
      mask = 037;
    if (!done)
      RAM = IR & mask;
    done = 1;
  }
}

static void constant_mode1 (void)
{
  LOG(CPU, ".LD %03o", IR & 0177);
  RAM = IR & 0x7F;
}

static void TE_cycle (void)
{
  if ((IR & 0x80) == 0)
    done = 0;

  switch (IR & 0xF8) {
  case 0x08:
    LOG (CPU, "ZXZY");
    X8 = X = Y = 0;
    break;
  case 0x18:
    LOG (CPU, "X8");
    X ^= 8;
    break;
  case 0x28:
    LOG (CPU, "IXDY"); //Or IADY?
    X++;
    Y--;
    X &= 0177;
    Y &= 037;
    break;
  case 0x38:
    LOG (CPU, "IX");
    X++;
    X &= 0177;
    break;
  case 0x48:
    LOG (CPU, "ZA");
    A = 0;
    break;
  case 0x58:
    LOG (CPU, "M1");
    constant = constant_mode1;
    TJ_cycle = TJ_cycle_mode1;
    break;
  case 0x68:
    LOG (CPU, "ZX"); //Or 2Z?
    X8 = X = 0;
    break;
  case 0x78:
    LOG (CPU, "M0");
    constant = constant_mode0;
    TJ_cycle = TJ_cycle_mode0;
    break;
  }
}

static void TF_cycle (void)
{
  switch (IR & 0xF4) {
  case 0x04:
    LOG (CPU, "DXDY");
    X--;
    Y--;
    X &= 0177;
    Y &= 037;
    break;
  case 0x14:
    LOG (CPU, "IA");
    A++;
    A &= 0177;
    break;
  case 0x24:
    LOG (CPU, "IA1");
    A++;
    A &= 0177;
    break;
  case 0x34:
    LOG (CPU, "IY");
    Y++;
    Y &= 037;
    break;
  case 0x44:
    LOG (CPU, "DY");
    Y--;
    Y &= 037;
    break;
  case 0x54:
    LOG (CPU, "IROM");
    R++;
    R &= 3;
    break;
  case 0x64:
    LOG (CPU, "DX");
    X--;
    X &= 0177;
    break;
  case 0x74:
    LOG (CPU, "DA");
    A--;
    A &= 0177;
    break;
  }
}

static void TW_cycle (void)
{
  switch (IR & 0xFF) {
  case 0x00:
    LOG (CPU, "SCFF");
    cursor = 1;
    break;
  case 0x10:
    LOG (CPU, "SVID");
    video_ff = 1;
    break;
  case 0x20:
    LOG (CPU, "B2Y");
    Y = B & 037;
    break;
  case 0x30:
    LOG (CPU, "CBFF");
    bell ^= 1;
    break;
  case 0x40:
    LOG (CPU, "ZCAV");
    cursor = video_ff = 0;
    A -= 0020;
    A &= 0177;
    break;
  case 0x50:
    LOG (CPU, "LPB");
    break;
  case 0x60:
    LOG (CPU, "EPR");
    break;
  case 0x70:
    LOG (CPU, "HPR!ZY");
    Y = 0;
    break;
  default:
    if (IR & 0x80)
      constant();
    break;
  }
}

static void TG_cycle (void)
{
  write = 0;
}

static void TH_cycle (void)
{
  INCREMENT_PC;
  switch (IR & 0xF2) {
  case 0x02:
    LOG (CPU, "M2A");
    A = RAM;
    break;
  case 0x12:
    LOG (CPU, "A2M");
    RAM = A;
    break;
  case 0x22:
    LOG (CPU, "M2U");
    uart_tx_data (RAM);
    break;
  case 0x32:
    LOG (CPU, "B2M");
    RAM = B;
    break;
  case 0x42:
    LOG (CPU, "M2X");
    X = RAM;
    break;
  case 0x52:
    LOG (CPU, "U2M");
    RAM = uart_rx_data ();
    break;
  case 0x62:
    LOG (CPU, "M2B");
    B = RAM;
    break;
  case 0x72: 
    LOG (CPU, "SPARE");
    break;
  }
}

static void TJ_cycle_mode0 (void)
{
  int a;

  switch (IR & 0xF1) {
  case 0x01:
    LOG (CPU, "PSCJ");
    JUMP (0); //Printer scan.
    break;
  case 0x11:
    LOG (CPU, "TABJ");
    JUMP ((A & 7) == 7);
    break;
  case 0x21:
    LOG (CPU, "KCLJ");
    JUMP (0); //Key click.
    break;
  case 0x31:
    LOG (CPU, "FRQJ");
    JUMP (hz60);
    break;
  case 0x41:
    LOG (CPU, "PRQJ");
    JUMP (0); //Printer request.
    break;
  case 0x51:
    LOG (CPU, "?");
    exit (1);
    break;
  case 0x61:
    LOG (CPU, "UTJ");
    JUMP (uart_tx_flag ());
    break;
  case 0x71:
    LOG (CPU, "TOSJ");
    JUMP (tos);
    break;
  }
}

static void TJ_cycle_mode1 (void)
{
  int a;

  switch (IR & 0xF1) {
  case 0x01:
    LOG (CPU, "URJ");
    JUMP (uart_rx_flag ());
    break;
  case 0x11:
  case 0x41:
    LOG (CPU, "AEMJ");
    JUMP (A == RAM);
    break;
  case 0x21:
    LOG (CPU, "ALMJ");
    JUMP (A < RAM);
    break;
  case 0x31:
    LOG (CPU, "ADXJ");
    JUMP (A != X);
    break;
  case 0x51:
    LOG (CPU, "TRUJ");
    JUMP (1);
    break;
  case 0x61:
    LOG (CPU, "VSCJ");
    JUMP (!scan);
    break;
  case 0x71:
    LOG (CPU, "KEYJ");
    JUMP (key_flag (A));
    break;
  }
}

static void T2_cycle (void)
{
  if ((!cursor && !video_ff) || fly)
    return;
  vsr_ld = 1;
  if (cursor && A + 1 == X) //Hack to place cursor in right column.
    video_shifter (000);
  else if (video_ff)
    video_shifter (CHAR[(MB << 3) + (A & 7)]);
  MB = RAM;
  if (video_ff) {
    auto_inc = 1;
    X++;
    X &= 0177;
  }
}

static void newline (void)
{
  LOG(CPU, "[A/%03o X/%02o Y/%02o %04o:%03o]\n", A, X, Y, ADDR, RAM);
}

static unsigned int address (void)
{
  int x = X, y = Y >> 1;

  if (x >= 64 || y >= 12) {
    x = x & 017;
    x |= (y & 014) << 2;
    y |= 014;
  }

  /* Least significant bit of Y selects RAM page. */
  if (Y & 1)
    x |= 02000;

  return x | (y << 6);
}

void step (void)
{
#ifdef DEBUG_VCD
  long long time;
#endif
  int t = cycles % 18;

  switch (t)
    {
    case 0:
      IR = ROM[PC];
      //case 1:
      TE_cycle ();
      break;
    case 1:
      T2_cycle ();
      TF_cycle ();
      break;
    case 2:
      vsr_ld = auto_inc = 0;
      break;
    case 6:
      TW_cycle ();
      break;
    case 10:
      T2_cycle ();
      break;
    case 11:
      vsr_ld = auto_inc = 0;
      TG_cycle ();
      break;
    case 12:
      TH_cycle ();
      break;
    case 14:
      TJ_cycle ();
      newline ();
      break;
    }

#ifdef DEBUG_VCD
  time = 1e9 * cycles / 13.824e6;
  vcd_value (time, index_vsr_ld, vsr_ld);
  vcd_value (time, index_auto_inc, auto_inc);
  vcd_value (time, index_reg_a, A);
  vcd_value (time, index_reg_b, B);
  vcd_value (time, index_reg_x, X);
  vcd_value (time, index_reg_y, Y);
  vcd_value (time, index_video, video_ff);
  vcd_value (time, index_pc, PC);
  vcd_value (time, index_rom, ROM[PC]);
  vcd_value (time, index_mem, M[ADDR]);
#endif
}
