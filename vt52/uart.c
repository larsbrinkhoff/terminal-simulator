#include <SDL.h>
#include <unistd.h>
#include "defs.h"
#include "event.h"
#include "pty.h"
#include "log.h"

static SDL_mutex *rx_lock;
static SDL_cond *rx_cond;
static uint8_t rx_shift;
static uint8_t rx_data;
static SDL_bool rx_empty;

static uint8_t tx_shift;
static SDL_bool tx_empty;

int uart_tx_flag (void)
{
  return tx_empty == SDL_FALSE;
}

void uart_tx_data (uint8_t data)
{
  tx_shift = data;
  send_character (tx_shift);
  LOG (UART, "Send %02X %c", data, data);
}

int uart_rx_flag (void)
{
  int flag;
  SDL_LockMutex (rx_lock);
  flag = rx_empty == SDL_FALSE;
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
  data = rx_shift;
  rx_empty = SDL_TRUE;
  SDL_CondSignal (rx_cond);
  SDL_UnlockMutex (rx_lock);

  return data;
}

static int receiver (void *arg)
{
  char c;

  (void)arg;

  for (;;) {
    c = receive_character ();
    LOG (UART, "Receive %02X %c", c, c);

    SDL_LockMutex (rx_lock);
    while (!rx_empty)
      SDL_CondWait (rx_cond, rx_lock);
    rx_shift = c;
    rx_empty = SDL_FALSE;
    SDL_UnlockMutex (rx_lock);
  }

  return 0;
}

static void rx_check (void);
static EVENT (rx_event, rx_check);

static void rx_check (void)
{
  SDL_LockMutex (rx_lock);
  if (rx_empty) {
    add_event (1000, &rx_event);
    SDL_UnlockMutex (rx_lock);
    return;
  }
  rx_data = rx_shift;
  rx_empty = SDL_TRUE;
  SDL_CondSignal (rx_cond);
  SDL_UnlockMutex (rx_lock);

  add_event (13824000LL / 960, &rx_event);
}

void reset_uart (void)
{
  rx_lock = SDL_CreateMutex ();
  rx_cond = SDL_CreateCond ();
  rx_empty = SDL_TRUE;
  tx_empty = SDL_TRUE;

  SDL_CreateThread (receiver, "VT52: RX", NULL);
  rx_check ();
}
