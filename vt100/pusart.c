#include <time.h>
#include <unistd.h>
#include <termios.h>
#include "vt100.h"

// Intel 8251 USART.

#define XMIT 0x01

/* Status bits. */
#define TX_EMPTY    0x01
#define RX_RDY      0x02
#define TX_RDY      0x04
#define ERR_PARITY  0x08
#define ERR_ORUN    0x10
#define ERR_FRAME   0x20
#define SYNDET      0x40
#define DSR         0x80

/* Mode bits. */
#define DIVISOR   0x03
#define LENGTH    0x0C
#define PENABLE   0x10
#define EVENP     0x20
#define STOPBITS  0xC0

/* Command bits. */
#define TX_ENABLE  0x01
#define DTR        0x02
#define RX_ENABLE  0x04
#define SBRK       0x08
#define ERESET     0x10
#define RTS        0x20
#define IRESET     0x40
#define HUNT       0x80

static u8 rx_baud;
static u8 tx_baud;
static u8 tx_shift;
static u8 rx_data;
static u8 tx_data;
static u8 cmd;
static u8 mode;
static u8 status;
static void (*handle) (u8 data);
static void set_mode (u8 data);

static float rate[16] = {
  50, 75, 110, 134.5, 150, 200, 300, 600, 1200,
  1800, 2000, 2400, 3600, 4800, 9600, 19200
};
static int rx_cycles_per_character;
static int tx_cycles_per_character;

static SDL_bool data_terminal_ready = 0;
static SDL_mutex *dtr_lock = NULL;
static SDL_cond *dtr_cond;
static SDL_atomic_t rdy;

static int csize (tcflag_t flags)
{
  switch (flags & CSIZE) {
  case CS5: return 5;
  case CS6: return 6;
  case CS7: return 7;
  case CS8: return 8;
  }
  return -1;
}

static int get_baud (speed_t speed) {
  switch (speed) {
  case B0: return 0;
  case B50: return 50;
  case B75: return 75;
  case B110: return 110;
  case B134: return 134;
  case B150: return 150;
  case B200: return 200;
  case B300: return 300;
  case B600: return 600;
  case B1200: return 1200;
  case B1800: return 1800;
  case B2400: return 2400;
  case B4800: return 4800;
  case B9600: return 9600;
  case B19200: return 19200;
  case B38400: return 38400;
  case B57600: return 57600;
  case B115200: return 115200;
  case B230400: return 230400;
  }
  return -1;
}

static void terminal_settings (int fd)
{
  struct termios tios;
  if (tcgetattr (fd, &tios) == -1)
    return;
  LOG (PUSART, "tty input modes: "
       "%cIGNBRK %cIGNPAR %cPARMRK %cINPCK %cISTRIP %cIXON %cIXOFF",
       (tios.c_iflag & IGNBRK) ? '+' : '-',
       (tios.c_iflag & IGNPAR) ? '+' : '-',
       (tios.c_iflag & PARMRK) ? '+' : '-',
       (tios.c_iflag & INPCK)  ? '+' : '-',
       (tios.c_iflag & ISTRIP) ? '+' : '-',
       (tios.c_iflag & IXON)   ? '+' : '-',
       (tios.c_iflag & IXOFF)  ? '+' : '-');
  LOG (PUSART, "tty control modes: "
       "CS%d %cSTOPB %cPARENB %cPARODD %cCLOCAL %cCRTSCTS",
       csize (tios.c_cflag),
       (tios.c_cflag & CSTOPB) ? '+' : '-',
       (tios.c_cflag & PARENB) ? '+' : '-',
       (tios.c_cflag & PARODD) ? '+' : '-',
       (tios.c_cflag & CLOCAL) ? '+' : '-',
       (tios.c_cflag & CRTSCTS)? '+' : '-');
  LOG (PUSART, "tty baud rates: %d %d",
       get_baud (cfgetispeed (&tios)), get_baud (cfgetospeed (&tios)));
}

static void (*decode_bit) (int bit, int cycles);
static int current_cycles;
static int bits;
static u8 character;
static int data_cycles;
static int stop_cycles;
static int char_size;
static void decode_edge (int bit, int cycles);
static void decode_start (int bit, int cycles);
static void decode_data (int bit, int cycles);
static void decode_stop (int bit, int cycles);

static void decode_edge (int bit, int cycles)
{
  if (bit == 1)
    // Mark state is idle.
    return;

  // Next sample point relative to edge.
  current_cycles = -data_cycles / 2;
  decode_bit = decode_start;
  decode_bit (bit, cycles);
}

static void decode_start (int bit, int cycles)
{
  current_cycles += cycles;
  if (current_cycles < 0)
    return;
  if (bit == 1) {
    status |= ERR_FRAME;
    decode_bit = decode_edge;
    decode_bit (bit, 0);
  }
  character = 0;
  bits = 0;
  current_cycles -= data_cycles;
  decode_bit = decode_data;
  decode_bit (bit, 0);
}

static void decode_data (int bit, int cycles)
{
  current_cycles += cycles;
  if (current_cycles < 0)
    return;
  character |= bit << bits;
  bits++;
  if (bits == char_size) {
    decode_bit = decode_stop;
    current_cycles -= stop_cycles;
  } else
    current_cycles -= data_cycles;
  decode_bit (bit, 0);
}

static void decode_stop (int bit, int cycles)
{
  current_cycles += cycles;
  if (current_cycles < 0)
    return;
  if (bit == 0)
    status |= ERR_FRAME;
  else
    pusart_rx (character);
  current_cycles -= data_cycles;
  decode_bit = decode_edge;
  decode_bit (bit, 0);
}

static void encode_character (u8 data)
{
  int i;
  decode_bit (0, data_cycles);
  for (i = 0; i < char_size; i++, data >>= 1)
    decode_bit (data & 1, data_cycles);
  decode_bit (1, stop_cycles);
}

static int receiver (void *arg)
{
  char c;
  static struct timespec slp;

  decode_bit = decode_edge;
  current_cycles = 0;

  dtr_lock = SDL_CreateMutex ();
  dtr_cond = SDL_CreateCond ();

  if (baud)
    slp.tv_nsec = 1000*1000*1000 / (baud/11);

  terminal_settings (pty);

  for (;;) {
    SDL_LockMutex (dtr_lock);
    while (!data_terminal_ready)
      SDL_CondWait (dtr_cond, dtr_lock);
    SDL_UnlockMutex (dtr_lock);

    data_cycles = 288;
    stop_cycles = 288;
    char_size = 8;

    if (read (pty, &c, 1) < 0){
      if (rerun){
        sleep (2);
        spawn ();
      }else{
        exit (0);
      }
    }
    /* TEMPORARY HACK to avoid excessive overruns. */
    while (SDL_AtomicGet (&rdy))
      ;
    //pusart_rx (c);
    encode_character (c);

    if (baud)
      nanosleep (&slp, NULL);
  }
}

void pusart_rx (u8 data)
{
  if ((cmd & RX_ENABLE) == 0) {
    LOG (UART, "RX character %02X (discarded)", data); 
    return;
  }

  LOG (UART, "RX character %02X", data); 

  if (status & RX_RDY) {
    LOG (UART, "RX overrun"); 
    status |= ERR_ORUN;
  }

  rx_data = data;
  status |= RX_RDY;
  SDL_AtomicSet (&rdy, 1);
  raise_interrupt (2);
}

static u8 pusart_in_data (u8 port)
{
  LOG (UART, "IN rx data %02X", rx_data); 
  clear_interrupt (2);
  status &= ~RX_RDY;
  SDL_AtomicSet (&rdy, 0);
  return rx_data;
}

static void tx_empty (void);
static EVENT (tx_event, tx_empty);

static void tx_start (void)
{
  tx_shift = tx_data;
  status &= ~TX_EMPTY;
  status |= TX_RDY;
  vt100_flags |= XMIT;
  add_event (tx_cycles_per_character, &tx_event);
}

static void tx_empty (void)
{
  sendchar (tx_shift);
  if (status & TX_RDY)
    status |= TX_EMPTY;
  else
    tx_start ();
}

void set_dtr (SDL_bool dtr)
{
	if (dtr_lock == NULL)
		return;
	SDL_LockMutex (dtr_lock);
	data_terminal_ready = dtr;
	if (dtr)
	  SDL_CondSignal(dtr_cond);
	SDL_UnlockMutex (dtr_lock);
}

static void command (u8 data)
{
  cmd = data;
  if (data & TX_ENABLE) {
    LOG (UART, "tx enable", data);
  }
  if (data & DTR) {
    LOG (UART, "dtr", data);
  }
  set_dtr (data & DTR);
  if (data & RX_ENABLE) {
    LOG (UART, "rx enable", data);
  }
  if (data & SBRK) {
    LOG (UART, "break", data);
    tcsendbreak (pty, 0);
  }
  if (data & ERESET) {
    LOG (UART, "clear errors", data);
    status &= ~(ERR_ORUN | ERR_FRAME | ERR_PARITY);
  }
  if (data & RTS) {
    LOG (UART, "rts", data);
  }
  if (data & HUNT) {
    LOG (UART, "hunt", data);
  }
  if (data & IRESET) {
    LOG (UART, "reset", data);
    handle = set_mode;
  }
}

static int compute_rate (float rate)
{
  float cycles_per_bit = 44236800.0 / rate;
  float bits_per_character = 5 + ((mode & LENGTH) >> 2);
  switch (mode & DIVISOR) {
  case 0x00: break;
  case 0x01: cycles_per_bit /= 1.0; break;
  case 0x02: cycles_per_bit /= 16.0; break;
  case 0x03: cycles_per_bit /= 64.0; break;
  }
  switch (mode & STOPBITS) { 
  case 0x00: break;
  case 0x40: bits_per_character += 1; break;
  case 0x80: bits_per_character += 1.5; break;
  case 0xC0: bits_per_character += 2; break;
  } 
  return (int)(cycles_per_bit * bits_per_character + .4);
}

static void set_mode (u8 data)
{
  LOG (UART, "set mode %02X", data); 
  mode = data;
  handle = command;
  rx_cycles_per_character = compute_rate (rate[rx_baud]);
  tx_cycles_per_character = compute_rate (rate[tx_baud]);
}

static void pusart_out_data (u8 port, u8 data)
{
  if ((cmd & TX_ENABLE) == 0) {
    LOG (UART, "OUT tx data %02X (discarded)", data); 
    return;
  }

  LOG (UART, "OUT tx data %02X", data); 

  tx_data = data;
  status &= ~TX_RDY;
  vt100_flags &= ~XMIT;

  if (status & TX_EMPTY)
    tx_start ();
}

static u8 pusart_in_status (u8 port)
{
  LOG (UART, "IN status %02X", status); 
  return status;
}

static void pusart_out_command (u8 port, u8 data)
{
  handle (data);
}

static u8 no_in (u8 port)
{
  // This port is write only.
  return 0;
}

static void no_out (u8 port, u8 data)
{
  // This port is read only.
}

static void baud_out (u8 port, u8 data)
{
  if (data != ((tx_baud << 4) | rx_baud))
    LOG (BAUD, "RX %d, TX %d", rate[data & 0x0F], rate[data >> 4]);
  rx_baud = data & 0x0F;
  tx_baud = data >> 4;
  rx_cycles_per_character = compute_rate (rate[rx_baud]);
  tx_cycles_per_character = compute_rate (rate[tx_baud]);
}

static u8 modem_in (u8 port)
{
  u8 data = 0x0F;
  data |= 0x10; //carrier detect
  //data |= 0x20; //ring indicator
  //data |= 0x40; //speed indicator
  data |= 0x80; //clear to send
  LOG (MODEM, "IN %02X", data);
  return data;
}

void reset_pusart (void)
{
  SDL_AtomicSet (&rdy, 0);
  SDL_CreateThread (receiver, "PUSART receiver", NULL);
  register_port (0x00, pusart_in_data, pusart_out_data);
  register_port (0x01, pusart_in_status, pusart_out_command);
  register_port (0x02, no_in, baud_out);
  register_port (0x22, modem_in, no_out);
  rx_baud = 0;
  tx_baud = 0;
  vt100_flags |= XMIT;
  status = TX_RDY | TX_EMPTY | DSR;
  cmd = 0;
  set_mode (0x8E);
  handle = set_mode;
}
