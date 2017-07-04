#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "pitch_utils.h"
#include "note_name.h"
#include "tuned_note.h"

// GUItool: begin automatically generated code
AudioInputAnalog         adc(20);
//AudioInputUSB            usb1;           //xy=71,90
AudioFilterBiquad        biquad1;        //xy=229,169
//AudioOutputUSB           usb2;           //xy=448,94
AudioOutputI2S           i2s1;           //xy=453,164
AudioAnalyzeFFT256       fft;       //xy=335,137
AudioAnalyzeNoteFrequency notefreq;
AudioConnection          patchCord1(adc, 0, i2s1, 0);
//AudioConnection          patchCord2(adc, 0, usb2, 0);
AudioConnection          patchCord3(adc, 1, biquad1, 0);
//AudioConnection          patchCord4(biquad1, 0, usb2, 1);
AudioConnection          patchCord5(biquad1, 0, i2s1, 1);
AudioConnection          patchCord6(adc, 0, notefreq ,0);
AudioConnection          patchCord7(adc, fft);
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

  notefreq.begin(1);
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.volume(0.5);
  biquad1.setLowpass(0,500, 0.707);

  // Pitch detection:
  float reference = 440.0;
  pitch_freqs = get_pitch_freqs(reference);
  pitch_names = get_pitch_names();
}

const int GATE_ON = 1023;
const int GATE_PIN = A16;
const int AUDIO_PIN = A14;
const int ENVELOPE_PIN = A15;

int set_freq = 770;

void loop() {
  int digit = 0;
  int val = 0;
  delay(50);
  // Reads in the frequency that the user wants
  if (Serial.available()) {
    val = 0;
    digit = Serial.available() - 1;
    
    while (Serial.available()) {
       int temp= Serial.read() - '0';
       val += temp * pow(10, digit);
       digit--;
    }
    
    set_freq = val;
    Serial.end();    // Ends the serial communication once all data is received
    Serial.begin(9600);  // Re-establishes serial communication , this causes deletion of anything previously stored in the buffer                             //or cache
  }
  
  /*
  if (fft.available()){
    Serial.println("yes");
    for (int i = 0 ; i < 128 ; i += 1) {
      Serial.println(6*fft.read(i));
    }
  }*/
      
  if (notefreq.available()) {
    int freq = notefreq.read();
    float prob = notefreq.probability();
   
    tuned_note n = freq_to_note(freq, pitch_freqs);
    int index = n.getPitch();
    double distance = n.getDistance();      
    note_name note = *pitch_names[index];

    //compare_with_desired_pitch(set_freq, freq);
    //Serial.println(analogRead(13)); // caps 65535
    //Serial.println(analogRead(12));
    analogWrite(A21, 70);
    analogWrite(A22, 70);
    serialize_as_JSON(set_freq, freq);    
  }
}

void compare_with_desired_pitch(int desired_pitch , int actual_pitch ) {
  int difference = actual_pitch -  desired_pitch;
  int margin_of_error = actual_pitch * 0.075;
  Serial.println(actual_pitch);
  
  note_name note_actual = *pitch_names[freq_to_note(actual_pitch, pitch_freqs).getPitch()];
  note_name note_desired = *pitch_names[freq_to_note(desired_pitch, pitch_freqs).getPitch()];

  // Output for user
  Serial.printf("Note that you meant to sing: %c%c\t", note_desired.getName(), note_desired.getModifier());
  Serial.printf(" note that you sang: %c%c", note_actual.getName(), note_actual.getModifier());
  Serial.println("");
  
  
  if (abs(difference) < margin_of_error) {
    Serial.println("perfect note!");
  } else if (difference > 0) {
    Serial.println("go down");
  } else if (difference < 0) {
    Serial.println("go up");
  }
}

void serialize_as_JSON(int desired_pitch, int actual_pitch) {
    
    Serial.printf("{\"timestamp\": \"%d\", \"desired\": \"%d\", \"actual\": \"%d\"}", millis(), desired_pitch, actual_pitch);
    Serial.println();
}
