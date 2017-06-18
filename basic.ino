#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputUSB            usb1;           //xy=71,90
AudioFilterBiquad        biquad1;        //xy=229,169
AudioOutputUSB           usb2;           //xy=448,94
AudioOutputI2S           i2s1;           //xy=453,164
AudioAnalyzeNoteFrequency note1;
AudioConnection          patchCord1(usb1, 0, i2s1, 0);
AudioConnection          patchCord2(usb1, 0, usb2, 0);
AudioConnection          patchCord3(usb1, 1, biquad1, 0);
AudioConnection          patchCord4(biquad1, 0, usb2, 1);
AudioConnection          patchCord5(biquad1, 0, i2s1, 1);
AudioConnection          patchCord6(usb1, 0, note1 ,0);
AudioControlSGTL5000     sgtl5000_1;     //xy=219,29
// GUItool: end automatically generated code

void setup() {
  // put your setup code here, to run once:
  delay(250);
  AudioMemory(30);
  delay(250);

  note1.begin(0.15);
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.volume(0.5);
  biquad1.setLowpass(0,500, 0.707);    
}

void loop() {
  delay(20);
  if (note1.available()) {
    Serial.println(note1.read());
  }
}