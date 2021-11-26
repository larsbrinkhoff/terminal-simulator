extern void mkpty (char **cmd, int th, int tw, int fw, int fh);
extern void send_break (void);
extern void send_character (uint8_t data);
extern uint8_t receive_character (void);
extern void reset_pty (char **cmd, int th, int tw, int fw, int fh);
