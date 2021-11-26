#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE
#define _DARWIN_C_SOURCE
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <stdio.h>
#include "event.h"
#include "log.h"
#include <SDL.h>

int pty;
static struct termios saved, old;
static int xonxoff;
static SDL_bool throttle;
static SDL_mutex *lock;
static SDL_cond *cond;

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

static void restore_termios (void)
{
  tcsetattr (pty, TCSAFLUSH, &saved);
}

static void terminal_settings (int fd)
{
  static struct termios new;
  if (tcgetattr (fd, &new) == -1)
    return;
  if (memcmp (&old, &new, sizeof saved) == 0)
    return;
  memcpy (&old, &new, sizeof saved);
  LOG (UART, "tty input modes: "
       "%cIGNBRK %cIGNPAR %cPARMRK %cINPCK %cISTRIP %cIXON %cIXOFF",
       (new.c_iflag & IGNBRK) ? '+' : '-',
       (new.c_iflag & IGNPAR) ? '+' : '-',
       (new.c_iflag & PARMRK) ? '+' : '-',
       (new.c_iflag & INPCK)  ? '+' : '-',
       (new.c_iflag & ISTRIP) ? '+' : '-',
       (new.c_iflag & IXON)   ? '+' : '-',
       (new.c_iflag & IXOFF)  ? '+' : '-');
  LOG (UART, "tty control modes: "
       "CS%d %cSTOPB %cPARENB %cPARODD %cCLOCAL %cCRTSCTS",
       csize (new.c_cflag),
       (new.c_cflag & CSTOPB) ? '+' : '-',
       (new.c_cflag & PARENB) ? '+' : '-',
       (new.c_cflag & PARODD) ? '+' : '-',
       (new.c_cflag & CLOCAL) ? '+' : '-',
       (new.c_cflag & CRTSCTS)? '+' : '-');
  LOG (UART, "tty baud rates: %d %d",
       get_baud (cfgetispeed (&new)),
       get_baud (cfgetospeed (&new)));
  xonxoff =
    (new.c_iflag & IXON) != 0 &&
    new.c_cc[VSTART] == 0x11 &&
    new.c_cc[VSTOP] == 0x13;
  if (!xonxoff) {
    SDL_LockMutex (lock);
    throttle = SDL_FALSE;
    SDL_CondSignal (cond);
    SDL_UnlockMutex (lock);
  }
}

static void check_settings (void);
static EVENT (pty_event, check_settings);

static void check_settings (void)
{
  terminal_settings (pty);
  add_event (2764800/3, &pty_event);
}

static void raw (void)
{
  struct termios new;
  atexit (restore_termios);
  new = saved;
  new.c_iflag = 0;
  new.c_oflag = 0;
  new.c_lflag = 0;
  new.c_cc[VMIN] = 1;
  new.c_cc[VTIME] = 0;
  tcsetattr (pty, TCSAFLUSH, &new);
}

static void shell (char **cmd)
{
  execvp (cmd[0], cmd);
  LOG (PTY, "Error executing command %s", cmd[0]);
  exit (1);
}

static void spawn (char **cmd)
{
  switch (fork ()) {
  case -1:
    LOG (PTY, "Error forking a child.");
    exit (1);

  case 0:
    close (0);
    close (1);
    close (2);

    setsid ();

    if (open (ptsname (pty), O_RDWR) != 0) {
      LOG (PTY, "Error opening pty %s", ptsname (pty));
      exit (1);
    }
    close (pty);
    dup (0);
    dup (1);
#ifdef __FreeBSD__
    ioctl (0, TIOCSCTTY);
#endif
    shell (cmd);
  }
}

static int mktty (char *device)
{
  struct stat st;
  if (stat (device, &st) != 0)
    return -1;
  if ((st.st_mode & S_IFMT) != S_IFCHR)
    return -1;
  return open (device, O_RDWR);
}

void reset_pty (char **cmd, int th, int tw, int fw, int fh)
{
  struct winsize ws;

  ws.ws_row = th;
  ws.ws_col = tw;
  ws.ws_xpixel = fw;
  ws.ws_ypixel = fh;

  pty = -1;
  if (cmd[1] == NULL)
    pty = mktty (cmd[0]);
  if (pty != -1) {
    terminal_settings (pty);
    raw ();
    ioctl (pty, TIOCSWINSZ, &ws);
    return;
  }

  pty = posix_openpt (O_RDWR);
  if (pty < 0 ||
      grantpt (pty) < 0 ||
      unlockpt (pty) < 0) {
    LOG (PTY, "Couldn't allocate pty");
    exit (1);
  }

  lock = SDL_CreateMutex ();
  cond = SDL_CreateCond ();
  throttle = SDL_FALSE;

  tcgetattr (pty, &saved);
  memset (&old, 0, sizeof old);
  check_settings ();

  ioctl (pty, TIOCSWINSZ, &ws);
  spawn (cmd);
}

void send_character (uint8_t data)
{
  if (!xonxoff)
    goto send;
  switch (data) {
  case 0x11:
    SDL_LockMutex (lock);
    throttle = SDL_FALSE;
    SDL_CondSignal (cond);
    SDL_UnlockMutex (lock);
    break;
  case 0x13:
    SDL_LockMutex (lock);
    throttle = SDL_TRUE;
    SDL_UnlockMutex (lock);
    break;
  default:
  send:
    write (pty, &data, 1);
    LOG (PTY, "Transmitted %02X", data);
    break;
  }
}

uint8_t receive_character (void)
{
  uint8_t data;
  SDL_LockMutex (lock);
  while (throttle)
    SDL_CondWait (cond, lock);
  SDL_UnlockMutex (lock);
  if (read (pty, &data, 1) <= 0) {
    LOG (PTY, "End of file or error from pty.");
    exit (0);
  }
  return data;
}

void send_break (void)
{
  tcsendbreak (pty, 0);
}
