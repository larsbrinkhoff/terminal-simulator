#include <SDL.h>
#include <string.h>
#include "vt100.h"

static u8 scan;
static u8 tx_data;
static SDL_SpinLock kbd_lock;

static void scanning (void);
static EVENT (scan_event, scanning);

static u8 keyboard_in (u8 port)
{
  //LOG (KEY, "IN %02X", tx_data); 
  clear_interrupt (1);
  return tx_data;
}

static void kbd_rdy (void)
{
  vt100_flags |= 0x80;
}

static EVENT (kbd_event, kbd_rdy);
static u8 previous = 0;

static void keyboard_out (u8 port, u8 data)
{
  u8 changed = previous ^ data;
  if ((vt100_flags & 0x80) == 0)
    return;
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
  add_event (3520, &kbd_event);
}

static int down[128];

void key_down (u8 code)
{
  LOG (KEY, "Down %02X", code);
  SDL_AtomicLock (&kbd_lock);
  down[code] = 1;
  SDL_AtomicUnlock (&kbd_lock);
}

void key_up (u8 code)
{
  LOG (KEY, "Up %02X", code);
  SDL_AtomicLock (&kbd_lock);
  down[code] = 0;
  SDL_AtomicUnlock (&kbd_lock);
}

static void scanning (void)
{
  SDL_AtomicLock (&kbd_lock);
  for (; scan < 0x80; scan++) {
    if (down[scan]) {
      //LOG (KEY, "Scanned %02X", scan);
      tx_data = scan;
      raise_interrupt (1);
      add_event (1000, &scan_event);
      scan++;
      break;
    }
  }
  SDL_AtomicUnlock (&kbd_lock);
}

void reset_keyboard (void)
{
  register_port (0x82, keyboard_in, keyboard_out);
  vt100_flags |= 0x80;
  memset (down, 0, sizeof down);
  down[0x7F] = 1;
  scan = 0x80;
}
