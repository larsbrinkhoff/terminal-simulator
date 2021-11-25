#include "defs.h"
#include "xsdl.h"

#define CODES 0120
static SDL_atomic_t down[CODES];

uint8_t keymap (SDL_Scancode key)
{
  switch (key) {
  case SDL_SCANCODE_KP_7: return 0001;
  //case SDL_SCANCODE_DELETE: return 0002; // DELETE
  case SDL_SCANCODE_BACKSPACE: return 0002; // BACKSPACE
  case SDL_SCANCODE_F2: return 0004; // F2
  case SDL_SCANCODE_KP_1: return 0005;
  case SDL_SCANCODE_KP_4: return 0006;
  case SDL_SCANCODE_KP_5: return 0007;
  case SDL_SCANCODE_KP_2: return 0010;
  case SDL_SCANCODE_L: return 0011;
  case SDL_SCANCODE_5: return 0012;
  case SDL_SCANCODE_3: return 0013;
  case SDL_SCANCODE_8: return 0014;
  case SDL_SCANCODE_SPACE: return 0015;
  case SDL_SCANCODE_SCROLLLOCK: return 0016; // SCROLL
  case SDL_SCANCODE_KP_PERIOD: return 0017; // Num .
  case SDL_SCANCODE_2: return 0020;
  case SDL_SCANCODE_SEMICOLON: return 0021;
  case SDL_SCANCODE_T: return 0022;
  case SDL_SCANCODE_F: return 0023;
  case SDL_SCANCODE_I: return 0024;
  case SDL_SCANCODE_N: return 0025;
  case SDL_SCANCODE_TAB: return 0026;
    // {} 0027
  case SDL_SCANCODE_W: return 0030;
  case SDL_SCANCODE_O: return 0031;
  case SDL_SCANCODE_4: return 0032;
  case SDL_SCANCODE_V: return 0033;
  case SDL_SCANCODE_U: return 0034;
  case SDL_SCANCODE_M: return 0035;
  case SDL_SCANCODE_ESCAPE: return 0036;
  case SDL_SCANCODE_SLASH: return 0037;
  case SDL_SCANCODE_S: return 0040;
  case SDL_SCANCODE_9: return 0041;
  case SDL_SCANCODE_H: return 0042;
  case SDL_SCANCODE_B: return 0043;
  case SDL_SCANCODE_7: return 0044;
  case SDL_SCANCODE_J: return 0045;
  case SDL_SCANCODE_A: return 0046;
  case SDL_SCANCODE_APOSTROPHE: return 0047;
  case SDL_SCANCODE_C: return 0050;
  case SDL_SCANCODE_P: return 0051;
  case SDL_SCANCODE_R: return 0052;
  case SDL_SCANCODE_D: return 0053;
  case SDL_SCANCODE_6: return 0054;
  case SDL_SCANCODE_Y: return 0055;
  case SDL_SCANCODE_Q: return 0056;
  case SDL_SCANCODE_MINUS: return 0057;
  case SDL_SCANCODE_X: return 0060;
    // LINE FEED 0061 (F7)
  case SDL_SCANCODE_BACKSLASH: return 0062;
  case SDL_SCANCODE_EQUALS: return 0063;
  case SDL_SCANCODE_GRAVE: return 0064;
  case SDL_SCANCODE_PERIOD: return 0065;
  case SDL_SCANCODE_RETURN: return 0066;
    // COPY 0067
  case SDL_SCANCODE_Z: return 0070;
  case SDL_SCANCODE_0: return 0071;
  case SDL_SCANCODE_G: return 0072;
  case SDL_SCANCODE_E: return 0073;
  case SDL_SCANCODE_K: return 0074;
  case SDL_SCANCODE_COMMA: return 0075;
  case SDL_SCANCODE_1: return 0076;
  case SDL_SCANCODE_RIGHTBRACKET: return 0077;
  case SDL_SCANCODE_LEFTBRACKET: return 0077;
  case SDL_SCANCODE_RIGHT: return 0100;
  case SDL_SCANCODE_KP_9: return 0101;
  case SDL_SCANCODE_DOWN: return 0102;
  case SDL_SCANCODE_F3: return 0103;
  case SDL_SCANCODE_UP: return 0104;
  case SDL_SCANCODE_KP_3: return 0105;
  case SDL_SCANCODE_KP_6: return 0106;
  case SDL_SCANCODE_LEFT: return 0107;
  case SDL_SCANCODE_KP_0: return 0110;
  case SDL_SCANCODE_KP_ENTER: return 0111;
  case SDL_SCANCODE_KP_8: return 0112;
  case SDL_SCANCODE_F1: return 0113;
    // BREAK 0114
    // REPEAT 0115
  case SDL_SCANCODE_RCTRL:
  case SDL_SCANCODE_LCTRL: return 0116;
  case SDL_SCANCODE_RSHIFT:
  case SDL_SCANCODE_LSHIFT: return 0117;
  case SDL_SCANCODE_CAPSLOCK: return capslock;
  default: return 0;
  }
}

int key_flag (uint8_t code)
{
  return !SDL_AtomicGet (&down[code]);
}

void key_down (uint8_t code)
{
  printf ("Key down: %03o\n", code);
  SDL_AtomicSet (&down[code], 1);
}

void key_up (uint8_t code)
{
  printf ("Key up: %03o\n", code);
  SDL_AtomicSet (&down[code], 0);
}

void reset_keyboard (void)
{
  int i;
  SDL_AtomicSet (&down[0], 1);
  for (i = 1; i < CODES; i++)
    SDL_AtomicSet (&down[i], 0);
}
