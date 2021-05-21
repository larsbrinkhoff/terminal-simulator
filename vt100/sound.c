#include <SDL.h>
#include "vt100.h"

static SDL_AudioDeviceID dev;
static float charge;
static float vr, vc;

static const float capacitance = 6.8e-6;
static const float resistance = 390.0;
// T=0.002652  0.0012804097311139564

// I = VR/R
// Q += I * dt
// VC = Q/I

static u8 buffer[48000 / 100];
static int flip_flop;

static void speaker (void);
static EVENT (sound_event, speaker);

void bell (void)
{
  flip_flop = 0;
}

static u8 sample (void)
{
  float current;
  if (flip_flop) {
    current = vr / resistance;
    charge += current;
    vc = charge / current;
    vr = 5.0 - vc;
    vc = 5;
  } else {
    vr = -vc;
    current = vr / resistance;
    charge += current;
    vc = charge / current;
    vr = -vc;
    vc = 0;
    flip_flop = 1;
  }
  return (u8)(255.0 * vc / 5.0);
}

static void speaker (void)
{
  Uint32 queued;
  int i;
  //number of bytes
  queued = SDL_GetQueuedAudioSize (dev);
  if (queued == 0)
    LOG (SND, "Queue empty");
  else if (queued > 3*480)
    //Sleep until there are 20 ms of audio queued.
    SDL_Delay ((queued - 2*480) / 48);

  for (i = 0; i < 480; i++)
    buffer[i] = sample ();
  SDL_QueueAudio (dev, buffer, sizeof buffer);
  add_event (2764800 / 100, &sound_event);
}

void reset_sound (void)
{
  SDL_AudioSpec want, have;

  flip_flop = 1;
  charge = 0.0;
  vc = 5.0;
  vr = 0.0;

  memset (&want, 0, sizeof want);
  want.freq = 48000;
  want.format = AUDIO_U8;
  want.channels = 1;
  want.samples = 48000 / 100;

  dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
  if (dev == 0) {
    return;
  }
  
  if (have.format != want.format)
    LOG (SND, "Didn't get wanted format.");
  LOG (SND, "Frequency: %d, channels: %d", have.freq, have.channels);
  speaker ();
  SDL_PauseAudioDevice (dev, 0);
}
