#include "defs.h"
#include "vcd.h"

static unsigned int shifter = 0;   //E2  74164
static unsigned int counter1 = 0;  //E27 74161
static unsigned int counter2 = 0;  //E18 7490
static unsigned int counter3 = 0;  //E13 7493
static unsigned int counter4 = 1;  //E11 7493
static unsigned int counter5 = 0;  //E5  74161
// counter6 E7 74161

void timing (void)
{
#ifdef DEBUG_VCD
  long long t;
#endif

  if (shifter != 0377)
    shifter = (shifter << 1) | 1;
  else
    shifter <<= 1;
  shifter &= 0377;

  if (shifter == 0177) {
    counter1++;
  }

  if (counter1 == 10) {
    counter1 = 0;
    counter2++;
    counter3++;
  }

  if (counter2 == 5)
    counter2 = 8;
  else if (counter2 == 13)
    counter2 = 0;

  if (counter3 == 16) {
    counter3 = 0;
    counter4++;
  }

  if (counter4 == 16) {
    counter4 = 0;
    counter5++;
  }

  if (counter5 == 16) {
    tos = 0;
    counter5 = 6; // 60Hz: 6, 50Hz: 4
  }

  horiz = (counter2 & 010) != 0;

  scan = (counter2 == 0) && !(counter1 & 010);

  if (counter1 & 2) {
    if (scan)
      fly = 1;
    if (counter2 & 2)
      fly = 0;
  }

  vert = (counter4 & 014) == 010 && counter5 == 017;

  if (counter1 & 010)
    tos = 1;

#ifdef DEBUG_VCD
  t = 1e9 * cycles / 13.824e6;
  vcd_value (t, index_e2_74164,  shifter);
  vcd_value (t, index_e27_74161, counter1);
  vcd_value (t, index_e18_7490,  counter2);
  vcd_value (t, index_e13_7493,  counter3);
  vcd_value (t, index_e11_7493,  counter4);
  vcd_value (t, index_e5_74161,  counter5);
  //vcd_value (t, index_e6_74161, );
  vcd_value (t, index_horiz,     horiz);
  vcd_value (t, index_scan,      scan);
  vcd_value (t, index_fly,       fly);
  vcd_value (t, index_vert,      vert);
  vcd_value (t, index_tos,       tos);
#endif
}
