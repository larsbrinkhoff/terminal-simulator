#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include "vt100.h"

int flowers = -1; // Trevor Flowers' mini VT100.

void flowers_init(const char *device)
{
  struct termios t;

  flowers = open(device, O_RDWR);
  if (flowers == -1)
    panic("Couldn't open %s: %s", optarg, strerror (errno));

  memset(&t, 0, sizeof t);
  tcsetattr(flowers, TCSAFLUSH, &t);
}

void flowers_leds(u8 data)
{
  char command[100];
  int n;
  if (flowers < 0)
    return;
  n = snprintf(command, sizeof command, "\nSET_ALL_LEDS %c%c%c%c%c%c%c\n",
               (data & 0x20) ? '0' : '1',
               (data & 0x20) ? '1' : '0',
               (data & 0x10) ? '1' : '0',
               (data & 0x08) ? '1' : '0',
               (data & 0x04) ? '1' : '0',
               (data & 0x02) ? '1' : '0',
               (data & 0x01) ? '1' : '0');
  write(flowers, command, n);
}

static const char *const prefix = "SPECIAL_KEY: ";
static const char *pointer = prefix;

int flowers_key(void)
{
  char data;
  int n;

  if (ioctl(flowers, FIONREAD, &n) != 0)
    return -1;
  if (n < 1)
    return -1;
  if (read(flowers, &data, 1) != 1)
    return -1;

  if (*pointer == 0) {
    pointer = prefix;
    return data;
  }

  if (data != *pointer)
    pointer = prefix;
  if (data == *pointer)
    pointer++;

  return -1;
}
