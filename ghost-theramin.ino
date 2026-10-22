#define MOZZI_CONTROL_RATE 64  // Hz, powers of 2 are most reliable; 64 Hz is actually the default, but shown here, for clarity
#include <Mozzi.h>
#include <Oscil.h>                // oscillator template
#include <Line.h>                 // linear interpolation between control updates
#include <Smooth.h>               // one-pole smoothing, to de-jitter the pot
#include <mozzi_fixmath.h>        // Q16n16 fixed-point frequency
#include <tables/sin2048_int8.h>  // sine table for oscillator

// use: Oscil <table_size, update_rate> oscilName (wavetable), look in .h file of table #included above
Oscil<SIN2048_NUM_CELLS, MOZZI_AUDIO_RATE> aSin(SIN2048_DATA);

// control-rate LFO for a constant, gentle vibrato
Oscil<SIN2048_NUM_CELLS, MOZZI_CONTROL_RATE> kVibrato(SIN2048_DATA);

const float MIN_FREQ = 100.0f;
const float MAX_FREQ = 2000.0f;

const float VIBRATO_RATE = 0.5f;
const float VIBRATO_DEPTH = 0.062f;

// one Line segment spans exactly one control update, so the glide always
// arrives just as the next target comes in
const unsigned int GLIDE_STEPS = MOZZI_AUDIO_RATE / MOZZI_CONTROL_RATE;
Line<Q16n16> aFreqGlide;

// 0.0 - 1.0; higher is smoother/laggier. Removes pot jitter before it becomes vibrato.
Smooth<unsigned int> kSmoothSensor(0.85f);

byte sensorValue;
byte gain = 50;  // out of 255

void setup() {
  startMozzi();  // :)
  kVibrato.setFreq(VIBRATO_RATE);
  aFreqGlide.set(float_to_Q16n16(MIN_FREQ));
}


void updateControl() {
  // put changing controls in here
  sensorValue = mozziAnalogRead<8>(1);
  unsigned int smoothed = kSmoothSensor.next(sensorValue);
  float frequency = MIN_FREQ + (smoothed / 255.0f) * (MAX_FREQ - MIN_FREQ);
  // proportional depth, so the wobble sounds the same at every pitch
  frequency *= 1.0f + (kVibrato.next() / 128.0f) * VIBRATO_DEPTH;
  aFreqGlide.set(float_to_Q16n16(frequency), GLIDE_STEPS);
}


AudioOutput updateAudio() {
  aSin.setFreq_Q16n16(aFreqGlide.next());  // new frequency every audio sample
  return MonoOutput::fromNBit(16, (int16_t)aSin.next() * gain);
}


void loop() {
  audioHook();  // required here
}
