/* MIDI Pattern Sequencer for Mystrix
 * Ported from Collin Cunningham's Neotrellis version for Adafruit Industries
 * Inspired by Stretta's Polygome
 *
 * To Do:
 * - light any corresponding LED for each note played
 * - fix note to grid mapping
 * - add config menu
 * - add option for external clock sync
 * - add option to select pattern
 * - add option to select scale
 * 
 */

#include "Arpy.h"

// Application metadata
Application_Info Arpy::info = {
  .name = "Arpy",
  .author = "Collin Cunningham",
  .color = Color(0xFF00FF),  // Magenta
  .version = 1,
  .visibility = true
};

void Arpy::Setup(const vector<string>& args) {
  MLOGI("Arpy", "MIDI Pattern Sequencer Started");

  // Initialize state variables
  beatInterval = 60000L / BPM;
  prevArpTime = 0L;
  // offColor = ArpyColors::off;
  // onColor = ArpyColors::white;
  // arpColor = ArpyColors::grey;

  // Initialize notes held array
  for (int i = 0; i < POLYPHONY; i++) {
    notesHeld[i] = {NULL_ID, NULL_NOTE, NULL_NOTE, NULL_INDEX};
  }

  // Clear LED grid
  MatrixOS::LED::Fill(offColor);
  MatrixOS::LED::Update();

  // Optional: Startup animation (similar to launchpad test)
  // MLOGI("Arpy", "Running startup animation...");
  // for (int i = 11; i < 89; i++) {
  //   MidiPacket noteOn = MidiPacket::NoteOn(MIDI_CHANNEL, i, 119);
  //   MatrixOS::MIDI::Send(noteOn);
  //   MatrixOS::SYS::DelayMs(22);
  //   MidiPacket noteOff = MidiPacket::NoteOff(MIDI_CHANNEL, i, 119);
  //   MatrixOS::MIDI::Send(noteOff);
  // }

  MLOGI("Arpy", "Initialization complete. BPM: %d, Channel: %d", BPM, MIDI_CHANNEL + 1);
}

void Arpy::Loop() {
  uint32_t currentTime = MatrixOS::SYS::Millis();

  // Process incoming MIDI messages
  // if (MIDI_IN_ENABLED) {
  //   MidiPacket midiPacket;
  //   if (MatrixOS::MIDI::Get(&midiPacket)) {
  //     MidiEventHandler(&midiPacket);
  //   }
  // }

  // Process key events (grid buttons)
  KeyEvent keyEvent;
  if (MatrixOS::KeyPad::Get(&keyEvent)) {
    KeyEventHandler(&keyEvent);
  }

  // Internal clock - trigger arpeggiator at beat intervals
  if ((currentTime - prevArpTime) >= beatInterval) {
    respondToPresses();
    prevArpTime = currentTime;
  }
}

void Arpy::End() {
  MLOGI("Arpy", "Sequencer Exited");

  // Stop all playing notes
  for (int i = 0; i < POLYPHONY; i++) {
    if (notesHeld[i].currNote != NULL_NOTE) {
      stopArpNote(notesHeld[i].currNote);
    }
  }

  // Clear LEDs
  MatrixOS::LED::Fill(offColor);
  MatrixOS::LED::Update();
}

void Arpy::MidiEventHandler(MidiPacket* midiPacket) {
  uint8_t channel = midiPacket->Channel();

  // Check if this is a Note On message
  if ((midiPacket->Status() & 0xF0) == 0x90) {
    uint8_t note = midiPacket->Note();
    uint8_t velocity = midiPacket->Velocity();

    // Treat velocity 0 as Note Off
    if (velocity == 0) {
      handleNoteOff(channel, note, velocity);
    } else {
      handleNoteOn(noteToGridId(note), channel, note, velocity);
    }
  }
  // Check if this is a Note Off message
  else if ((midiPacket->Status() & 0xF0) == 0x80) {
    uint8_t note = midiPacket->Note();
    uint8_t velocity = midiPacket->Velocity();
    handleNoteOff(channel, note, velocity);
  }
}

void Arpy::KeyEventHandler(KeyEvent* keyEvent) {
  // Future enhancement: Use grid buttons to trigger notes
  // For now, this sequencer primarily works via MIDI input

  // Example: Map grid buttons to MIDI notes
  // uint8_t noteNum = keyEvent->ID() + 36; 
  uint16_t gridId = keyEvent->ID();
  Point xy = MatrixOS::KeyPad::ID2XY(gridId);
  uint8_t noteNum = 36 + (xy.x * COLUMN_OFFSET) + (xy.y * ROW_OFFSET); // +36 = start at C2
  uint16_t id = keyEvent->ID();

  if (keyEvent->Active()) {
    handleNoteOn(gridId, MIDI_CHANNEL, noteNum, 100);

    // Light up the pressed key
    // Point xy = MatrixOS::KeyPad::ID2XY(keyEvent->ID());
    MatrixOS::LED::SetColor(id, onColor);
    MatrixOS::LED::Update();
  } 
  
  else {
    handleNoteOff(MIDI_CHANNEL, noteNum, 100);

    // Turn off the LED
    // Point xy = MatrixOS::KeyPad::ID2XY(keyEvent->ID());
    MatrixOS::LED::SetColor(id, offColor);
    MatrixOS::LED::Update();
  }
}

void Arpy::handleNoteOn(uint16_t gridId, uint8_t channel, uint8_t note, uint8_t velocity) {
  // Check if note is already in array
  for (int i = 0; i < POLYPHONY; i++) {
    if (notesHeld[i].rootNote == note) {
      MLOGD("Arpy", "Note already pressed: %d", note);
      return;
    }
    
    else if (notesHeld[i].rootNote == NULL_NOTE) {
      // Add the note here
      MLOGD("Arpy", "Adding note: %d", note);
      notesHeld[i] = {gridId, note, NULL_NOTE, NULL_INDEX};
      return;
    }
  }

  // If we get here, array is full - oldest note gets bumped
  MLOGD("Arpy", "Polyphony limit reached, note ignored: %d", note);
}

void Arpy::handleNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  // Find and remove the note from array
  for (int i = 0; i < POLYPHONY; i++) {
    if (notesHeld[i].rootNote == note) {
      MLOGD("Arpy", "Removing note: %d", note);
      stopArpNote(notesHeld[i].currNote);
      notesHeld[i] = {NULL_ID, NULL_NOTE, NULL_NOTE, NULL_INDEX};
      compact(notesHeld, POLYPHONY);
      return;
    }
  }
}

void Arpy::respondToPresses() {
  // Play arpeggio for each held note
  for (int i = 0; i < POLYPHONY; i++) {
    if (notesHeld[i].rootNote != NULL_NOTE) {
      playArpFromNoteKey(&notesHeld[i]);
    }
  }
}

void Arpy::playArpFromNoteKey(PressedNote* noot) {
  uint8_t seqIndex = 0;
  uint8_t oldSeqIndex = noot->arpIndex;

  // If not starting arp, increment index
  if (oldSeqIndex == NULL_INDEX) {
    seqIndex = 0;
    MLOGD("Arpy", "Starting arp for note: %d", noot->rootNote);
  } else {
    seqIndex = oldSeqIndex + 1;
  }

  // Loop sequence
  if (seqIndex >= ARP_NOTE_COUNT) {
    seqIndex = 0;
  }

  // Find & add note offsets from pattern
  int8_t x = ARP_PATTERN[seqIndex][0] * COLUMN_OFFSET;
  int8_t y = ARP_PATTERN[seqIndex][1] * ROW_OFFSET;

  uint8_t newNote = noot->rootNote + x + y;
  newNote = noteQuantized(newNote);

  uint8_t oldNote = noot->currNote;
  Point oldGridCoords = Point(ARP_PATTERN[oldSeqIndex][0],ARP_PATTERN[oldSeqIndex][1]);

  // Find grid xy coordinates for LED control
  Point gridCoords = MatrixOS::KeyPad::ID2XY(noot->gridId);
  // gridCoords.x += ARP_PATTERN[seqIndex][0];
  // gridCoords.y += ARP_PATTERN[seqIndex][1];
  gridCoords = gridCoords.operator+(Point(ARP_PATTERN[seqIndex][0],ARP_PATTERN[seqIndex][1]));

  if (oldNote != NULL_NOTE) {
    // Stop previous note in sequence
    stopArpNote(oldNote);
    // Turn off corresponding LED
    MatrixOS::LED::SetColor(oldGridCoords, offColor);
    MatrixOS::LED::Update();
  }

  // Play new note
  playArpNote(newNote);
  // Light corresponding LED
  MatrixOS::LED::SetColor(gridCoords, arpColor);
  MatrixOS::LED::Update();

  // Store new note
  noot->currNote = newNote;
  noot->arpIndex = seqIndex;

}

void Arpy::playArpNote(uint8_t note) {
  MidiPacket noteOn = MidiPacket::NoteOn(MIDI_CHANNEL, note, 100);
  MatrixOS::MIDI::Send(noteOn);
}

void Arpy::stopArpNote(uint8_t note) {
  MidiPacket noteOff = MidiPacket::NoteOff(MIDI_CHANNEL, note, 100);
  MatrixOS::MIDI::Send(noteOff);
}

void Arpy::compact(PressedNote arr[], size_t len) {
  // Remove NULL_NOTE entries and shift remaining entries up
  int i = 0;
  for (; i < len && arr[i].rootNote != 0; i++);
  if (i == len) return;

  int j = i;
  for (; i < len; i++) {
    if (arr[i].rootNote != 0) {
      arr[j++] = arr[i];
      arr[i] = {NULL_NOTE, NULL_NOTE, NULL_INDEX};
    }
  }
}

uint8_t Arpy::noteQuantized(uint8_t note) {
  // Find octave & offset
  uint8_t oct = note / 12;
  uint8_t ofs = note - (oct * 12);

  // Find nearest value from scale
  uint8_t nofs = SYNTH_SCALE[0];
  for (int i = 7; i > 0; i--) {
    if (ofs >= SYNTH_SCALE[i]) {
      nofs = SYNTH_SCALE[i];
      break;
    }
  }

  // Reapply octave
  note = oct * 12 + nofs;
  return note;
}

void Arpy::printTupleArray(PressedNote arr[], uint8_t len) {
  MLOGD("Arpy", "====START ARRAY====");
  for (int i = 0; i < len; i++) {
    MLOGD("Arpy", "INDEX %d root: %d curr: %d aidx: %d",
          i, arr[i].rootNote, arr[i].currNote, arr[i].arpIndex);
  }
  MLOGD("Arpy", "====END ARRAY====");
}

uint16_t Arpy::noteToGridId(uint8_t note) {
  int16_t offset = note - 36;  // Remove the base note offset
  
  // Try to find valid x,y coordinates (assuming 8x8 grid)
  for (int8_t y = 0; y < 8; y++) {
    int16_t remaining = offset - (y * ROW_OFFSET);
    
    // Check if x would be a valid integer
    if (remaining >= 0 && remaining % COLUMN_OFFSET == 0) {
      int8_t x = remaining / COLUMN_OFFSET;
      
      // Check if x is within grid bounds
      if (x >= 0 && x < 8) {
        Point xy(x, y);
        return MatrixOS::KeyPad::XY2ID(xy);
      }
    }
  }
  
  // If no valid grid position found, return NULL_ID
  return NULL_ID;
}
