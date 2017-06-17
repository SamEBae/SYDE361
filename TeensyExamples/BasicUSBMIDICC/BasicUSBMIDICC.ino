#include <Bounce2.h> // A library for debouncing switches

#define notePin 32 // Define the momentary switch pin used for input
#define potPin A0 // Define the pin used for reading the potentiometer

// A basic Sketch to send MIDI CC messages using the USB MIDI protocol
int channel = 1; // Defines the MIDI channel to send messages on (values from 1-16)
int velocity = 100; // Defines the velocity that the note plays at (values from 0-127)
int noteValue = 57; // Sets a MIDI note number for the note that is played when the button is pressed
int CC = 1; // The Control Change number - 1 is usually associated with the "mod wheel" (values from 0-127, but most are already predefined for certain purposes)

// Instantiate a Bounce object
Bounce note = Bounce(); 

// Set up integers to store the state of the note button and potentiometer value
int currNoteState = 0;
int prevNoteState = 0;
int currPotValue = 0;
int prevPotValue = 0;

void setup() {
  // Setup the note button with an internal pull-up :
  pinMode(notePin,INPUT_PULLUP);
  // After setting up the button, setup the Bounce instance :
  note.attach(notePin);
  note.interval(5); // interval in ms
}

void loop() {
  updateStates(); // Update the state of the button and the potentiometer value
  sendMIDImessages(); // Send MIDI messages as required
}

void updateStates() {
  note.update();
  currNoteState = note.read();
  currPotValue = analogRead(potPin)/8; // Read the pin and divide by 8 to get a value from (0-127)
}

void sendMIDImessages() {
  // Check if the Potentiometer value has changed, and send a new CC message if it has
  if(currPotValue != prevPotValue){
    usbMIDI.sendControlChange(CC,currPotValue,channel);
  }
  prevPotValue = currPotValue; // Update the prevPotValue for the next loop

  if(currNoteState != prevNoteState){ // If something has changed in the state of the button
    if(currNoteState == 0){  // If the button has been pressed (we're using internal pullups, so when the button is pressed the pin is pulled to ground)
      usbMIDI.sendNoteOn(noteValue,velocity,channel); // Turn the note ON
    } else { // If it isn't pressed, turn off the note
      usbMIDI.sendNoteOff(noteValue,0,channel); // Turn the note Off
    }
  }
  prevNoteState = currNoteState; // Update the prevNoteState for the next loop
}

