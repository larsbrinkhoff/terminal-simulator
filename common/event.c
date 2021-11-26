#include <stdio.h>
#include "event.h"

static struct event head = { 0ULL, "(head)", NULL, NULL };

void cancel_event (struct event *event)
{
  struct event *node;
  for (node = &head; node->next != NULL; node = node->next) {
    if (node->next != event)
      continue;
    node->next = event->next;
    event->cycles = 0;
    return;
  }  
}

void add_event (unsigned cycles, struct event *event)
{
  struct event *node;

  if (event->cycles > 0)
    cancel_event (event);

  event->cycles = cycles;

  for (node = &head; node->next != NULL; node = node->next) {
    if (event->cycles > node->next->cycles) {
      event->cycles -= node->next->cycles;
      continue;
    }
    node->next->cycles -= event->cycles;
    event->next = node->next;
    node->next = event;
    return;
  }

  node->next = event;
  event->next = NULL;
}

void events (unsigned cycles)
{
  void (*callback) (void);
  struct event *node;

  for (node = &head; node->next != NULL;) {
    if (cycles < node->next->cycles) {
      node->next->cycles -= cycles;
      return;
    }

    callback = node->next->callback;
    cycles -= node->next->cycles;
    node->next->cycles = 0;
    node->next = node->next->next;
    callback ();
  }
}

void print_events (FILE *f)
{
  unsigned long long cycles = 0;
  struct event *node;
  for (node = head.next; node != NULL; node = node->next) {
    cycles += node->cycles;
    fprintf (f, "%20llu %s\r\n", cycles, node->name);
  }
}
