struct event {
  unsigned cycles;
  char *name;
  void (*callback) (void);
  struct event *next;
};

#define EVENT(NAME, CALLBACK) \
  struct event NAME = { 0, #NAME, (CALLBACK), NULL }

extern void add_event (unsigned cycles, struct event *event);
extern void print_events (FILE *);
extern void cancel_event (struct event *event);
extern void events (unsigned cycles);
