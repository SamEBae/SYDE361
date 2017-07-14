#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "pitch_utils.h"
#include "note_name.h"
#include "tuned_note.h"

// GUItool: begin automatically generated code
AudioInputAnalog         adc0(22);
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
  pinMode(A12,INPUT);
  pinMode(A13,INPUT);

  /*notefreq.begin(1);
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.volume(0.5);
  biquad1.setLowpass(0,500, 0.707);

  // Pitch detection:
  float reference = 440.0;
  pitch_freqs = get_pitch_freqs(reference);
  pitch_names = get_pitch_names();*/
}

const int GATE_ON = 1023;
const int GATE_PIN = A16;
const int AUDIO_PIN = A14;
const int ENVELOPE_PIN = A15;

int set_freq = 220; // used to be 880

void loop() {
  int digit = 0;
  int val = 0;
  delay(50);
  //analogWrite(A6,0);
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
   *  D[2] -  73.416
      C[3]  - 130.81
      E[3] -  164.81
      F[3]   - 174.61
      G[3]  - 196.00
      A[3]  - 220.00
      B[3]   - 246.94
      A[4]  - 440.00
   * 
   */
  /*
   * 
   * if (digitalRead(0)) set_freq = 880;
      else if (digitalRead(1)) set_freq = 988;
      else if (digitalRead(2)) set_freq = 1046;
      else if (digitalRead(3)) set_freq = 1175;
      else if (digitalRead(4)) set_freq = 1319;
      else if (digitalRead(5)) set_freq = 1397;
      else if (digitalRead(6)) set_freq = 1568;
      else if (digitalRead(7)) set_freq = 1760;

        if (digitalRead(0)) set_freq = 73;
      else if (digitalRead(1)) set_freq = 131;
      else if (digitalRead(2)) set_freq = 165;
      else if (digitalRead(3)) set_freq = 175;
      else if (digitalRead(4)) set_freq = 196;
      else if (digitalRead(5)) set_freq = 220;
      else if (digitalRead(6)) set_freq = 247;
      else if (digitalRead(7)) set_freq = 440;
   */
  //Calibration button
  /*if (digitalRead(7)) {
    analogWrite(A5,255);
    analogWrite(A21,127);
    
    return;
  }

  if (digitalRead(0)) set_freq = 131;
  else if (digitalRead(1)) set_freq = 165;
  else if (digitalRead(2)) set_freq = 175;
  else if (digitalRead(3)) set_freq = 196;
  else if (digitalRead(4)) set_freq = 220;
  else if (digitalRead(5)) set_freq = 247;
  else if (digitalRead(6)) set_freq = 440;
  //else if (digitalRead(7)) set_freq = 800;//doesn't work right now*/
  if (analogRead(A13) > 1000) {
    int buttonRead = analogRead(A13);
    //Calibration button
    if (buttonRead<24000 && buttonRead>21000) {
      analogWrite(A21,127);
      set_freq = 220;
      return;
    }
    else if (buttonRead<27000 && buttonRead>25000) set_freq = 262;
    else if (buttonRead<32000 && buttonRead>29000) set_freq = 278;
    else if (buttonRead<38000 && buttonRead>35000) set_freq = 294;
    else if (buttonRead<47000 && buttonRead>45000) set_freq = 312;
    else if (buttonRead<62000 && buttonRead>59000) set_freq = 330;
    else if (buttonRead<66000 && buttonRead>65000) set_freq = 350;
  }
  else if (analogRead(A12) > 1000){
    int buttonRead = analogRead(A12);
    if (buttonRead<66000 && buttonRead>65000) set_freq = 370;
    else if (buttonRead<62000 && buttonRead>59000) set_freq = 392;
    else if (buttonRead<47000 && buttonRead>44000) set_freq = 416;
    else if (buttonRead<38000 && buttonRead>35000) set_freq = 440;
    else if (buttonRead<31000 && buttonRead>29000) set_freq = 466;
    else if (buttonRead<27000 && buttonRead>25000) set_freq = 494;
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
    //tuned_note n = freq_to_note(set_freq, pitch_freqs);
    int index = n.getPitch();
    double distance = n.getDistance();      
    note_name note = *pitch_names[index];
    note_name note_desired = *pitch_names[freq_to_note(set_freq, pitch_freqs).getPitch()];
    
    compare_with_desired_pitch(set_freq, freq);
    //Serial.printf("{\"timestamp\": \"%d\", \"desired\": \"%d\", \"actual\": \"%d\", \"note\": \"%c%c\", \"octave\": \"%i\" }", millis(), set_freq, freq, note.getName(), note.getModifier() , note.getOctave()); 
    Serial.printf("{\"timestamp\": \"%d\", \"desired\": \"%d\", \"actual\": \"%d\", \"note\": \"%c%c\", \"octave\": \"%i\" }", millis(), set_freq, freq, note_desired.getName(), note_desired.getModifier() , note_desired.getOctave()); 
    Serial.println();
    
    //serialize_as_JSON(set_freq, freq, note.getName(), note.getModifier(), note.getOctave());
    //Serial.println(note.getName());
  }
}

void compare_with_desired_pitch(int desired_pitch , int actual_pitch ) {
  int difference = actual_pitch -  desired_pitch;
  int margin_of_error = actual_pitch * 0.075;
  
  note_name note_actual = *pitch_names[freq_to_note(actual_pitch, pitch_freqs).getPitch()];
  note_name note_desired = *pitch_names[freq_to_note(desired_pitch, pitch_freqs).getPitch()];

  // Output for user
  //Serial.printf("Note that you meant to sing: %c%c\t", note_desired.getName(), note_desired.getModifier());
  //Serial.printf(" note that you sang: %c%c", note_actual.getName(), note_actual.getModifier());
  //Serial.println("");
  
  int out = 0;
  if (abs(difference) < margin_of_error) {
    //Serial.println("perfect note!");
    out = 127;
  } else if (difference > 0) {
    //Serial.println("go down");
    if (abs(difference) < desired_pitch) {
      out = floor(abs(difference)/(2.0*desired_pitch)*128+127);
    }
    else out = 255;
  } else if (difference < 0) {
    //Serial.println("go up");
    out = ceil((1-abs(difference)/(1.0*desired_pitch))*128);
  }
  if (out > 255) out = 255;
  if (out < 0) out = 0;
  analogWrite(A6,255);
  analogWrite(A21,out);
}

// void serialize_as_JSON(int desired_pitch, int actual_pitch) {
//     Serial.printf("{\"timestamp\": \"%d\", \"desired\": \"%d\", \"actual\": \"%d\"}", millis(), desired_pitch, actual_pitch);
//     Serial.println();
// }

void serialize_as_JSON(int desired_pitch, int actual_pitch, const String& note, const String& modifier, const String& octave) {
  Serial.printf("{\"timestamp\": \"%d\", \"desired\": \"%d\", \"actual\": \"%d\", \"note\": \"%c%c\", \"octave\": \"%c\"}", millis(), desired_pitch, actual_pitch, note, modifier, octave);  
  Serial.println();
}
