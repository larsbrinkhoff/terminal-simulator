#include "vt100.h"

int columns;
int reverse_field;
int underline;
static int hertz;
static int interlace;
static u8 line_buffer[137];
int line_scroll;
static u8 line_attr;
extern SDL_SpinLock lock_update;

static void refresh (void);
static EVENT (refresh_event, refresh);

static u8 video_a2_in (u8 port)
{
  LOG (VID, "A2 IN"); 
  return 0;
}

static void set_scroll (u8 x)
{
  SDL_AtomicLock(&lock_update);
  line_scroll = x;
  updatebuf = 1;
  SDL_AtomicUnlock(&lock_update);
}

static void video_a2_out (u8 port, u8 data)
{
  switch (data & 0x0F) {
  case 0x00:
    set_scroll ((line_scroll & 0x0C) | 0x00);
    LOG (VID, "Load low order scroll latch 00");
    break;
  case 0x01:
    set_scroll ((line_scroll & 0x0C) | 0x01);
    LOG (VID, "Load low order scroll latch 01");
    break;
  case 0x02:
    set_scroll ((line_scroll & 0x0C) | 0x02);
    LOG (VID, "Load low order scroll latch 10");
    break;
  case 0x03:
    set_scroll ((line_scroll & 0x0C) | 0x03);
    LOG (VID, "Load low order scroll latch 11");
    break;
  case 0x04:
    set_scroll ((line_scroll & 0x03) | 0x00);
    LOG (VID, "Load high order scroll latch 00");
    break;
  case 0x05:
    set_scroll ((line_scroll & 0x03) | 0x04);
    LOG (VID, "Load high order scroll latch 01");
    break;
  case 0x06:
    set_scroll ((line_scroll & 0x03) | 0x08);
    LOG (VID, "Load high order scroll latch 10");
    break;
  case 0x07:
    set_scroll ((line_scroll & 0x03) | 0x0C);
    LOG (VID, "Load high order scroll latch 11");
    break;
  case 0x08: LOG (VID, "Toggle blink flip-flop"); break;
  case 0x09: clear_interrupt (4); break;
  case 0x0A: reverse_field = 1; LOG (VID, "Set reverse field on"); break;
  case 0x0B: reverse_field = 0; LOG (VID, "Set reverse field off"); break;
  case 0x0C: underline = 1; LOG (VID, "Set basic attribute to underline"); break;
  case 0x0D: underline = 0; LOG (VID, "Set basic attribute to reverse video"); break;
  case 0x0E:
  case 0x0F: LOG (VID, "Reserved"); break;
  }
}

static u8 video_c2_in (u8 port)
{
  LOG (VID, "C2 IN"); 
  return 0;
}

static void video_c2_out (u8 port, u8 data)
{
  switch (data & 0x30) {
  case 0x00: columns = 80; interlace = 1; break;
  case 0x10: columns = 132; interlace = 1; break;
  case 0x20: hertz = 60; interlace = 0; break;
  case 0x30: hertz = 50; interlace = 0; break;
  }
  updatebuf = 1;
}

static u16 video_line (int n, u16 addr)
{
  u8 *p = line_buffer;
  u8 data;
  int skip = hertz == 60 ? 2 : 5;
  int i;

  for (;;) {
    if (addr < 0x2000 || addr >= 0x2C00) {
      LOG (VID, "Address outside RAM: %04X", addr);
      return 0x2000;
    }
    if (p - line_buffer > columns)
      return 0x2000;

    data = memory[addr++];
    switch (data) {
    case 0x7F:
      if (n >= skip)
        for (i = p - line_buffer; i < columns; i++)
          *p++ = 0;
      if (n >= skip)
        draw_line (line_scroll, line_attr & 0xE0, n - skip, line_buffer);
      line_attr = memory[addr];
      switch (line_attr & 0x60) {
      case 0x00: /*double height, bottom line*/ break;
      case 0x20: /*double height, top line*/ break;
      case 0x40: /*double width line*/ break;
      case 0x60: /*normal*/ break;
      }
      addr = ((line_attr & 0x0F) << 8) | memory[addr + 1];
      return (line_attr & 0x10 ? 0x2000 : 0x4000) | addr;
      break;
    default:
      *p++ = data;
      break;
    }
  }
}

static void refresh (void)
{
  int i;
  u16 addr;

  raise_interrupt (4);

  addr = 0x2000;
  for (i = 0; i < 27; i++)
    addr = video_line (i, addr);
  
  checkupdate ();
  add_event (2764800/hertz, &refresh_event);
}

void reset_video (void)
{
  register_port (0xA2, video_a2_in, video_a2_out);
  register_port (0xC2, video_c2_in, video_c2_out);

  columns = 80;
  hertz = 60;
  interlace = 0;
  refresh ();
}

int video (void *arg)
{
  register_port (0xA2, video_a2_in, video_a2_out);
  register_port (0xC2, video_c2_in, video_c2_out);
  reverse_field = 0;
  underline = 0;
  columns = 80;
  hertz = 60;
  interlace = 0;
  line_scroll = 0;
  line_attr = 0x60;

  SDL_Delay (1000);
  for (;;) {
    SDL_Delay (1000/hertz);
    refresh ();
  }

  return 0;
}
