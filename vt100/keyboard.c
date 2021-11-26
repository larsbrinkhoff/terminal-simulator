#include <string.h>
#include "vt100.h"
#include "event.h"
#include "log.h"
#include "xsdl.h"

static u8 scan;
static u8 tx_data;
static SDL_SpinLock kbd_lock;

static void scanning (void);
static EVENT (scan_event, scanning);

uint8_t keymap (SDL_Scancode key)
{
  switch (key) {
  case SDL_SCANCODE_DELETE: return 0x03; // DELETE
  case SDL_SCANCODE_P: return 0x05;
  case SDL_SCANCODE_O: return 0x06;
  case SDL_SCANCODE_Y: return 0x07;
  case SDL_SCANCODE_T: return 0x08;
  case SDL_SCANCODE_W: return 0x09;
  case SDL_SCANCODE_Q: return 0x0A;
  case SDL_SCANCODE_RIGHT: return 0x10;
  case SDL_SCANCODE_RIGHTBRACKET: return 0x14;
  case SDL_SCANCODE_LEFTBRACKET: return 0x15;
  case SDL_SCANCODE_I: return 0x16;
  case SDL_SCANCODE_U: return 0x17;
  case SDL_SCANCODE_R: return 0x18;
  case SDL_SCANCODE_E: return 0x19;
  case SDL_SCANCODE_1: return 0x1A;
  case SDL_SCANCODE_LEFT: return 0x20;
  case SDL_SCANCODE_DOWN: return 0x22;
  case SDL_SCANCODE_PAUSE: return 0x23; // BREAK
  case SDL_SCANCODE_GRAVE: return 0x24;
  case SDL_SCANCODE_MINUS: return 0x25;
  case SDL_SCANCODE_9: return 0x26;
  case SDL_SCANCODE_7: return 0x27;
  case SDL_SCANCODE_4: return 0x28;
  case SDL_SCANCODE_3: return 0x29;
  case SDL_SCANCODE_ESCAPE: return 0x2A;
  case SDL_SCANCODE_UP: return 0x30;
  case SDL_SCANCODE_F3: return 0x31; // PF3
  case SDL_SCANCODE_F1: return 0x32; // PF1
  case SDL_SCANCODE_BACKSPACE: return 0x33; // BACKSPACE
  case SDL_SCANCODE_EQUALS: return 0x34;
  case SDL_SCANCODE_0: return 0x35;
  case SDL_SCANCODE_8: return 0x36;
  case SDL_SCANCODE_6: return 0x37;
  case SDL_SCANCODE_5: return 0x38;
  case SDL_SCANCODE_2: return 0x39;
  case SDL_SCANCODE_TAB: return 0x3A;
  case SDL_SCANCODE_KP_7: return 0x40;
  case SDL_SCANCODE_F4: return 0x41; // PF4
  case SDL_SCANCODE_F2: return 0x42; // PF2
  case SDL_SCANCODE_KP_0: return 0x43;
  case SDL_SCANCODE_F7: return 0x44; // LINE-FEED
  case SDL_SCANCODE_BACKSLASH: return 0x45;
  case SDL_SCANCODE_L: return 0x46;
  case SDL_SCANCODE_K: return 0x47;
  case SDL_SCANCODE_G: return 0x48;
  case SDL_SCANCODE_F: return 0x49;
  case SDL_SCANCODE_A: return 0x4A;
  case SDL_SCANCODE_KP_8: return 0x50;
  case SDL_SCANCODE_KP_ENTER: return 0x51;
  case SDL_SCANCODE_KP_2: return 0x52;
  case SDL_SCANCODE_KP_1: return 0x53;
  case SDL_SCANCODE_APOSTROPHE: return 0x55;
  case SDL_SCANCODE_SEMICOLON: return 0x56;
  case SDL_SCANCODE_J: return 0x57;
  case SDL_SCANCODE_H: return 0x58;
  case SDL_SCANCODE_D: return 0x59;
  case SDL_SCANCODE_S: return 0x5A;
  case SDL_SCANCODE_KP_PERIOD: return 0x60; // KEYPAD PERIOD
  case SDL_SCANCODE_KP_COMMA: return 0x61; // KEYPAD COMMA
  case SDL_SCANCODE_KP_5: return 0x62;
  case SDL_SCANCODE_KP_4: return 0x63;
  case SDL_SCANCODE_RETURN: return 0x64;
  case SDL_SCANCODE_PERIOD: return 0x65;
  case SDL_SCANCODE_COMMA: return 0x66;
  case SDL_SCANCODE_N: return 0x67;
  case SDL_SCANCODE_B: return 0x68;
  case SDL_SCANCODE_X: return 0x69;
  case SDL_SCANCODE_SCROLLLOCK: return 0x6A; // NO-SCROLL
  case SDL_SCANCODE_KP_9: return 0x70;
  case SDL_SCANCODE_KP_3: return 0x71;
  case SDL_SCANCODE_KP_6: return 0x72;
  case SDL_SCANCODE_KP_MINUS: return 0x73; // KEYPAD MINUS
  case SDL_SCANCODE_SLASH: return 0x75;
  case SDL_SCANCODE_M: return 0x76;
  case SDL_SCANCODE_SPACE: return 0x77;
  case SDL_SCANCODE_V: return 0x78;
  case SDL_SCANCODE_C: return 0x79;
  case SDL_SCANCODE_Z: return 0x7A;
  case SDL_SCANCODE_F9: return 0x7B; // SETUP
  case SDL_SCANCODE_RCTRL:
  case SDL_SCANCODE_LCTRL: return 0x7C;
  case SDL_SCANCODE_RSHIFT:
  case SDL_SCANCODE_LSHIFT: return 0x7D;
  case SDL_SCANCODE_CAPSLOCK: return capslock;
  default: return 0;
  }
}

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
