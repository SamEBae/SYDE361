// A Basic Sketch to write to the Digital to Analog Convertors on the Teensy 3.5!

#define DAC0 A21
#define DAC1 A22

void setup() {
  analogWriteResolution(12); // Define the write precision for DAC0 and DAC1 pins 
  // by default the Teensy 3.5 DAC uses 3.3Vp-p output
  // if your 3.3V power has noise, switching to the
  // internal 1.2V reference can give you a clean signal
  //dac.analogReference(INTERNAL);

}

void loop() {
  analogWrite(DAC0,int(2/3.3*4096)); // Should output 2 Volts on DAC0 Pin
  analogWrite(DAC1,int(1/3.3*4096)); // Should output 1 Volt on DAC1 Pin
  delay(3000);
  analogWrite(DAC0,0); // Should output 0 Volts on DAC0 Pin
  analogWrite(DAC1,0); // Should output 0 Volts on DAC1 Pin
  delay(3000);  
}
