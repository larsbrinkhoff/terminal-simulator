#include <SDL.h>
#include "vt100.h"
#include "xsdl.h"
#include "event.h"
#include "log.h"

#define FREQUENCY 48000
#ifdef __FreeBSD__
# define SAMPLES 64 /* must be a power of two */
#else
# define SAMPLES (FREQUENCY * 3520 / 2764800)
#endif

int sound_scope;
static SDL_AudioDeviceID dev;
static float charge;
static float voltage;

static const float capacitance = 6.8e-6; //C8 in schematic.
static const float resistance1 = 390.0; //R16 in schematic.
static const float resistance2 = 16.0; //Speaker, just a guess.

static u8 buffer[SAMPLES];
static int flip_flop;

static void speaker (void);
static EVENT (sound_event, speaker);

void bell (void)
{
  //LOG (SND, "Bell");
  flip_flop = 0;
}

static u8 sample (void)
{
  const float dt = 1.0 / FREQUENCY;
  float current;
  if (flip_flop) {
    current = (5 - voltage) / resistance1;
    charge += dt * current;
    voltage = charge / capacitance;
    //LOG (SND, "Charging: %.3fA, %.6fC, %.2fV", current, charge, voltage);
  } else {
    current = -voltage / resistance2;
    charge += dt * current;
    voltage = charge / capacitance;
    if (voltage < 0.8)
      flip_flop = 1;
    //LOG (SND, "Discharging: %.3fA, %.6fC, %.2fV", current, charge, voltage);
  }
  return (u8)(255.0 * voltage / 5.0);
}

static void speaker (void)
{
  Uint32 queued;
  int i;
  //number of bytes
  queued = SDL_GetQueuedAudioSize (dev);
  if (queued == 0)
    LOG (SND, "Queue empty");
  else if (queued > 30*48)
    //Sleep until there are 20 ms of audio queued.
    SDL_Delay ((queued - 20*48) / 48);

  for (i = 0; i < SAMPLES; i++)
    buffer[i] = sample ();
  SDL_QueueAudio (dev, buffer, sizeof buffer);
  if (sound_scope)
    sdl_sound (buffer, sizeof buffer);
  add_event (2764800 * SAMPLES / FREQUENCY, &sound_event);
}

void reset_sound (void)
{
  SDL_AudioSpec want, have;

  sound_scope = 0;

  flip_flop = 1;
  charge = 0.0;
  voltage = 0.0;

  memset (&want, 0, sizeof want);
  want.freq = FREQUENCY;
  want.format = AUDIO_U8;
  want.channels = 1;
  want.samples = SAMPLES;

  dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
  if (dev == 0) {
    LOG (SND, "Could not open an audio device.");
    LOG (SND, "For now, audio is needed to synchronize simulated time against real time.");
    exit (1);
  }
  
  if (have.format != want.format)
    LOG (SND, "Didn't get wanted format.");
  LOG (SND, "Frequency: %d, channels: %d", have.freq, have.channels);
  speaker ();
  SDL_PauseAudioDevice (dev, 0);
}
