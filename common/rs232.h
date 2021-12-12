struct rs232 {
  unsigned bit;

  int encode_bits;
  unsigned encode_shift;
  void (*encode) (struct rs232 *rs232);

  int tick;
  int empty;
  int decode_bits;
  unsigned decode_shift;
  unsigned data;
  void (*decode) (struct rs232 *rs232);
  void (*handle) (struct rs232 *rs232);
};

extern void rs232_reset (struct rs232 *rs232, void (*handle) (struct rs232 *));
extern void rs232_encode (struct rs232 *rs232, unsigned data);
extern void rs232_encode_clock (struct rs232 *rs232);
extern void rs232_decode_clock (struct rs232 *rs232);
