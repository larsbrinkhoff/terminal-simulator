#include <SDL.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/select.h>
#include "vt100.h"

static struct termios saved_termios;
static FILE *output;

static void (*normal_table[]) (void);
static void (*altmode_table[]) (void);
static void (*double_altmode_table[]) (void);

#define NBKPT (8+1)
static int trace;
static int breakpoints[NBKPT+1];
static int prefix = -1;
static int infix = -1;
static int *accumulator = &prefix;
static int dot = 0;
static void (**dispatch) (void) = normal_table;
static void (*typeout) (u8 data);
static int radix = 16;
static int type, operand;
static int crlf;
static int clear;
static char ch;
static u16 pc;

static struct { char *name; int type; } dis[] ={
  {"NOP",0},     {"LXI BC,",2}, {"STAX BC",0}, {"INX BC",0},
  {"INR B",0},   {"DCR B",0},   {"MVI B,",1},  {"RLC",0},
  {"(NOP)",0},   {"DAD BC",0},  {"LDAX BC",0}, {"DCX BC",0},
  {"INR C",0},   {"DCR C",0},   {"MVI C,",1},  {"RRC",0},
  {"(NOP)",0},   {"LXI DE,",2}, {"STAX DE",0}, {"INX D",0},
  {"INR D",0},   {"DCR D",0},   {"MVI D,",1},  {"RAL",0},
  {"(NOP)",0},   {"DAD DE",0},  {"LDAX DE",0}, {"DCX DE",0},
  {"INR E",0},   {"DCR E",0},   {"MVI E,",1},  {"RAR",0},
  {"(NOP)",0},   {"LXI HL,",2}, {"SHLD",0},    {"INX HL",0},
  {"INR HL",0},  {"DCR HL",0},  {"MVI H,",1},  {"DAA",0},
  {"(NOP)",0},   {"DAD HL",0},  {"LHLD ",2},   {"DCX HL",0},
  {"INR L",0},   {"DCR L",0},   {"MVI L,",1},  {"CMA",0},
  {"(NOP)",0},   {"LXI SP,",2}, {"STA ",2},    {"INX SP",0},
  {"INR M",0},   {"DCR M",0},   {"MVI M,",1},  {"STC",0},
  {"(NOP)",0},   {"DAD SP",0},  {"LDA ",2},    {"DCX SP",0},
  {"INR A",0},   {"DCR A",0},   {"MVI A,",1},  {"CMC",0},
  {"MOV B,B",0}, {"MOV B,C",0}, {"MOV B,D",0}, {"MOV B,E",0},
  {"MOV B,H",0}, {"MOV B,L",0}, {"MOV B,M",0}, {"MOV B,A",0},
  {"MOV C,B",0}, {"MOV C,C",0}, {"MOV C,D",0}, {"MOV C,E",0},
  {"MOV C,H",0}, {"MOV C,L",0}, {"MOV C,M",0}, {"MOV C,A",0},
  {"MOV D,B",0}, {"MOV D,C",0}, {"MOV D,D",0}, {"MOV D,E",0},
  {"MOV D,H",0}, {"MOV D,L",0}, {"MOV D,M",0}, {"MOV D,A",0},
  {"MOV E,B",0}, {"MOV E,C",0}, {"MOV E,D",0}, {"MOV E,E",0},
  {"MOV E,H",0}, {"MOV E,L",0}, {"MOV E,M",0}, {"MOV E,A",0},
  {"MOV H,B",0}, {"MOV H,C",0}, {"MOV H,D",0}, {"MOV H,E",0},
  {"MOV H,H",0}, {"MOV H,L",0}, {"MOV H,M",0}, {"MOV H,A",0},
  {"MOV L,B",0}, {"MOV L,C",0}, {"MOV L,D",0}, {"MOV L,E",0},
  {"MOV L,H",0}, {"MOV L,L",0}, {"MOV L,M",0}, {"MOV L,A",0},
  {"MOV M,B",0}, {"MOV M,C",0}, {"MOV M,D",0}, {"MOV M,E",0},
  {"MOV M,H",0}, {"MOV M,L",0}, {"HLT",0},     {"MOV M,A",0},
  {"MOV A,B",0}, {"MOV A,C",0}, {"MOV A,D",0}, {"MOV A,E",0},
  {"MOV A,H",0}, {"MOV A,L",0}, {"MOV A,M",0}, {"MOV A,A",0},

  {"ADD B",0},   {"ADD C",0},   {"ADD D",0},   {"ADD E",0},
  {"ADD H",0},   {"ADD L",0},   {"ADD M",0},   {"ADD A",0},
  {"ADC B",0},   {"ADC C",0},   {"ADC D",0},   {"ADC E",0},
  {"ADC H",0},   {"ADC L",0},   {"ADC M",0},   {"ADC A",0},
  {"SUB B",0},   {"SUB C",0},   {"SUB D",0},   {"SUB E",0},
  {"SUB H",0},   {"SUB L",0},   {"SUB M",0},   {"SUB A",0},
  {"SBB B",0},   {"SBB C",0},   {"SBB D",0},   {"SBB E",0},
  {"SBB H",0},   {"SBB L",0},   {"SBB M",0},   {"SBB A",0},
  {"ANA B",0},   {"ANA C",0},   {"ANA D",0},   {"ANA E",0},
  {"ANA H",0},   {"ANA L",0},   {"ANA M",0},   {"ANA A",0},
  {"XRA B",0},   {"XRA C",0},   {"XRA D",0},   {"XRA E",0},
  {"XRA H",0},   {"XRA L",0},   {"XRA M",0},   {"XRA A",0},
  {"ORA B",0},   {"ORA C",0},   {"ORA D",0},   {"ORA E",0},
  {"ORA H",0},   {"ORA L",0},   {"ORA M",0},   {"ORA A",0},
  {"CMP B",0},   {"CMP C",0},   {"CMP D",0},   {"CMP E",0},
  {"CMP H",0},   {"CMP L",0},   {"CMP M",0},   {"CMP A",0},

  {"RNZ",0},     {"POP BC",0},  {"JNZ ",2},    {"JMP ",2},
  {"CNZ ",2},    {"PUSH BC",0}, {"ADI ",1},    {"RST 0",0},
  {"RZ ",0},     {"RET",0},     {"JZ ",2},     {"(JMP) ",2},
  {"CZ ",2},     {"CALL ",2},   {"ACI ",1},    {"RST 1",0},

  {"RNC",0},     {"POP DE",0},  {"JNC ",2},    {"OUT ",1},
  {"CNC ",2},    {"PUSH DE",0}, {"SUI ",1},    {"RST 2",0},
  {"RC ",0},     {"(RET)",0},   {"JC ",2},     {"IN ",1},
  {"CC ",2},     {"(CALL) ",2}, {"SBI ",1},    {"RST 3",0},

  {"RPO",0},     {"POP HL",0},  {"JPO ",2},    {"XTHL",0},
  {"CPO ",2},    {"PUSH HL",0}, {"ANI ",1},    {"RST 4",0},
  {"RPE",0},     {"PCHL",0},    {"JPE ",2},    {"XCHG",0},
  {"CPE ",2},    {"(CALL) ",2}, {"XRI ",1},    {"RST 5",0},

  {"RP ",0},     {"POP PSW",0}, {"JP ",2},     {"DI",0},
  {"CP ",2},     {"PUSH PSW",0},{"ORI ",1},    {"RST 6",0},
  {"RM",0},      {"SPHL",0},    {"JM ",2},     {"EI",0},
  {"CM ",2},     {"(CALL) ",2}, {"CPI ",1},    {"RST 7",0}
};

static void altmode (void)
{
  dispatch = altmode_table;
  clear = 0;
}

static void double_altmode (void)
{
  dispatch = double_altmode_table;
  clear = 0;
}

static void breakpoint (void)
{
  int i;
  if (prefix == -1) {
    if (infix != -1)
      breakpoints[infix] = -1;
    return;
  }
  if (infix != -1) {
    breakpoints[infix] = prefix;
    return;
  }
  for (i = 1; i < NBKPT; i++) {
    if (breakpoints[i] == -1) {
      breakpoints[i] = prefix;
      return;
    }
  }
  fprintf (output, "TOO MANY PTS? ");
}

static void clear_breakpoints (void)
{
  int i;
  for (i = 0; i < NBKPT+1; i++)
    breakpoints[i] = -1;
}

static void number (void)
{
  if (ch >= '0' && ch <= '9')
    ch -= '0';
  else if (ch >= 'A' && ch <= 'F')
    ch += -'A' + 10;
  else
    ch += -'a' + 10;
  if (*accumulator == -1)
    *accumulator = ch;
  else
    *accumulator = 16 * *accumulator + ch;
  clear = 0;
}

static void symbolic (u8 data)
{
  fprintf (output, "%s", dis[data].name);
  switch (type) {
  case 0: break;
  case 1: fprintf (output, "%02X", memory[operand]); break;
  case 2: fprintf (output, "%02X%02X", memory[operand+1], memory[operand]); break;
  }
  fprintf (output, "   ");
}

static void equals (void)
{
  int digit;
  if (prefix == -1)
    return;
  digit = prefix % radix;
  prefix /= radix;
  if (prefix > 0)
    equals ();
  fprintf (output, "%X", digit);
}

static void constant (u8 data)
{
  prefix = memory[dot];
  equals ();
}

static void carriagereturn (void)
{
  if (prefix != -1)
    memory[dot] = prefix;
}

static void slash (void)
{
  if (prefix != -1)
    dot = prefix;
  operand = dot + 1;
  type = dis[memory[dot]].type;
  fprintf (output, "   ");
  typeout (memory[dot]);
  fprintf (output, "   ");
  crlf = 0;
}

static void linefeed (void)
{
  carriagereturn ();
  if (typeout == symbolic)
    dot += dis[memory[dot]].type + 1;
  else
    dot++;
  prefix = dot;
  fprintf (output, "\r\n%04X/", prefix);
  slash ();
}

static void tab (void)
{
  carriagereturn ();
  prefix = memory[dot + 1] | memory[dot + 2] << 8;
  fprintf (output, "\r\n%04X/", prefix);
  slash ();
}

static u16 previous (u16 addr)
{
  if (dis[memory[addr - 3]].type == 2)
    return addr - 3;
  else if (dis[memory[addr - 2]].type == 1)
    return addr - 2;
  else
    return addr - 1;
}

static void caret (void)
{
  carriagereturn ();
  if (typeout == symbolic)
    dot = previous (dot);
  else
    dot--;
  prefix = dot;
  fprintf (output, "\r\n%04X/", prefix);
  slash ();
}

static void temporarily (void (*mode) (u8), void (*fn) (void))
{
  void (*saved) (u8) = typeout;
  typeout = mode;
  fn ();
  typeout = saved;
}

static void rbracket (void)
{
  temporarily (constant, slash);
}

static void lbracket (void)
{
  temporarily (symbolic, slash);
}

static void control_c (void)
{
  fputs ("\r\n", output);
}

static void control_g (void)
{
  fputs (" QUIT? ", output);
}

static void control_t (void)
{
  trace = !trace;
}

static void kbd_rdy (void)
{
  vt100_flags |= 0x80;
}

static EVENT (kbd_event, kbd_rdy);

static void no_kbd_out (u8 port, u8 data)
{
  vt100_flags &= ~0x80;
  add_event (3520, &kbd_event);
}

static u8 no_kbd_in (u8 port)
{
  LOG (SYS, "Keyboard disconnected");
  clear_interrupt (1);
  return 0x7F;
}

static void control_k (void)
{
  static u8 (*keyboard_in) (u8) = NULL;
  static void (*keyboard_out) (u8, u8) = NULL;
  if (keyboard_in == NULL)
    keyboard_in = device_in[0x82];
  if (keyboard_out == NULL)
    keyboard_out = device_out[0x82];

  if (prefix <= 0) {
    device_in[0x82] = no_kbd_in;
    device_out[0x82] = no_kbd_out;
  } else {
    device_in[0x82] = keyboard_in;
    device_out[0x82] = keyboard_out;
  }
}

static void control_e (void)
{
  fprintf (output, "\r\n");
  print_events (output);
}

static void release (void);
static EVENT (release_event, release);

static void release (void)
{
  key_up (0x66);
}

static int control_z (void)
{
  struct timeval tv;
  fd_set fds;
  FD_ZERO (&fds);
  FD_SET (fileno (stdin), &fds);
  tv.tv_sec = tv.tv_usec = 0;
  if (select (1, &fds, NULL, NULL, &tv) > 0) {
    int c = getchar ();
    LOG (DDT, "Typed %02X", c);
    if (c == 007 || c == 032) {
      fputs ("^Z\r\n", output);
      return 1;
    }
    key_down (0x66);
    add_event (2764800 / 50, &release_event);
  }
  return 0;
}

static void stop (u16 addr)
{
  breakpoints[0] = addr;
  jump (addr);
}

static void stopped (char *x)
{
  fprintf (output, "%04X%s", pc, x);
  operand = pc + 1;
  breakpoints[0] = -1;
  type = dis[memory[pc]].type;
  symbolic (memory[pc]);
  crlf = 0;
}

static void step (void)
{
  unsigned long long previous = get_cycles ();
  u8 insn = memory[pc];
  u16 start = pc;

  jump (pc);
  pc = execute ();
  events (get_cycles () - previous);

  if (trace) {
    u16 reg_SP;
    u8 reg[10];
    cpu_state (&reg_SP, reg);
    LOG (CPU,
         "%04X: %02X  %02X:%02X %02X:%02X %02X:%02X %02X  %c%c%c%c%c:%c",
         start, insn, 
         reg[2], reg[3], reg[4], reg[5], reg[6], reg[7], reg[9],
         reg[0] & 0x80 ? 'S' : '-',
         reg[0] & 0x40 ? 'Z' : '-',
         reg[0] & 0x10 ? 'A' : '-',
         reg[0] & 0x04 ? 'P' : '-',
         reg[0] & 0x01 ? 'C' : '-',
         reg[1] ? 'I' : '-');
  }
}

static void proceed (void)
{
  int n, i;

  fprintf (output, "\r\n");
  fflush (output);

  for (n = 1;; n++) {
    step ();
    for (i = 0; i < NBKPT+1; i++) {
      if (pc == breakpoints[i]) {
        if (i > 0)
          fprintf (output, "$%dB; ", i);
        stopped (">>");
        return;
      }
    }
    if (control_z ()) {
      stopped (")   ");
      return;
    }
  }
}

static void go (void)
{
  if (prefix == -1)
    jump (pc = starta);
  else
    jump (pc = prefix);
  proceed ();
}

static void oneproceed (void)
{
  fprintf (output, "\r\n");
  fflush (output);
  step ();
  stopped (">>");
}

static void next (void)
{
  breakpoints[NBKPT] = pc + 1 + dis[memory[pc]].type;
  proceed ();
  breakpoints[NBKPT] = -1;
}

static void rubout (void)
{
}

static void error (void)
{
  fprintf (output, " OP? ");
}

static char buffer[100];

static char *line (void)
{
  char *p = buffer;
  char c;

  for (;;) {
    c = getchar();
    if (c == 015)
      return buffer;
    *p++ = c;
    fputc (c, output);
  }
}

static void load (void)
{
  char *file;
  u16 offset = 0;
  FILE *f;

  fputc (' ', output);
  file = line ();
  f = fopen (file, "rb");
  if (f == NULL) {
    fprintf (output, "\r\n%s - file not found", file);
    return;
  }
  if (prefix != -1)
    offset = prefix;
  if (dispatch == altmode_table)
    memset (memory, 0, 0x10000);
  if (fread (memory + offset, 1, 0x10000 - offset, f) <= 0)
    fprintf (output, "\r\n%s - error reading file", file);
  fclose (f);
  if (prefix == 0x100) {
    void bdos_out (u8 port, u8 data);
    register_port (0xFF, NULL, bdos_out);
    memory[0] = 0x76;
    memory[5] = 0xD3;
    memory[6] = 0xFF;
    memory[7] = 0xC9;
  }
}

static void dump (void)
{
  char *file;
  u16 offset = 0;
  FILE *f;

  fputc (' ', output);
  file = line ();
  f = fopen (file, "wb");
  if (f == NULL) {
    fprintf (output, "\r\n%s - error opening file\r\n", file);
    return;
  }
  if (prefix != -1)
    offset = prefix;
  if (fwrite (memory + offset, 1, 0x10000 - offset, f) <= 0)
    fprintf (output, "\r\n%s - error writing file\r\n", file);
  fclose (f);
}

static void zero (void)
{
  u16 offset = 0;
  if (prefix != -1)
    offset = prefix;
  memset (memory, offset, 0x10000 - offset);
}

static void login (void)
{
  fprintf (output, "Welcome!\r\n");
}

static void logout (void)
{
  tcsetattr (fileno (stdin), TCSAFLUSH, &saved_termios);
  fprintf (output, "\r\n");
  exit (0);
}

static void period (void)
{
  prefix = dot;
  clear = 0;
}

static void altmode_period (void)
{
  prefix = pc;
  clear = 0;
}

static void altmode_s (void)
{
  typeout = symbolic;
}

static void altmode_c (void)
{
  typeout = constant;
}

static void altmode_d (void)
{
  radix = 10;
  typeout = constant;
}

static void altmode_o (void)
{
  radix = 8;
  typeout = constant;
}

static void altmode_h (void)
{
  radix = 16;
  typeout = constant;
}

static void space (void)
{
}

static void listj (void)
{
  fprintf (output, "* CPU\r\n");
  fprintf (output, "  VIDEO\r\n");
  fprintf (output, "  KEYBOARD\r\n");
  fprintf (output, "  PUSART\r\n");
  fprintf (output, "  NVM");
}

static char *mname[3] = { "RAM", "ROM", "NXM" };

static void cortyp (char *arg)
{
  u16 addr;
  sscanf (arg, "%hx", &addr);
  fprintf (output, "\r\n%04X, %s", addr, mname[mtype[addr >> 10]]);
}

static void corprt (void)
{
  u16 start = 0;
  int i, type = mtype[0];

  for (i = 1024; i < 65*1024; i += 1024) {
    if (i == 64*1024 || mtype[i >> 10] != type) {
      if (type != 2)
        fprintf (output, "\r\n%04X-%04X, %s", start, i - 1, mname[type]);
      type = mtype[i >> 10];
      start = i;
    }
  }
}

static void corblk (char *arg)
{
  //ram, rom, nxm
}

static void colon (void)
{
  char *command = line ();
  if (strncasecmp (command, "cortyp", 6) == 0)
    cortyp (command + 6);
  else if (strncasecmp (command, "corprt", 6) == 0)
    corprt ();
  else if (strncasecmp (command, "corblk", 6) == 0)
    corblk (command + 6);
}

static void (*normal_table[]) (void) = {
  error,                  //^@
  error,                  //^A
  error,                  //^B
  control_c,              //^C
  error,                  //^D
  control_e,              //^E
  error,                  //^F
  control_g,              //^G
  error,                  //^H
  tab,                    //^I
  linefeed,               //^J
  control_k,              //^K
  error,                  //^L
  carriagereturn,         //^M
  oneproceed,             //^N
  error,                  //^O
  error,                  //^P
  error,                  //^Q
  error,                  //^R
  error,                  //^S
  control_t,              //^T
  error,                  //^U
  error,                  //^V
  error,                  //^W
  error,                  //^X
  error,                  //^Y
  error,                  //^Z
  altmode,                //^[
  error,                  //control-backslash
  error,                  //^]
  error,                  //^^
  error,                  //^_
  space,                  // 
  error,                  //!
  error,                  //"
  error,                  //#
  error,                  //$
  error,                  //%
  error,                  //&
  error,                  //'
  error,                  //(
  error,                  //)
  error,                  //*
  error,                  //+
  error,                  //,
  error,                  //-
  period,                 //.
  slash,                  ///
  number,                 //0
  number,                 //1
  number,                 //2
  number,                 //3
  number,                 //4
  number,                 //5
  number,                 //6
  number,                 //7
  number,                 //8
  number,                 //9
  colon,                  //:
  error,                  //;
  error,                  //<
  equals,                 //=
  error,                  //>
  error,                  //?
  error,                  //@
  number,                 //A
  number,                 //B
  number,                 //C
  number,                 //D
  number,                 //E
  number,                 //F
  error,                  //G
  error,                  //H
  error,                  //I
  error,                  //J
  error,                  //K
  error,                  //L
  error,                  //M
  error,                  //N
  error,                  //O
  error,                  //P
  error,                  //Q
  error,                  //R
  error,                  //S
  error,                  //T
  error,                  //U
  error,                  //V
  error,                  //W
  error,                  //X
  error,                  //Y
  error,                  //Z
  rbracket,               //[
  error,                  //backslash
  lbracket,               //]
  caret,                  //^
  error,                  //_
  error,                  //`
  number,                 //a
  number,                 //b
  number,                 //c
  number,                 //d
  number,                 //e
  number,                 //f
  error,                  //g
  error,                  //h
  error,                  //i
  error,                  //j
  error,                  //k
  error,                  //l
  error,                  //m
  error,                  //n
  error,                  //o
  error,                  //p
  error,                  //q
  error,                  //r
  error,                  //s
  error,                  //t
  error,                  //u
  error,                  //v
  error,                  //w
  error,                  //x
  error,                  //y
  error,                  //z
  error,                  //{
  error,                  //|
  error,                  //}
  error,                  //~
  rubout,                 //^?
};

static void (*altmode_table[]) (void) = {
  error,                  //^@
  error,                  //^A
  error,                  //^B
  error,                  //^C
  error,                  //^D
  error,                  //^E
  error,                  //^F
  error,                  //^G
  error,                  //^H
  error,                  //^I
  error,                  //^J
  error,                  //^K
  error,                  //^L
  error,                  //^M
  next,                   //^N
  error,                  //^O
  error,                  //^P
  error,                  //^Q
  error,                  //^R
  error,                  //^S
  error,                  //^T
  error,                  //^U
  error,                  //^V
  error,                  //^W
  error,                  //^X
  error,                  //^Y
  error,                  //^Z
  double_altmode,         //^[
  error,                  //control-backslash
  error,                  //^]
  error,                  //^^
  error,                  //^_
  error,                  // 
  error,                  //!
  error,                  //"
  error,                  //#
  error,                  //$
  error,                  //%
  error,                  //&
  error,                  //'
  error,                  //(
  error,                  //)
  error,                  //*
  error,                  //+
  error,                  //,
  error,                  //-
  altmode_period,         //.
  error,                  ///
  error,                  //0
  error,                  //1
  error,                  //2
  error,                  //3
  error,                  //4
  error,                  //5
  error,                  //6
  error,                  //7
  error,                  //8
  error,                  //9
  error,                  //:
  error,                  //;
  error,                  //<
  error,                  //=
  error,                  //>
  error,                  //?
  error,                  //@
  error,                  //A
  breakpoint,             //B
  altmode_c,              //C
  altmode_d,              //D
  error,                  //E
  error,                  //F
  go,                     //G
  altmode_h,              //H
  error,                  //I
  error,                  //J
  error,                  //K
  load,                   //L
  error,                  //M
  error,                  //N
  altmode_o,              //O
  proceed,                //P
  error,                  //Q
  error,                  //R
  altmode_s,              //S
  error,                  //T
  login,                  //U
  error,                  //V
  error,                  //W
  error,                  //X
  dump,                   //Y
  error,                  //Z
  error,                  //[
  error,                  //backslash
  error,                  //]
  error,                  //^
  error,                  //_
  error,                  //`
  error,                  //a
  breakpoint,             //b
  altmode_c,              //c
  altmode_d,              //d
  error,                  //e
  error,                  //f
  go,                     //g
  altmode_h,              //h
  error,                  //i
  error,                  //j
  error,                  //k
  load,                   //L
  error,                  //m
  error,                  //n
  altmode_o,              //o
  proceed,                //p
  error,                  //q
  error,                  //r
  altmode_s,              //s
  error,                  //t
  login,                  //u
  error,                  //v
  error,                  //w
  error,                  //x
  dump,                   //y
  error,                  //z
  error,                  //{
  error,                  //|
  error,                  //}
  error,                  //~
  error,                  //^?
};

static void (*double_altmode_table[]) (void) = {
  error,                  //^@
  error,                  //^A
  error,                  //^B
  error,                  //^C
  error,                  //^D
  error,                  //^E
  error,                  //^F
  error,                  //^G
  error,                  //^H
  error,                  //^I
  error,                  //^J
  error,                  //^K
  error,                  //^L
  error,                  //^M
  error,                  //^N
  error,                  //^O
  error,                  //^P
  error,                  //^Q
  error,                  //^R
  error,                  //^S
  error,                  //^T
  error,                  //^U
  error,                  //^V
  error,                  //^W
  error,                  //^X
  error,                  //^Y
  error,                  //^Z
  error,                  //^[
  error,                  //control-backslash
  error,                  //^]
  error,                  //^^
  error,                  //^_
  error,                  // 
  error,                  //!
  error,                  //"
  error,                  //#
  error,                  //$
  error,                  //%
  error,                  //&
  error,                  //'
  error,                  //(
  error,                  //)
  error,                  //*
  error,                  //+
  error,                  //,
  error,                  //-
  error,                  //.
  error,                  ///
  error,                  //0
  error,                  //1
  error,                  //2
  error,                  //3
  error,                  //4
  error,                  //5
  error,                  //6
  error,                  //7
  error,                  //8
  error,                  //9
  error,                  //:
  error,                  //;
  error,                  //<
  error,                  //=
  error,                  //>
  error,                  //?
  error,                  //@
  error,                  //A
  clear_breakpoints,      //B
  error,                  //C
  error,                  //D
  error,                  //E
  error,                  //F
  error,                  //G
  error,                  //H
  error,                  //I
  error,                  //J
  error,                  //K
  load,                   //L
  error,                  //M
  error,                  //N
  error,                  //O
  error,                  //P
  error,                  //Q
  error,                  //R
  error,                  //S
  error,                  //T
  logout,                 //U
  listj,                  //V
  error,                  //W
  error,                  //X
  dump,                   //Y
  zero,                   //Z
  error,                  //[
  error,                  //backslash
  error,                  //]
  error,                  //^
  error,                  //_
  error,                  //`
  error,                  //a
  clear_breakpoints,      //b
  error,                  //c
  error,                  //d
  error,                  //e
  error,                  //f
  error,                  //g
  error,                  //h
  error,                  //i
  error,                  //j
  error,                  //k
  load,                   //l
  error,                  //m
  error,                  //n
  error,                  //o
  error,                  //p
  error,                  //q
  error,                  //r
  error,                  //s
  error,                  //t
  logout,                 //u
  listj,                  //v
  error,                  //w
  error,                  //x
  dump,                   //y
  zero,                   //z
  error,                  //{
  error,                  //|
  error,                  //}
  error,                  //~
  error,                  //^?
};

static void cbreak (void)
{
  struct termios t;

  if (tcgetattr (fileno (stdin), &t) == -1)
    return;

  saved_termios = t;
  t.c_lflag &= ~(ICANON | ECHO);
  t.c_lflag |= ISIG;
  t.c_iflag &= ~ICRNL;
  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;

  atexit (logout);

  if (tcsetattr (fileno (stdin), TCSAFLUSH, &t) == -1)
    return;
}

static void echo (char c)
{
  if (c == 015)
    ;
  else if (c == 033)
    fprintf (output, "$");
  else if (c < 32)
    fprintf (output, "^%c", c + '@');
  else if (c == 0177)
    fprintf (output, "^?");
  else
    fprintf (output, "%c", c);
  fflush (output);
}

static void key (char c)
{
  void (**before) (void) = dispatch;
  if (c & -128)
    return;
  echo (c);
  clear = 1;
  crlf = 1;
  ch = c;
  dispatch[(int)c] ();
  if (dispatch == before)
    dispatch = normal_table;
  if (clear) {
    prefix = -1;
    infix = -1;
    accumulator = &prefix;
    if (crlf)
      fprintf (output, "\r\n");
  }
  fflush (output);
}

static int thread (void *arg)
{
  for (;;)
    key (getchar ());
  return 0;
}

void ddt (void)
{
  output = stdout;
  cbreak ();
  trace = 0;
  halt = stop;
  typeout = symbolic;
  radix = 16;
  prefix = infix = -1;
  clear_breakpoints ();
  SDL_CreateThread (thread, "vt100: DDT", NULL);
}
