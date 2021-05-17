#include "vt100.h"

static unsigned long long cycles;

static u8 reg[8];
static u16 reg_SP;
static u16 reg_PC;
static u16 result;
static u8 aux_carry;
static u8 sign_parity;
u8 mtype[64];

static u8 (*opcode) (void);

#define reg_B reg[0]
#define reg_C reg[1]
#define reg_D reg[2]
#define reg_E reg[3]
#define reg_H reg[4]
#define reg_L reg[5]
//      memory    6
#define reg_A reg[7]

#define reg_BC (reg_B << 8 | reg_C)
#define reg_DE (reg_D << 8 | reg_E)
#define reg_HL (reg_H << 8 | reg_L)

#define flag_C  0x01
#define flag_P  0x04
#define flag_A  0x10
#define flag_Z  0x40
#define flag_S  0x80
#define CARRY  0x100

static int irq;

unsigned long long get_cycles (void)
{
  return cycles;
}

static void add_cycles (unsigned n)
{
  cycles += n;
}

static u8 load (u16 addr)
{
  return memory[addr];
}

static u16 load16 (u16 addr)
{
  u16 data = load (addr);
  return data | (u16)load (addr + 1) << 8;
}

static u8 fetch (void)
{
  return load (reg_PC++);
}

static u8 ifetch (void)
{
  int insn = irq;
  if (insn >= 0) {
    opcode = fetch;
    return insn;
  }
  return fetch ();
}

void interrupt (int rst)
{
  irq = rst;
}

static u16 fetch16 (void)
{
  u16 data = fetch ();
  data |= (u16)fetch() << 8;
  return data;
}

static void store (u16 addr, u8 data)
{
  if (mtype[addr >> 10] == 0)
    memory[addr] = data;
}

static void store16 (u16 addr, u16 data)
{
  store (addr, data);
  store (addr + 1, data >> 8);
}

static u8 alu (u8 insn, u8 x, u8 y)
{
  switch (insn) {
  case 0x00: //ADD, ADI
    result = x + y;
    aux_carry = x ^ y;
    break;
  case 0x08: //ADC, ACI
    result = x + y + (result >> 8);
    aux_carry = x ^ y;
    break;
  case 0x01: //INR
    result = (x + 1) & 0xFF | (result & CARRY);
    aux_carry = x;
    break;
  case 0x18: //SBB, SBI
    y ^= 0xFF;
    result ^= CARRY;
    alu (0x08, x, y);
    result ^= CARRY;
    break;
  case 0x10: //SUB, SUI
  case 0x38: //CMP, CPI
    y ^= 0xFF;
    result = x + y + 1;
    aux_carry = x ^ y;
    result ^= CARRY;
    break;
  case 0x02: //DCR
    result = (x - 1) & 0xFF | (result & CARRY);
    aux_carry = x ^ 0xFF;
    break;
  case 0x20: //ANA, ANI
    aux_carry = result = x & y;
    aux_carry ^= (x | y) << 1;
    break;
  case 0x28: //XRA, XRI
    aux_carry = result = x ^ y;
    break;
  case 0x30: //ORA, ORI
    aux_carry = result = x | y;
    break;
  default:
    fprintf (stderr, "Error in ALU.\n");
    exit (1);
  }
  return sign_parity = result;
}

static u8 *dst (u8 insn)
{
  return &reg[(insn >> 3) & 7];
}

static u8 src (u8 insn)
{
  return reg[insn & 7];
}

static u8 parity[] = {
  4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
  0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,
  0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,
  4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
  0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,
  4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
  4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
  0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4
};

static int condition (u8 insn)
{
  if (insn & 0x08)
    return !condition (insn & 0x30);

  switch (insn & 0x30) {
  case 0x00: return result & 0xFF;
  case 0x10: return !(result & CARRY);
  case 0x20: return !parity[sign_parity];
  case 0x30: return !(sign_parity & flag_S);
  default:   return 0;
  }
}

static u8 get_flags (void)
{
  return ((aux_carry ^ result) & flag_A) | (result >> 8)
    | (result & 0xFF ? 0 : flag_Z) | (sign_parity & flag_S)
    | parity[sign_parity] | 0x02;
}

static u16 get_pair (u8 insn)
{
  switch (insn & 0xB0) {
  case 0x00: case 0x80: return reg_BC;
  case 0x10: case 0x90: return reg_DE;
  case 0x20: case 0xA0: return reg_HL;
  case 0x30: return reg_SP;
  case 0xB0: return reg_A << 8 | get_flags ();
  default:
    fprintf (stderr, "Error in CPU.\n");
    exit (1);
  }
}

static void set_flags (u8 data)
{
  result = data & flag_Z ? 0 : 1;
  result |= (data << 8) & CARRY;
  aux_carry = data ^ result;
  sign_parity = data & flag_P ? 0 : 1;
  sign_parity ^= data & flag_S ? 0x81 : 0;
}

static void set_pair (u8 insn, u16 data)
{
  switch (insn & 0xB0) {
  case 0x00: case 0x80: reg_C = data; reg_B = data >> 8; break;
  case 0x10: case 0x90: reg_E = data; reg_D = data >> 8; break;
  case 0x20: case 0xA0: reg_L = data; reg_H = data >> 8; break;
  case 0x30: reg_SP = data; break;
  case 0xB0: set_flags (data); reg_A = data >> 8; break;
  default:
    fprintf (stderr, "Error in CPU.\n");
    exit (1);
  }
}

static void nop (u8 insn)
{
}

static void lxi (u8 insn)
{
  set_pair (insn, fetch16 ());
}

static void stax (u8 insn)
{
  store (get_pair (insn), reg_A);
}

static void inx (u8 insn)
{
  set_pair (insn, get_pair (insn) + 1);
}

static void inr (u8 insn)
{
  *dst (insn) = alu (0x01, *dst (insn), 1);
}

static void inrm (u8 insn)
{
  store (reg_HL, alu (0x01, load (reg_HL), 1));
}

static void dcr (u8 insn)
{
  *dst (insn) = alu (0x02, *dst (insn), 1);
}

static void dcrm (u8 insn)
{
  store (reg_HL, alu (0x02, load (reg_HL), 1));
}

static void mvi (u8 insn)
{
  *dst (insn) = fetch ();
}

static void mvim (u8 insn)
{
  store (reg_HL, fetch ());
}

static void rlc (u8 insn)
{
  result = (result & 0xFF) | ((reg_A << 1) & CARRY);
  reg_A = reg_A << 1 | reg_A >> 7;
}

static void dad (u8 insn)
{
  u16 data = reg_HL;
  data += get_pair (insn);
  result = (result & 0xFF) | ((data < reg_HL) << 8);
  set_pair (0x20, data);
}

static void ldax (u8 insn)
{
  reg_A = load (get_pair (insn));
}

static void dcx (u8 insn)
{
  set_pair (insn, get_pair (insn) - 1);
}

static void rrc (u8 insn)
{
  reg_A = reg_A >> 1 | reg_A << 7;
  result = (result & 0xFF) | ((reg_A << 1) & CARRY);
}

static void ral (u8 insn)
{
  u8 carry = result >> 8;
  result = (result & 0xFF) | ((reg_A << 1) & CARRY);
  reg_A = (reg_A << 1) | carry;
}

static void rar (u8 insn)
{
  u8 carry = result >> 8;
  result = (result & 0xFF) | ((reg_A << 8) & CARRY);
  reg_A = (reg_A >> 1) | (carry << 7);
}

static void shld (u8 insn)
{
  store16 (fetch16 (), reg_HL);
}

static void daa (u8 insn)
{
  u16 carry = 0;
  u8 act = 0;
  if ((reg_A & 0xF) > 9 || ((aux_carry ^ result) & flag_A))
    act = 0x06;
  if (reg_A > 0x99 || (result & CARRY)) {
    act |= 0x60;
    carry = CARRY;
  }
  reg_A = alu (0x00, reg_A, act);
  result = (result & 0xFF) | carry;
}

static void lhld (u8 insn)
{
  set_pair (0x20, load16 (fetch16 ()));
}

static void cma (u8 insn)
{
  reg_A = ~reg_A;
}

static void sta (u8 insn)
{
  store (fetch16 (), reg_A);
}

static void stc (u8 insn)
{
  result |= CARRY;
}

static void lda (u8 insn)
{
  reg_A = load (fetch16 ());
}

static void cmc (u8 insn)
{
  result ^= CARRY;
}

static void mov (u8 insn)
{
  *dst (insn) = src (insn);
}

static void movm (u8 insn)
{
  store (reg_HL, src (insn));
}

static void mov_m (u8 insn)
{
  *dst (insn) = load (reg_HL);
}

static void arit (u8 insn)
{
  reg_A = alu (insn & 0x38, reg_A, src (insn));
}

static void aritm (u8 insn)
{
  reg_A = alu (insn & 0x38, reg_A, load (reg_HL));
}

static void cmp (u8 insn)
{
  alu (insn & 0x38, reg_A, src (insn));
}

static void cmpm (u8 insn)
{
  alu (insn & 0x38, reg_A, load (reg_HL));
}

void jump (u16 addr)
{
  reg_PC = addr;
}

static void ret (u8 insn)
{
  jump (load16 (reg_SP));
  reg_SP += 2;
}

static void r (u8 insn)
{
  if (condition (insn)) {
    add_cycles (6);
    ret (insn);
  }
}

static void pop (u8 insn)
{
  set_pair (insn, load16 (reg_SP));
  reg_SP += 2;
}

static void j (u8 insn)
{
  u16 addr = fetch16 ();
  if (condition (insn))
    jump (addr);
}

static void jmp (u8 insn)
{
  jump (fetch16 ());
}

static void call (u8 insn)
{
  reg_SP -= 2;
  store16 (reg_SP, reg_PC + 2);
  jump (fetch16 ());
}

static void c (u8 insn)
{
  if (condition (insn)) {
    call (insn);
    add_cycles (6);
  } else
    jump (reg_PC + 2);
}

static void push (u8 insn)
{
  reg_SP -= 2;
  store16 (reg_SP, get_pair (insn));
}

static void imm (u8 insn)
{
  reg_A = alu (insn & 0x38, reg_A, fetch ());
}

static void rst (u8 insn)
{
  reg_SP -= 2;
  store16 (reg_SP, reg_PC);
  jump (insn & 0x38);
}

static void out (u8 insn)
{
  u8 port = fetch ();
  device_out[port](port, reg_A);
}

static void in (u8 insn)
{
  u8 port = fetch ();
  reg_A = device_in[port](port);
}

static void xthl (u8 insn)
{
  u16 tmp = load16 (reg_SP);
  store16 (reg_SP, reg_HL);
  set_pair (0x20, tmp);
}

static void xchg (u8 insn)
{
  u16 tmp = reg_DE;
  set_pair (0x10, reg_HL);
  set_pair (0x20, tmp);
}

static void di (u8 insn)
{
  opcode = fetch;
}

static void ei (u8 insn)
{
  opcode = ifetch;
}

static void cpi (u8 insn)
{
  alu (insn & 0x38, reg_A, fetch ());
}

static void pchl (u8 insn)
{
  jump (reg_HL);
}

static void sphl (u8 insn)
{
  reg_SP = reg_HL;
}

static void hlt (u8 insn)
{
  halt (reg_PC - 1);
}

static void (*const dispatch[]) (u8 insn) = {
  nop,  lxi,  stax, inx,  inr,  dcr,  mvi,  rlc,
  nop,  dad,  ldax, dcx,  inr,  dcr,  mvi,  rrc,
  nop,  lxi,  stax, inx,  inr,  dcr,  mvi,  ral,
  nop,  dad,  ldax, dcx,  inr,  dcr,  mvi,  rar,
  nop,  lxi,  shld, inx,  inr,  dcr,  mvi,  daa,
  nop,  dad,  lhld, dcx,  inr,  dcr,  mvi,  cma,
  nop,  lxi,  sta,  inx,  inrm, dcrm, mvim, stc,
  nop,  dad,  lda,  dcx,  inr,  dcr,  mvi,  cmc,

  mov,  mov,  mov,  mov,  mov,  mov,  mov_m,mov,
  mov,  mov,  mov,  mov,  mov,  mov,  mov_m,mov,
  mov,  mov,  mov,  mov,  mov,  mov,  mov_m,mov,
  mov,  mov,  mov,  mov,  mov,  mov,  mov_m,mov,
  mov,  mov,  mov,  mov,  mov,  mov,  mov_m,mov,
  mov,  mov,  mov,  mov,  mov,  mov,  mov_m,mov,
  movm, movm, movm, movm, movm, movm, hlt,  movm,
  mov,  mov,  mov,  mov,  mov,  mov,  mov_m,mov,

  arit, arit, arit, arit, arit, arit, aritm,arit,
  arit, arit, arit, arit, arit, arit, aritm,arit,
  arit, arit, arit, arit, arit, arit, aritm,arit,
  arit, arit, arit, arit, arit, arit, aritm,arit,
  arit, arit, arit, arit, arit, arit, aritm,arit,
  arit, arit, arit, arit, arit, arit, aritm,arit,
  arit, arit, arit, arit, arit, arit, aritm,arit,
  cmp,  cmp,  cmp,  cmp,  cmp,  cmp,  cmpm,  cmp,

  r,    pop,  j,    jmp,  c,    push, imm,  rst,
  r,    ret,  j,    jmp,  c,    call, imm,  rst,
  r,    pop,  j,    out,  c,    push, imm,  rst,
  r,    ret,  j,    in,   c,    call, imm,  rst,
  r,    pop,  j,    xthl, c,    push, imm,  rst,
  r,    pchl, j,    xchg, c,    call, imm,  rst,
  r,    pop,  j,    di,   c,    push, imm,  rst,
  r,    sphl, j,    ei,   c,    call, cpi,  rst
};

static u8 states[] = {
  4, 10,  7,  5,  5,  5,  7,  4,  4, 10,  7,  5,  5,  5,  7,  4,
  4, 10,  7,  5,  5,  5,  7,  4,  4, 10,  7,  5,  5,  5,  7,  4,
  4, 10, 16,  5,  5,  5,  7,  4,  4, 10, 16,  5,  5,  5,  7,  4,
  4, 10, 13,  5, 10, 10, 10,  4,  4, 10, 13,  5,  5,  5,  7,  4,
  5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,
  5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,
  5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,
  7,  7,  7,  7,  7,  7,  7,  7,  5,  5,  5,  5,  5,  5,  7,  5,
  4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
  4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
  4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
  4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
  5, 10, 10, 10, 11, 11,  7, 11,  5, 10, 10, 10, 11, 17,  7, 11,
  5, 10, 10, 10, 11, 11,  7, 11,  5, 10, 10, 10, 11, 17,  7, 11, 
  5, 10, 10, 18, 11, 11,  7, 11,  5,  5, 10,  5, 11, 17,  7, 11,
  5, 10, 10,  4, 11, 11,  7, 11,  5,  5, 10,  4, 11, 17,  7, 11
};

u16 execute (void)
{
  u8 insn = opcode ();
  dispatch[insn] (insn);
  cycles += states[insn];
  return reg_PC;
}

void cpu_state (u16 *sp, u8 *regs)
{
  u8 reg_INTE = (opcode == ifetch);
  u8 reg_F = get_flags ();
  *sp = reg_SP;
  memcpy (regs, &reg_F, sizeof reg_F);
  memcpy (regs + 1, &reg_INTE, sizeof reg_INTE);
  memcpy (regs + 2, reg, sizeof reg);
}

void cpu_reset (void)
{
  cycles = 0;
  reg_PC = 0;
  reg_A = 0;
  result = 0;
  aux_carry = 0;
  sign_parity = 0;
  irq = -1;
  opcode = fetch;
}
