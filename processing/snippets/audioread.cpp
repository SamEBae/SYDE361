//check if the sensor is on
if (analogRead(GATE_PIN) == GATE_ON) {
    int freq = analogRead(AUDIO_PIN);
    Serial.println(analogRead(AUDIO_PIN));
    tuned_note n = freq_to_note(freq, pitch_freqs);
    int index = n.getPitch();
    note_name note = *pitch_names[index];
    Serial.printf("closest tuned_note %c%c\n", note.getName(), note.getModifier());
    Serial.println();
}