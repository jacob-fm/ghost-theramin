#define MOZZI_CONTROL_RATE 64  // Hz, powers of 2 are most reliable; 64 Hz is actually the default, but shown here, for clarity
#include <Mozzi.h>
#include <Oscil.h>                // oscillator template
#include <Line.h>                 // linear interpolation between control updates
#include <Smooth.h>               // one-pole smoothing, to de-jitter the pot
#include <mozzi_fixmath.h>        // Q16n16 fixed-point frequency
#include <tables/sin2048_int8.h>  // sine table for oscillator

// use: Oscil <table_size, update_rate> oscilName (wavetable), look in .h file of table #included above
Oscil<SIN2048_NUM_CELLS, MOZZI_AUDIO_RATE> aSin(SIN2048_DATA);

// control-rate LFOs: a slow swell plus a faster shimmer, compounded
Oscil<SIN2048_NUM_CELLS, MOZZI_CONTROL_RATE> kVibrato(SIN2048_DATA);
Oscil<SIN2048_NUM_CELLS, MOZZI_CONTROL_RATE> kVibratoFast(SIN2048_DATA);

const int FREQ_INPUT = 1;
const int WOW_INPUT = 2;
const int FLUTTER_INPUT = 3;

const float MIN_FREQ = 100.0f;
const float MAX_FREQ = 2000.0f;

const float WOW_RATE = 0.5f;
const float WOW_DEPTH = 0.062f;
const float WOW_MAX = 0.8f;
const float WOW_MIN = 0.03f;

// keep the fast rate well under MOZZI_CONTROL_RATE / 2 so the LFO stays
// sampled finely enough to read as a wobble rather than a stepped buzz
const float FLUTTER_RATE = 6.0f;
const float FLUTTER_DEPTH = 0.038f;
const float FLUTTER_MAX = 0.4f;
const float FLUTTER_MIN = 0.015f;

// one Line segment spans exactly one control update, so the glide always
// arrives just as the next target comes in
const unsigned int GLIDE_STEPS = MOZZI_AUDIO_RATE / MOZZI_CONTROL_RATE;
Line<Q16n16> aFreqGlide;

// 0.0 - 1.0; higher is smoother/laggier. Removes pot jitter before it becomes vibrato.
Smooth<unsigned int> kSmoothSensor(0.85f);

byte freqInputVal;
byte wowInputVal;
byte flutterInputVal;
byte gain = 50;  // out of 255

void setup() {
  startMozzi();
  kVibrato.setFreq(WOW_RATE);
  kVibratoFast.setFreq(FLUTTER_RATE);
  aFreqGlide.set(float_to_Q16n16(MIN_FREQ));
}


void updateControl() {
  // put changing controls in here
  freqInputVal = mozziAnalogRead<8>(FREQ_INPUT);
  wowInputVal = mozziAnalogRead<8>(WOW_INPUT);
  flutterInputVal = mozziAnalogRead<8>(FLUTTER_INPUT);
  unsigned int smoothedFreq = kSmoothSensor.next(freqInputVal);
  float frequency = mapRange(smoothedFreq, 0.0f, 255.0f, MIN_FREQ, MAX_FREQ);
  // float frequency = MIN_FREQ + (smoothedFreq / 255.0f) * (MAX_FREQ - MIN_FREQ);
  float wow = mapRange(wowInputVal, 0.0f, 255.0f, WOW_MIN, WOW_MAX);
  float flutter = mapRange(flutterInputVal, 0.0f, 255.0f, FLUTTER_MIN, FLUTTER_MAX);
  // proportional depth, so the wobble sounds the same at every pitch;
  // the two LFOs compound, so the fast shimmer rides on top of the slow swell
  frequency *= 1.0f + (kVibrato.next() / 128.0f) * wow;
  frequency *= 1.0f + (kVibratoFast.next() / 128.0f) * flutter;
  aFreqGlide.set(float_to_Q16n16(frequency), GLIDE_STEPS);
}

float mapRange(float initVal, float inputMin, float inputMax, float outputMin, float outputMax) {
  return outputMin + (initVal - inputMin) / (inputMax - inputMin) * (outputMax - outputMin);
}

AudioOutput updateAudio() {
  aSin.setFreq_Q16n16(aFreqGlide.next());  // new frequency every audio sample
  return MonoOutput::fromNBit(16, (int16_t)aSin.next() * gain);
}


void loop() {
  audioHook();  // required here
}
