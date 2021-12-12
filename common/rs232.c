#include "rs232.h"

static void encode_idle (struct rs232 *rs232);
static void encode_start (struct rs232 *rs232);
static void encode_data (struct rs232 *rs232);
static void encode_stop (struct rs232 *rs232);
static void decode_edge (struct rs232 *rs232);
static void decode_start (struct rs232 *rs232);
static void decode_data (struct rs232 *rs232);
static void decode_stop (struct rs232 *rs232);

void rs232_encode (struct rs232 *rs232, unsigned data)
{
  rs232->encode_shift = data;
  rs232->encode_bits = 7;
  rs232->encode = encode_start;
}

static void encode_idle (struct rs232 *rs232)
{
  rs232->bit = 1;
}

static void encode_start (struct rs232 *rs232)
{
  rs232->bit = 0;
  rs232->encode = encode_data;
}

static void encode_data (struct rs232 *rs232)
{
  rs232->bit = rs232->encode_shift & 1;
  rs232->encode_shift >>= 1;
  rs232->encode_bits--;

  if (rs232->encode_bits == 0)
    rs232->encode = encode_stop;
}

static void encode_stop (struct rs232 *rs232)
{
  rs232->bit = 1;
  rs232->encode = encode_idle;
}

static void decode_edge (struct rs232 *rs232)
{
  if (rs232->bit == 0) {
    rs232->decode = decode_start;
    rs232->tick = 7;
  }
}

static void decode_start (struct rs232 *rs232)
{
  rs232->decode = decode_data;
  rs232->tick = 15;
  rs232->decode_bits = 0;
  rs232->decode_shift = 0;
}

static void decode_data (struct rs232 *rs232)
{
  rs232->decode_shift |= rs232->bit << rs232->decode_bits;
  rs232->decode_bits++;
  rs232->tick = 15;

  if (rs232->decode_bits == 7) {
    rs232->empty = 0;
    rs232->data = rs232->decode_shift;
    rs232->decode = decode_stop;
    rs232->tick = 15;
    rs232->handle (rs232);
  }
}

static void decode_stop (struct rs232 *rs232)
{
  rs232->decode = decode_edge;
}

void rs232_decode_clock (struct rs232 *rs232)
{
  if (rs232->tick == 0)
    rs232->decode (rs232);
  else
    rs232->tick--;

}

void rs232_encode_clock (struct rs232 *rs232)
{
  rs232->encode (rs232);
}

void rs232_reset (struct rs232 *rs232, void (*handle) (struct rs232 *))
{
  rs232->bit = 1;
  rs232->tick = 0;
  rs232->decode = decode_edge;
  rs232->encode = encode_idle;
  rs232->handle = handle;
}
