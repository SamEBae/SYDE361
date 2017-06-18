#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "pitch_utils.h"
#include "note_name.h"
#include "tuned_note.h"

// GUItool: begin automatically generated code
AudioInputUSB            usb1;           //xy=71,90
AudioFilterBiquad        biquad1;        //xy=229,169
AudioOutputUSB           usb2;           //xy=448,94
AudioOutputI2S           i2s1;           //xy=453,164
AudioAnalyzeNoteFrequency notefreq;
AudioConnection          patchCord1(usb1, 0, i2s1, 0);
AudioConnection          patchCord2(usb1, 0, usb2, 0);
AudioConnection          patchCord3(usb1, 1, biquad1, 0);
AudioConnection          patchCord4(biquad1, 0, usb2, 1);
AudioConnection          patchCord5(biquad1, 0, i2s1, 1);
AudioConnection          patchCord6(usb1, 0, notefreq ,0);
AudioControlSGTL5000     sgtl5000_1;     //xy=219,29
// GUItool: end automatically generated code

// Global access
std::array<float, 88> pitch_freqs;
std::array<note_name *, 88> pitch_names;
//

void setup() {
  // put your setup code here, to run once:
  delay(250);
  AudioMemory(30);
  delay(250);

  notefreq.begin(0.15);
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.volume(0.5);
  biquad1.setLowpass(0,500, 0.707);

  // Pitch detection:
  float reference = 440.0;
  pitch_freqs = get_pitch_freqs(reference);
  pitch_names = get_pitch_names();
}

void loop() {
  delay(20);
  if (notefreq.available()) {
    int freq = notefreq.read();
    float prob = notefreq.probability();
    
    tuned_note n = freq_to_note(freq, pitch_freqs);
    int index = n.getPitch();
    double distance = n.getDistance();

    note_name note = *pitch_names[index];
    
    Serial.println(freq);
    Serial.println(prob);
    Serial.printf("closest tuned_note %c%c\n", note.getName(), note.getModifier());
  }
}
