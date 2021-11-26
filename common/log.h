#ifdef DEBUG
#define LOG(COMPONENT, MESSAGE, ...) \
  logger (#COMPONENT, MESSAGE, ##__VA_ARGS__)
#else
#define LOG(COMPONENT, MESSAGE, ...) do {} while (0)
#endif

extern FILE *log_file;
extern void logger (const char *device, const char *format, ...);
extern unsigned long long get_cycles (void);

