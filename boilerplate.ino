#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputUSB            usb1;           //xy=201,206
AudioAnalyzeNoteFrequency notefreq1;      //xy=445,146
AudioConnection          patchCord1(usb1, 0, notefreq1, 0);
// GUItool: end automatically generated code