#include "vt100.h"

static u8 scan;
static u8 tx_data;

static void scanning (void);
static EVENT (scan_event, scanning);

static u8 keyboard_in (u8 port)
{
  //LOG (KEY, "IN %02X", tx_data); 
  clear_interrupt (1);
  return tx_data;
}

static void tx_rdy (void)
{
  vt100_flags |= 0x80;
}

static EVENT (tx_event, tx_rdy);
static u8 previous = 0;

static void keyboard_out (u8 port, u8 data)
{
  u8 changed = previous ^ data;
  if (changed & ~0x40)
    LOG (KEY, "LED:%c%c%c%c %s %s%s",
            (data & 0x08) ? '1' : '-',
            (data & 0x04) ? '2' : '-',
            (data & 0x02) ? '3' : '-',
            (data & 0x01) ? '4' : '-',
            (data & 0x10) ? "Locked" : "------",
            (data & 0x20) ? "Local" : "-----",
            //(data & 0x40) ? " Start scan" : "",
            (data & 0x80) ? " Bell" : "");
  previous = data;
  if ((data & 0x40) && scan == 0x80) {
    scan = 0;
    scanning ();
  }
  if (data & 0x80)
    bell ();

  vt100_flags &= ~0x80;
  //Clock from LBA4, which is main clock divided by 22.
  //Each bit is 16 cycles, and a character is 10 bits.
  add_event (3520, &tx_event);
}

static int down[128];

void key_down (u8 code)
{
  LOG (KEY, "Down %02X", code);
  down[code] = 1;
}

void key_up (u8 code)
{
  LOG (KEY, "Up %02X", code);
  down[code] = 0;
}

static void scanning (void)
{
  for (; scan < 0x80; scan++) {
    if (down[scan]) {
      //LOG (KEY, "Scanned %02X", scan);
      tx_data = scan;
      raise_interrupt (1);
      add_event (1000, &scan_event);
      scan++;
      return;
    }
  }
}

void reset_keyboard (void)
{
  register_port (0x82, keyboard_in, keyboard_out);
  vt100_flags |= 0x80;
  memset (down, 0, sizeof down);
  down[0x7F] = 1;
  scan = 0x80;
}
