#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "vt100.h"

int pty;
char *name;

static void shell (char **cmd)
{
  setenv ("TERM", "vt100", 1);
  execvp (cmd[0], cmd);
  exit (1);
}

static void spawn (char **cmd)
{
  switch (fork ()) {
  case -1:
    exit (1);

  case 0:
    close (pty);
    close (0);
    close (1);
    close (2);

    setsid ();

    if (open (name, O_RDWR) != 0)
      exit (1);
    dup (0);
    dup (1);
    shell (cmd);
  }
}

void mkpty (char **cmd, int th, int tw, int fw, int fh)
{
  struct winsize ws;

  pty = posix_openpt (O_RDWR);
  if (pty < 0 ||
      grantpt (pty) < 0 ||
      unlockpt (pty) < 0) {
    logger ("PTY", "Couldn't allocate pty");
    exit (1);
  }

  name = ptsname (pty);

  ws.ws_row = th;
  ws.ws_col = tw;
  ws.ws_xpixel = fw;
  ws.ws_ypixel = fh;
  ioctl (pty, TIOCSWINSZ, &ws);

  spawn (cmd);
}
