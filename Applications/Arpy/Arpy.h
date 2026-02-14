#pragma once

#include "MatrixOS.h"
#include "Application.h"

// Configuration
#define OCTAVE           3      // determines note pitch
#define BPM              300    // increase for faster tempo
#define MIDI_CHANNEL     0      // MIDI channel (0-15, where 0 = channel 1)
#define MIDI_XCC         1      // CC number for X axis control
#define MIDI_IN_ENABLED  0

#define ARP_NOTE_COUNT   6      // Number of steps in arpeggiator pattern
#define POLYPHONY        4      // Max simultaneous notes
#define NULL_NOTE        0
#define NULL_ID          255
#define NULL_INDEX       255
#define ROW_OFFSET       5      // Difference in notes between rows
#define COLUMN_OFFSET    1      // ... columns

class Arpy : public Application {
public:
  // Application metadata
  static Application_Info info;

  // Colors
  // namespace ArpyColors {
  //   const Color off     = Color(0, 0, 0);
  //   const Color white   = Color(255, 255, 255);
  //   const Color grey    = Color(64, 64, 64);
  //   const Color red     = Color(255, 0, 0);
  //   const Color blue    = Color(0, 0, 255);
  //   const Color green   = Color(0, 255, 0);
  //   const Color teal    = Color(0, 255, 255);
  //   const Color magenta = Color(255, 0, 255);
  //   const Color yellow  = Color(255, 255, 0);
  // }

  // Data structure for tracking pressed notes
  struct PressedNote {
    uint16_t gridId;       // Grid ID
    uint8_t rootNote;  // The note key being pressed
    uint8_t currNote;  // Currently playing note from the arp sequence
    uint8_t arpIndex;  // Current index in the arp sequence
  };

  // Musical scales
  const uint8_t dorian_scale[] = { 0, 2, 3, 5, 7, 9, 10, 12 };
  const uint8_t ionian_scale[] = { 0, 2, 4, 5, 7, 9, 11, 12 };
  const uint8_t phrygian_scale[] = { 0, 1, 2, 3, 5, 7, 8, 10 };
  const uint8_t lydian_scale[] = { 0, 2, 4, 6, 7, 9, 10, 11 };
  const uint8_t mixolydian_scale[] = { 0, 2, 4, 5, 7, 9, 10, 12 };
  const uint8_t aeolian_scale[] = { 0, 2, 3, 5, 7, 8, 10, 12 };
  const uint8_t locrian_scale[] = { 0, 1, 3, 5, 6, 8, 10, 12 };
  const uint8_t launchpad_scale[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  // Current scale selection
  int8_t synthScale = dorian_scale;

  // Arpeggiator patterns (x, y offsets)
  const int8_t dipper_arp[ARP_NOTE_COUNT][2] = {
    {  0,  0 }, {  1,  0 }, {  2,  0 },
    {  2,  1 }, {  1,  1 }, {  1,  0 }
  };
  const int8_t circle_arp[ARP_NOTE_COUNT][2] = {
    {  0,  0 }, {  0, -1 }, {  1,  0 },
    {  0,  1 }, { -1,  0 }, {  0, -1 }
  };
  const int8_t onenote_arp[ARP_NOTE_COUNT][2] = {
    {  0,  0 }, {  0,  0 }, {  0,  0 },
    {  0,  0 }, {  0,  0 }, {  0,  0 }
  };
  const int8_t square_arp[ARP_NOTE_COUNT][2] = {
    {  0,  0 }, { -1, -1 }, {  1, -1 },
    {  1,  1 }, { -1,  1 }, { -1, -1 }
  };
  const int8_t sshape_arp[ARP_NOTE_COUNT][2] = {
    {  0,  0 }, {  0, -1 }, {  1, -1 },
    {  0,  0 }, {  0,  1 }, { -1,  1 }
  };
  const int8_t tshape_arp[ARP_NOTE_COUNT][2] = {
    {  0,  0 }, {  1,  0 }, {  2,  0 },
    {  2, -1 }, {  2,  0 }, {  2,  1 }
  };
  // Current pattern selection
  int8_t arpPattern = square_arp;

  // Core lifecycle methods
  void Setup(const vector<string>& args) override;
  void Loop() override;
  void End() override;

  // Event handlers
  void MidiEventHandler(MidiPacket* midiPacket);
  void KeyEventHandler(KeyEvent* keyEvent);

private:
  // State variables
  PressedNote notesHeld[POLYPHONY];
  uint32_t prevArpTime;
  uint32_t beatInterval;
  
  Color offColor  = Color(0, 0, 0);       //none/dark
  Color onColor   = Color(255, 255, 255); //white
  Color arpColor  = Color(64, 64, 64);    //grey

  // Arpeggiator functions
  void handleNoteOn(uint16_t gridId, uint8_t channel, uint8_t note, uint8_t velocity);
  void handleNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
  void respondToPresses();
  void playArpFromNoteKey(PressedNote* noot);
  void playArpNote(uint8_t note);
  void stopArpNote(uint8_t note);
  uint16_t noteToGridId(uint8_t note);

  // Utility functions
  void compact(PressedNote arr[], size_t len);
  uint8_t noteQuantized(uint8_t note);
  void printTupleArray(PressedNote arr[], uint8_t len);

};

