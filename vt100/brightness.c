#include "vt100.h"

u8 brightness;

static u8 flags_in (u8 port)
{
  // 0 xmit flag H
  // 1 advance video L
  // 2 graphics L
  // 3 option present H
  // 4 even field L
  // 5 nvr data H
  // 6 LBA7 H
  // 7 keyboard H
  u8 old = vt100_flags & 0x40;
  vt100_flags &= ~0x40;
  if ((get_cycles () / 200) & 1)
    vt100_flags |= 0x40;
  if (!old && (vt100_flags & 0x40))
    nvr_clock ();
  //LOG (FLAGS, "IN %02X", vt100_flags); 
  return vt100_flags;
}

static void brightness_out (u8 port, u8 data)
{
  brightness = data;
  //LOG (BRIGHT, "OUT %02X", data);
}

void reset_brightness (void)
{
  vt100_flags = 0x06; //No AVO or graphics option.
  register_port (0x42, flags_in, brightness_out);
}
