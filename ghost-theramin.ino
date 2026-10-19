/*  Example playing a sinewave at a set frequency,
    using Mozzi sonification library.

    Demonstrates the use of Oscil to play a wavetable.

    Circuit: Audio output on digital pin 9 on a Uno or similar, or
    DAC/A14 on Teensy 3.1, or
    check the README or http://sensorium.github.io/Mozzi/

    Mozzi documentation/API
    https://sensorium.github.io/Mozzi/doc/html/index.html

    Mozzi help/discussion/announcements:
    https://groups.google.com/forum/#!forum/mozzi-users

    Copyright 2012-2024 Tim Barrass and the Mozzi Team

    Mozzi is licensed under the GNU Lesser General Public Licence (LGPL) Version 2.1 or later.
*/

#define MOZZI_CONTROL_RATE 64  // Hz, powers of 2 are most reliable; 64 Hz is actually the default, but shown here, for clarity
#include <Mozzi.h>
#include <Oscil.h>                // oscillator template
#include <tables/sin2048_int8.h>  // sine table for oscillator

// use: Oscil <table_size, update_rate> oscilName (wavetable), look in .h file of table #included above
Oscil<SIN2048_NUM_CELLS, MOZZI_AUDIO_RATE> aSin(SIN2048_DATA);
byte sensorValue;

void setup() {
  startMozzi();        // :)
  aSin.setFreq(1000);  // set the frequency
}


void updateControl() {
  // put changing controls in here
  sensorValue = mozziAnalogRead<8>(1);
  float frequency = map(sensorValue, 0, 1023, 100, 2000);  // scale it to use it with the servo (value between 0 and 180)
  aSin.setFreq(frequency);
}


AudioOutput updateAudio() {
  return MonoOutput::from8Bit(aSin.next());  // return an int signal centred around 0
}


void loop() {
  audioHook();  // required here
}
