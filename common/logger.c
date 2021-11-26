#include <SDL.h>
#include <stdarg.h>
#include "log.h"

FILE *log_file = NULL;
static SDL_SpinLock lock;

void logger (const char *device, const char *format, ...)
{
  va_list ap;
  if (log_file == NULL)
    return;
  SDL_AtomicLock (&lock);
  fprintf (log_file, "[LOG %12llu %-4s | ", get_cycles (), device);
  va_start (ap, format);
  vfprintf (log_file, format, ap);
  fprintf (log_file, "]\n");
  va_end (ap);
  SDL_AtomicUnlock (&lock);
}
