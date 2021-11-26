#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "vt100.h"
#include "log.h"

//ER1400 device.

static u16 word[100];
static u8 nvr_latch;
static u32 nvr_addr;
static u16 nvr_data;
static FILE *f;
static char *filename = NULL;
#define NVRAM "vt100-nvram"

static void setfilename(void) {
	char *statehome, *home;

	if (filename != NULL)
		return;

	statehome = getenv("XDG_STATE_HOME");
	if (statehome != NULL) {
		asprintf(&filename, "%s/%s", statehome, NVRAM);
		return;
	}

	home = getenv("HOME");
	if (home != NULL) {
		asprintf(&statehome, "%s/.local", home);
		mkdir(statehome, 0777);
		free(statehome);
		asprintf(&statehome, "%s/.local/state", home);
		mkdir(statehome, 0777);
		free(statehome);
		asprintf(&filename, "%s/.local/state/%s", home, NVRAM);
		return;
	}

	filename = NVRAM;
}

static u8 nvr_in (u8 port)
{
  LOG (NVR, "IN"); 
  return 0;
}

static void nvr_out (u8 port, u8 data)
{
  //LOG (NVR, "OUT %02X", data); 
  nvr_latch = data;
}

static void save (int addr)
{
  setfilename();
  if (f == NULL)
    f = fopen (filename, "wb");
  if (f == NULL)
    return;
  fseek (f, 2 * addr, SEEK_SET);
  fputc (word[addr] & 0xFF, f);
  fputc (word[addr] >> 8, f);
  fflush (f);
}

static u32 word_addr (void)
{
  int i, ones = 0, tens = 0;
  u32 addr = ~nvr_addr;
  for (i = 0; i < 10; i++, addr >>= 1) {
    if (addr & 1)
      tens = 9 - i;
  }
  for (i = 0; i < 10; i++, addr >>= 1) {
    if (addr & 1)
      ones = 9 - i;
  }
  return 10 * tens + ones;
}

void nvr_clock (void)
{
  u32 addr;
  u8 bit = nvr_latch & 1;
  switch ((nvr_latch >> 1) & 7) {
  case 0: //Accept Data.
    nvr_data <<= 1;
    nvr_data |= bit;
    nvr_data &= 0x3FFF;
    //LOG (NVR, "data shift in (%04X)", nvr_data);
    break;
  case 1: //Accept Address.
    nvr_addr <<= 1;
    nvr_addr |= bit;
    nvr_addr &= 0xFFFFF;
    //LOG (NVR, "address shift in (%05X)", nvr_addr);
    break;
  case 2: //Shift Data Out.
    //LOG (NVR, "data shift out (%04X", nvr_data);
    nvr_data <<= 1;
    vt100_flags &= ~0x20;
    if (nvr_data & 0x4000)
      vt100_flags |= 0x20;
    nvr_data &= 0x3FFF;
    break;
  case 3:
    break;
  case 4: //Write.
    nvr_data &= 0x3FFF;
    addr = word_addr ();
    if (addr < 100) {
      word[addr] = nvr_data;
      save (addr);
      LOG (NVR, "write %02d, %04X", addr, nvr_data);
    }
    break;
  case 5: //Erase.
    addr = word_addr ();
    if (addr < 100) {
      word[addr] = 0X3FFF;
      save (addr);
      LOG (NVR, "erase %02d", addr);
    }
    break;
  case 6: //Read.
    addr = word_addr ();
    if (addr < 100) {
      nvr_data = word[addr];
      LOG (NVR, "read %02d, %04X", addr, nvr_data);
    }
    break;
  case 7: //Standby.
    //LOG (NVR, "stand by");
    break;
  }
}

static u16 defaults[100] = {
#if 1
  //jeffpar
  0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80,
  0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80,
  0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80,
  0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80,
  0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E00,
  0x2E08, 0x2E8E, 0x2E00, 0x2ED0, 0x2E70, 0x2E00, 0x2E20, 0x2E00,
  0x2EE0, 0x2EE0, 0x2E7D, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000
#else
  0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80,
  0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80,
  0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80,
  0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80,
  0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E80, 0x2E00,
  0x2E08, 0x2E8E, 0x2E20, 0x2ED0, 0x2E50, 0x2E00, 0x2E20, 0x2E00,
  0x2EE0, 0x2EE0, 0x2E69, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000
#endif
};

static void load (void)
{
  u16 data;
  int i;
  fseek (f, 0, SEEK_SET);
  for (i = 0; i < 100; i++) {
    data = fgetc (f);
    data |= fgetc (f) << 8;
    if (feof (f) || ferror (f))
      word[i] = 0x3FFF;
    else
      word[i] = data & 0x3FFF;
  }
  // Make sure it's all committed to disk.
  for (i = 0; i < 100; i++)
    save (i);
}

void reset_nvr (void)
{
  register_port (0x62, nvr_in, nvr_out);
  nvr_data = 0;
  nvr_addr = 0;
  setfilename();
  f = fopen (filename, "r+b");
  if (f != NULL)
    load ();
  else
    memcpy (word, defaults, sizeof defaults);
}
