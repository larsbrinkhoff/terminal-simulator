#include <SDL.h>
#include <unistd.h>
#include "defs.h"
#include "pty.h"
#include "rs232.h"
#include "log.h"
#include "vcd.h"

static SDL_mutex *rx_lock;
static SDL_cond *rx_cond;
static uint8_t rx_data;
static SDL_bool rx_empty;

struct rs232 rx_rs232, tx_rs232;

int uart_tx_flag (void)
{
  return tx_rs232.encode_bits > 0;
}

void uart_tx_data (uint8_t data)
{
  rs232_encode (&tx_rs232, data);
  LOG (UART, "Send %02X %c", data, data);
}

int uart_rx_flag (void)
{
  int flag;
  SDL_LockMutex (rx_lock);
  flag = !rx_empty;
  SDL_UnlockMutex (rx_lock);
  return flag;
}

uint8_t uart_rx_data (void)
{
  uint8_t data;

  SDL_LockMutex (rx_lock);
  if (rx_empty) {
    SDL_UnlockMutex (rx_lock);
    return 0;
  }
  data = rx_data;
  rx_empty = SDL_TRUE;
  SDL_UnlockMutex (rx_lock);

  return data;
}

static void receive (struct rs232 *rs232)
{
  rx_data = rs232->data;
  rx_empty = SDL_FALSE;
}

static void transmit (struct rs232 *rs232)
{
  send_character (rs232->data);
}

static int receiver (void *arg)
{
  char c;

  (void)arg;

  for (;;) {
    c = receive_character ();
    LOG (UART, "Receive %02X %c", c, c);

    SDL_LockMutex (rx_lock);
    while (rx_rs232.encode_bits > 0)
      SDL_CondWait (rx_cond, rx_lock);
    rs232_encode (&rx_rs232, c);
    SDL_UnlockMutex (rx_lock);
  }

  return 0;
}

static int tick = 0;

void uart_clock (void)
{
  tick++;

  SDL_LockMutex (rx_lock);

  if (tick == 16) {
    tick = 0;
    if (rx_rs232.encode_bits == 0)
      SDL_CondSignal (rx_cond);
    rs232_encode_clock (&tx_rs232);
    rs232_encode_clock (&rx_rs232);
  }

  rs232_decode_clock (&rx_rs232);
  SDL_UnlockMutex (rx_lock);

  rs232_decode_clock (&tx_rs232);

#ifdef DEBUG_VCD
  vcd_value (cycles, index_rx, rx_rs232.bit);
  vcd_value (cycles, index_tx, tx_rs232.bit);
#endif
}

void reset_uart (void)
{
  rx_lock = SDL_CreateMutex ();
  rx_cond = SDL_CreateCond ();
  rx_empty = SDL_TRUE;
  rs232_reset (&rx_rs232, receive);
  rs232_reset (&tx_rs232, transmit);

  SDL_CreateThread (receiver, "VT52: RX", NULL);
}
