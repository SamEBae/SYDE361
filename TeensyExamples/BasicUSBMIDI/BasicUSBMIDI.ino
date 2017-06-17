// A basic Sketch to send MIDI messages using the USB MIDI protocol

int channel = 1; // Defines the MIDI channel to send messages on (values from 1-16)
int velocity = 100; // Defines the velocity that the note plays at (values from 0-127)

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  randNote();
}

void randNote() {
  int note = random(32,48);
  usbMIDI.sendNoteOn(note,velocity,channel); // Turn the note ON
  delay(random(100,500));
  usbMIDI.sendNoteOff(note,0,channel); // Turn the note OFF - don't forget to do this ;)
  delay(10);
}

