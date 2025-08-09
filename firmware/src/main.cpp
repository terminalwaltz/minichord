#include "audio_definition.h"
#include "def.h"
#include <AT42QT2120.h>
#include <Arduino.h>
#include <Audio.h>
#include <LittleFS.h>
#include <SPI.h>
#include <Wire.h>
#include <button_matrix.h>
#include <debouncer.h>
#include <harp.h>
#include <potentiometer.h>

//>>SOFWTARE VERSION 
int version_ID=0007; //to be read 00.03, stored at adress 7 in memory
//>>BUTTON ARRAYS<<
debouncer harp_array[12];
debouncer chord_matrix_array[22];

//>>HARDWARE SETUP<<
harp harp_sensor;
button_matrix chord_matrix(SHIFT_DATA_PIN, SHIFT_STORAGE_CLOCK_PIN, SHIFT_CLOCK_PIN, READ_MATRIX_1_PIN, READ_MATRIX_2_PIN, READ_MATRIX_3_PIN);
debouncer hold_button;
debouncer up_button;
debouncer down_button;
debouncer LBO_flag;
bool flag_save_needed=false; //to know if we need to save the preset
potentiometer chord_pot(POT_CHORD_PIN);
potentiometer harp_pot(POT_HARP_PIN);
potentiometer mod_pot(POT_MOD_PIN);
LittleFS_Program myfs; // to save the settings
float color_led_blink_val = 1.0;
bool led_blinking_flag = false;
float led_attenuation = 0.0; 
//>>CHORD DEFINITION<<
//for each chord, we first have the 4 notes of the chord, then decoration that might be used in specific modes
uint8_t major[7] = {0, 4, 7, 12, 2, 5, 9};  // After the four notes of the chord (fundamental, third, fifth of seven, and octave of fifth, the next notes are the second fourth and sixth)
uint8_t minor[7] = {0, 3, 7, 12, 1, 5, 8};
uint8_t maj_sixth[7] = {0, 4, 7, 9, 2, 5, 12};
uint8_t min_sixth[7] = {0, 3, 7, 9, 1, 5, 12};
uint8_t seventh[7] = {0, 4, 10, 7, 2, 5, 9};
uint8_t maj_seventh[7] = {0, 4, 11, 7, 2, 5, 9};
uint8_t min_seventh[7] = {0, 3, 10, 7, 1, 5, 8};
uint8_t aug[7] = {0, 4, 8, 12, 2, 5, 9};
uint8_t dim[7] = {0, 3, 6, 12, 2, 5, 9};
uint8_t full_dim[7] = {0, 3, 6, 9, 2, 5, 12};
uint8_t key_signature_selection = 0; // 0=C, 1=G, 2=D, 3=A, 4=E, 5=B, 6=F, 7=Bb, 8=Eb, 9=Ab, 10=Db, 11=Gb
enum KeySig {
  KEY_SIG_C,       // 0 sharps, index 0
  KEY_SIG_G,       // 1 sharp, index 1
  KEY_SIG_D,       // 2 sharps, index 2
  KEY_SIG_A,       // 3 sharps, index 3
  KEY_SIG_E,       // 4 sharps, index 4
  KEY_SIG_B,       // 5 sharps, index 5
  KEY_SIG_F,       // 1 flat, index 6
  KEY_SIG_Bb,      // 2 flats, index 7
  KEY_SIG_Eb,      // 3 flats, index 8
  KEY_SIG_Ab,      // 4 flats, index 9
  KEY_SIG_Db,      // 5 flats, index 10
  KEY_SIG_Gb,      // 6 flats, index 11
  KEY_SIG_Csharp,  // 7 sharps, index 12
  KEY_SIG_Dsharp,  // 6 sharps (enharmonic to Eb), index 13
  KEY_SIG_Esharp,  // 11 sharps (enharmonic to F), index 14
  KEY_SIG_Fsharp,  // 6 sharps, index 15
  KEY_SIG_Gsharp,  // 4 sharps (enharmonic to Ab), index 16
  KEY_SIG_Asharp,  // 3 sharps (enharmonic to Bb), index 17
  KEY_SIG_Bsharp,  // 0 sharps (enharmonic to C), index 18
  KEY_SIG_Fb,      // 7 flats (enharmonic to E), index 19
  KEY_SIG_Cb       // 7 flats (enharmonic to B), index 20
};
enum Button { // Button enum in hardware order: B, E, A, D, G, C, F
  BTN_B, BTN_E, BTN_A, BTN_D, BTN_G, BTN_C, BTN_F
};
enum FrameShift { //Enums for chord frame shifts
  FRAMESHIFT_0, FRAMESHIFT_1,FRAMESHIFT_2,FRAMESHIFT_3,FRAMESHIFT_4,FRAMESHIFT_5,FRAMESHIFT_6
};
const int8_t base_notes[7] = {11, 4, 9, 2, 7, 0, 5}; // Base note offsets for buttons in key of C (relative to C4 = MIDI 60), in hardware order B, E, A, D, G, C, F
const int8_t key_offsets[21] = {
  0, 7, 2, 9, 4, 11, 5, 10, 3, 8, 1, 6, // C, G, D, A, E, B, F, Bb, Eb, Ab, Db, Gb
  1, 3, 5, 6, 8, 10, 0, // C#, D#, E#, F#, G#, A#, B#
  4, 11 // Fb, Cb
};
const int8_t key_signatures[21] = {
  0, 1, 2, 3, 4, 5, -1, -2, -3, -4, -5, -6, // C (0), G (1#), D (2#), A (3#), E (4#), B (5#), F (-1), Bb (-2), Eb (-3), Ab (-4), Db (-5), Gb (-6)
  7, 6, 11, 6, 4, 3, 0, // C# (7#), D# (6#), E# (11#), F# (6#), G# (4#), A# (3#), B# (0#)
  -7, -7 // Fb (-7), Cb (-7)
};
//const int8_t key_signatures[12] = {0, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 6}; // Number of sharps or flats for each key: Sharps for C, G, D, A, E, B; flats for F, Bb, Eb, Ab, Db, Gb
const int8_t sharp_notes[12][7] = {
  {BTN_F},                            // 1 sharp: F#
  {BTN_F, BTN_C},                     // 2 sharps: F#, C#
  {BTN_F, BTN_C, BTN_G},              // 3 sharps: F#, C#, G#
  {BTN_F, BTN_C, BTN_G, BTN_D},       // 4 sharps: F#, C#, G#, D#
  {BTN_F, BTN_C, BTN_G, BTN_D, BTN_A},// 5 sharps: F#, C#, G#, D#, A#
  {BTN_F, BTN_C, BTN_G, BTN_D, BTN_A, BTN_E},         // 6 sharps: F#, C#, G#, D#, A#, E#
  {BTN_F, BTN_C, BTN_G, BTN_D, BTN_A, BTN_E, BTN_B},  // 7 sharps: F#, C#, G#, D#, A#, E#, B#
  {BTN_F, BTN_C, BTN_G, BTN_D, BTN_A, BTN_E, BTN_B},  // 8 sharps: G#
  {BTN_F, BTN_C, BTN_G, BTN_D, BTN_A, BTN_E, BTN_B},  // 9 sharps: D#
  {BTN_F, BTN_C, BTN_G, BTN_D, BTN_A, BTN_E, BTN_B},  // 10 sharps: A#
  {BTN_F, BTN_C, BTN_G, BTN_D, BTN_A, BTN_E, BTN_B},  // 11 sharps: E#
  {BTN_F, BTN_C, BTN_G, BTN_D, BTN_A, BTN_E, BTN_B}   // 12 sharps: B#
};
const int8_t flat_notes[7][7] = {
  {BTN_B},                            // 1 flat: Bb
  {BTN_B, BTN_E},                     // 2 flats: Bb, Eb
  {BTN_B, BTN_E, BTN_A},              // 3 flats: Bb, Eb, Ab
  {BTN_B, BTN_E, BTN_A, BTN_D},       // 4 flats: Bb, Eb, Ab, Db
  {BTN_B, BTN_E, BTN_A, BTN_D, BTN_G},// 5 flats: Bb, Eb, Ab, Db, Gb
  {BTN_B, BTN_E, BTN_A, BTN_D, BTN_G, BTN_C},         // 6 flats: Bb, Eb, Ab, Db, Gb, Cb
  {BTN_B, BTN_E, BTN_A, BTN_D, BTN_G, BTN_C, BTN_F}   // 7 flats: Bb, Eb, Ab, Db, Gb, Cb, Fb
};

uint8_t scalar_harp_selection = 0; // 0=Chord-based, 1=Major, 2=Major Pentatonic, 3=Minor Pentatonic, 4=Diminished 6th Scale, 5=Relative Natural Minor, 6=Relative Harmonic Minor, 7=Relative Minor Pentatonic, 8/9=Chord-specific scales
const uint8_t scale_intervals[8][8] = {
    {0, 2, 4, 5, 7, 9, 11, 0}, // Major (Ionian)
    {0, 2, 4, 7, 9, 0, 0, 0},  // Major Pentatonic
    {0, 2, 3, 7, 10, 0, 0, 0}, // Minor Pentatonic
    {0, 2, 4, 5, 7, 8, 9, 11}, // Diminished 6th Scale
    {0, 2, 3, 5, 7, 8, 10, 0}, // Relative Natural Minor
    {0, 2, 3, 5, 7, 8, 11, 0}, // Relative Harmonic Minor
    {0, 2, 3, 7, 10, 0, 0, 0}, // Relative Minor Pentatonic
    {0, 0, 0, 0, 0, 0, 0, 0}   // Scale Per Chord Mode
};
const uint8_t scale_lengths[8] = {7, 5, 5, 8, 7, 7, 5, 7};
const uint8_t chord_scale_intervals[15][8] = {
    {0, 2, 4, 7, 9, 0, 0, 0},  // Major Pentatonic for major
    {0, 2, 4, 6, 9, 0, 0, 0},  // Lydian Pentatonic for maj_seventh
    {0, 3, 5, 7, 10, 0, 0, 0}, // Minor Pentatonic for minor
    {0, 2, 4, 7, 10, 0, 0, 0}, // Mixolydian Pentatonic for seventh
    {0, 3, 5, 7, 9, 0, 0, 0},  // Dorian Pentatonic for min_seventh
    {0, 1, 3, 4, 6, 7, 9, 10}, // Octatonic for dim
    {0, 2, 4, 6, 8, 10, 0, 0}, // Whole Tone for aug
    {0, 2, 4, 5, 7, 8, 9, 11}, // Diminished 6th for maj_sixth
    {0, 2, 3, 5, 7, 8, 9, 11}, // Diminished 6th Minor for min_sixth
    {0, 2, 3, 4, 6, 7, 9, 11}, // Offset Diminished 6th for full_dim
    {0, 2, 4, 5, 7, 9, 11, 0}, // Major (Ionian) for major
    {0, 2, 3, 5, 7, 9, 10, 0}, // Dorian for min_seventh
    {0, 2, 4, 6, 7, 9, 11, 0}, // Lydian for maj_seventh
    {0, 2, 4, 5, 7, 9, 10, 0}, // Mixolydian for seventh
    {0, 2, 3, 5, 7, 8, 10, 0}  // Natural Minor for minor
};
const uint8_t chord_scale_lengths[15] = {5, 5, 5, 5, 5, 8, 6, 8, 8, 8, 7, 7, 7, 7, 7};

// Key and column names for logging and key signature changes
const char* key_names[21] = {
  "C", "G", "D", "A", "E", "B", "F", "Bb", "Eb", "Ab", "Db", "Gb",
  "C#", "D#", "E#", "F#", "G#", "A#", "B#",
  "Fb", "Cb"
};
const char* column_names[] = {"B", "E", "A", "D", "G", "C", "F"};

float c_frequency = 130.81;                      // for C3
uint8_t chord_octave_change=4;
uint8_t harp_octave_change=4;
uint8_t chord_frame_shift=0;
uint8_t transpose_semitones=0;                       // to use to transpose the instrument, number of semitones
uint8_t (*current_chord)[7] = &major;            // the array holding the current chord
uint8_t current_chord_notes[7];                  // the array for the note calculation within the chord, calculate 7 of them for the arpeggiator mode
uint8_t current_applied_chord_notes[7];          // the array for the note calculation within the chord
uint8_t current_harp_notes[12];                  // the array for the note calculation within the string

//>>SWITCHING LOGIC GLOBAL VARIABLES<<
int8_t current_line = -1;      // holds the current selected line of button, -1 if nothing is on
int8_t fundamental = 0;        // holds the value of the last selected line, hence the fundamental
uint8_t slash_value = 0;       // stores the "slash", ie when a different alternative note is selected
bool slash_chord = false;      // flag for when a slashed chord is currently activated
bool button_pushed = false;    // flag for when any button has been pushed during the main loop
bool trigger_chord = false;    // flag to trigger the enveloppe of the chord
bool chord_sharpened = false; // Tracks if the current chord is sharpened
bool sharp_active = false;     // flag for when the sharp is active
bool flat_button_modifier= false; //flag to set the modifier to flat instead of sharp
bool continuous_chord = false; // wether the chord is held continuously. Controlled by the "hold" button
bool rythm_mode = false;
bool barry_harris_mode = false;
IntervalTimer note_timer[4]; // timers for delayed chord enveloppe
bool inhibit_button=false;

//>>SWITCHING LOGIC PARAMETERS<<
uint8_t note_slash_level = 0;     // the level we are replacing in the chord when slashing (usually the fundamental)
bool retrigger_chord = true;      // wether or not to retrigger the enveloppe when the chord is switched within current line (including when selecting slash chord)
bool change_held_strings = false; // to control wether hold strings change with chord:
bool chromatic_harp_mode = false; // to switch the harp to chromatic mode
//>>SYSEX PARAMETERS<<
// SYSEX midi message are used to control up to 256 synthesis parameters.
const uint16_t parameter_size = 256;
const uint8_t preset_number = 12;
int16_t default_bank_sysex_parameters[preset_number][parameter_size] = {
  {0,0,50,50,512,512,512,0,0,0,192,100,49,100,184,100,157,100,0,0,0,0,0,0,43,0,50,37,38,67,0,0,0,0,0,0,0,0,0,0,0,16,0,8,8,12,42,1171,1,423,20,70,3,35,83,59,2658,1,0,0,0,0,0,0,0,1,1,1,100,1,1,0,1,1,1,1,14,0,0,70,0,0,0,100,0,6,0,0,755,195,23,61,29,0,0,0,0,162,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,13,8,100,16,0,200,0,0,50,0,50,18,32,50,0,0,10,66,353,65,995,1,569,16,141,32,83,28,48,54,1,0,0,0,56,0,389,0,20,0,0,0,0,1,1,1,0,1,1,0,1,1,1,1,0,0,0,70,0,0,0,100,0,64,0,0,80,16,4,94,753,474,70,5,100,100,100,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,6,6,32,0,6,0,16,0,6,6,32,0,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,50,50,512,512,512,0,0,0,194,100,85,100,60,100,61,100,0,0,10,1,0,0,0,0,0,0,0,100,0,0,0,0,0,0,0,0,0,0,4,25,8,3,29,18,65,488,3,159,25,70,5,18,4,26,6,1,77,0,587,32,0,390,0,76,1,1,100,1,1,68,17,14,22,20,0,340,1682,70,48,0,0,100,18,54,0,0,800,70,57,100,100,0,0,0,0,199,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,7,0,100,0,0,100,6,8,200,2,50,36,75,50,28,0,3,1,1,80,1218,2,1659,38,114,19,8,32,80,1,1,0,30,0,0,0,0,0,0,0,24,0,3,1,1,1,100,1,1,100,1,1,1,1,31,0,0,70,0,0,0,100,0,33,2,1,162,16,4,100,100,1436,118,100,50,32,107,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,13,0,4,0,7,0,0,2,13,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,50,50,512,512,512,0,0,0,192,100,49,100,184,100,157,100,0,0,30,0,1,1,87,33,62,12,80,67,0,0,0,0,0,0,0,0,0,0,0,6,3,8,8,12,42,1855,1,42,20,217,3,35,83,59,2658,1,185,0,282,14,0,247,11,1,1,1,100,1,1,0,1,1,1,1,14,0,0,70,0,0,0,100,0,23,0,0,755,195,82,61,29,0,0,0,0,162,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,13,0,100,23,8,200,0,0,50,0,50,18,32,50,0,0,10,66,353,44,1452,1,569,16,141,32,83,28,48,54,1,0,698,82,56,0,579,0,28,0,0,0,0,1,1,1,0,1,1,100,1,1,1,1,0,0,0,70,0,0,0,100,0,100,0,0,80,16,4,94,753,474,70,5,100,100,100,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,6,6,32,0,6,0,16,0,6,6,32,0,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,50,50,512,512,512,0,0,0,196,76,92,100,184,100,85,100,0,0,60,0,1,0,14,0,0,0,42,46,0,0,0,0,0,0,0,0,0,0,3,10,8,11,42,30,61,2137,1,106,22,140,4,35,83,24,1956,1,139,0,282,0,0,247,10,1,1,1,100,1,1,0,1,1,1,1,0,0,0,70,0,0,0,100,0,57,0,1,858,70,66,100,100,0,0,0,0,127,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,17,8,200,11,0,134,10,0,100,7,50,59,15,50,0,15,10,10,45,61,1489,1,652,16,84,32,21,15,33,19,1,0,490,0,109,0,252,0,8,0,244,1,49,1,1,1,15,1,1,100,1,1,1,1,26,479,1931,70,0,50,0,100,40,78,21,0,162,16,4,100,1000,639,140,100,47,100,85,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,65,0,12,0,0,12,0,0,65,0,6,0,0,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,50,50,512,512,512,0,0,0,146,100,76,100,85,79,184,100,0,0,110,0,0,0,57,0,0,0,67,58,0,0,0,0,0,0,0,0,0,0,0,6,8,8,65,12,38,1855,5,57,24,152,3,35,83,59,2658,6,137,0,640,8,0,247,11,1,1,1,100,1,1,131,4,116,3,1,11,424,1360,70,46,0,0,100,60,85,0,0,996,125,82,61,71,0,0,0,0,90,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,20,0,100,23,0,200,15,8,100,0,50,0,18,50,0,0,2,293,17,44,1012,1,10,33,141,32,83,28,67,239,1,0,0,0,287,0,579,0,0,0,366,0,35,5,52,12,0,1,1,100,1,1,1,1,16,0,0,70,0,0,0,100,0,50,0,0,184,16,4,100,100,657,70,86,100,100,95,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,2,4,2,15,0,2,2,6,2,6,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,50,50,512,512,512,0,0,0,192,60,47,56,159,100,157,100,0,0,138,1,0,0,0,0,0,0,27,74,0,0,0,0,0,0,0,0,0,0,4,14,11,3,29,18,65,1102,3,302,11,70,5,18,4,26,6,1,38,0,516,16,0,0,0,76,1,1,100,1,1,68,17,14,22,20,0,340,1682,70,48,0,0,100,18,58,0,0,800,70,43,100,100,0,0,0,0,200,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,10,11,100,9,0,200,4,8,200,0,50,36,75,50,0,0,3,1,1,80,1855,2,769,15,114,19,8,32,80,1,1,0,30,0,11,0,467,0,35,0,24,0,3,1,1,1,100,1,1,100,1,1,1,1,0,341,2164,70,54,0,0,100,36,42,0,1,80,16,4,100,20,2101,70,100,6,22,55,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,4,8,17,0,12,0,1,2,4,8,16,6,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,50,50,512,512,512,0,0,0,192,100,61,100,130,100,83,100,0,0,175,0,0,0,0,0,0,0,0,35,0,0,0,0,0,0,0,0,0,0,0,12,9,11,1,1,70,2141,1,383,17,302,101,103,34,30,1,12,0,0,367,17,0,363,5,1,1,1,86,1,1,0,1,1,1,1,8,0,0,70,0,0,0,100,0,33,50,0,5000,70,100,0,0,0,0,0,0,90,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,9,9,200,13,6,99,6,0,50,27,50,50,50,50,0,0,345,1,1,80,800,32,2200,0,70,1,1,1,100,1,1,0,30,0,0,0,166,0,6,0,0,0,0,1,1,1,0,1,1,100,1,1,1,1,0,0,0,70,0,0,0,100,0,59,100,0,80,16,4,100,100,2366,70,100,40,23,57,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,4,4,16,16,12,0,1,6,8,6,0,6,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,50,50,512,512,512,0,0,0,146,100,76,100,85,79,184,100,0,0,220,0,0,0,0,0,0,86,55,63,0,0,0,0,0,0,0,0,0,0,0,6,8,8,65,12,38,1855,5,57,24,152,3,35,83,59,2658,6,137,0,640,8,0,247,11,1,1,1,100,1,1,131,4,116,3,1,11,424,1360,70,46,0,0,100,60,84,0,0,996,125,82,61,71,0,0,0,0,90,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,20,0,100,23,0,200,15,8,100,0,50,0,18,50,0,0,2,293,17,44,1012,1,10,33,141,32,83,28,67,239,1,0,0,0,287,0,579,0,0,0,366,0,35,5,52,12,0,1,1,100,1,1,1,1,16,0,0,70,0,0,0,100,0,50,0,0,184,16,4,100,100,657,70,86,100,100,95,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,2,4,2,15,0,2,2,6,2,6,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,50,50,512,512,512,0,0,0,196,100,47,56,159,100,157,100,0,0,253,1,0,0,0,0,0,0,0,59,0,0,0,0,0,0,0,0,0,0,4,15,11,3,29,18,65,1102,3,302,11,70,5,18,4,26,6,1,38,0,516,16,0,0,12,76,1,22,100,1,1,68,17,14,22,20,0,340,1682,70,48,0,0,100,18,58,0,0,800,70,43,100,100,0,0,0,0,158,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,10,0,100,9,0,200,4,8,200,0,50,36,75,50,0,0,3,1,1,80,1855,2,769,15,114,19,8,32,80,1,1,0,30,0,11,0,467,0,22,0,24,0,3,1,1,1,100,1,1,100,1,1,1,1,0,341,2164,70,54,0,0,100,36,42,0,1,80,16,4,100,20,532,70,100,6,82,96,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,4,8,17,0,12,0,1,2,4,8,16,6,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,50,50,512,512,512,0,0,0,149,69,76,100,85,79,184,100,0,0,266,0,0,0,0,0,0,0,65,70,0,0,0,0,0,0,0,0,0,0,0,5,0,8,65,12,38,1855,5,57,24,152,3,35,83,59,2658,6,137,0,640,8,0,247,11,1,1,1,100,1,1,131,4,116,3,1,0,424,1360,70,46,0,0,100,60,81,0,0,996,125,82,61,71,0,0,0,0,108,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,20,12,100,23,0,200,15,8,100,0,50,45,17,0,0,0,2,293,17,44,1012,1,10,33,141,32,83,28,67,239,1,0,0,0,287,0,579,0,0,0,366,0,35,5,52,12,0,1,1,100,1,1,1,1,16,0,0,70,0,0,0,100,0,50,0,0,184,16,4,100,100,857,70,42,100,100,158,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,2,4,2,15,0,2,2,6,2,6,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,50,50,512,512,512,0,0,0,192,55,61,100,184,100,85,100,0,0,310,0,0,0,86,0,56,87,37,60,0,0,0,0,0,0,0,0,0,0,4,15,12,36,42,30,59,1410,1,30,33,140,4,35,83,24,615,1,94,0,282,15,0,247,0,1,1,1,100,1,1,0,1,1,1,1,0,0,0,70,0,0,0,100,0,40,0,1,858,70,46,100,82,0,0,0,0,124,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,16,8,200,18,0,134,21,0,200,2,50,59,15,50,18,15,10,66,43,35,711,1,271,16,84,32,83,733,31,54,1,0,698,0,220,0,252,0,8,0,244,1,49,1,1,1,15,1,1,100,1,1,1,1,26,479,1931,70,0,50,0,100,34,75,0,0,162,16,4,100,1000,1889,116,49,100,42,129,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,65,0,12,0,0,12,0,0,65,0,6,0,0,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,50,50,512,512,512,0,0,0,194,100,85,100,60,100,61,100,0,0,340,1,0,0,0,0,0,0,0,62,0,0,0,0,0,0,0,0,0,0,4,25,3,3,29,18,65,488,3,159,25,70,5,18,4,26,40,1,77,0,587,32,0,715,11,76,1,1,100,1,1,68,17,14,22,20,17,340,1682,70,48,0,0,100,18,54,0,0,800,70,57,100,100,0,0,0,0,199,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,3,1,100,0,0,100,6,8,200,2,50,36,75,50,28,0,3,1,1,80,1218,2,706,38,114,19,8,32,80,1,1,0,30,0,0,0,0,0,0,0,24,0,3,1,1,1,100,1,1,100,1,1,1,1,31,0,0,70,0,0,0,100,0,33,2,1,162,16,4,100,100,678,118,100,50,32,168,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,13,0,4,0,7,0,0,2,13,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
}; 
int16_t current_sysex_parameters[parameter_size] = {0,0,50,50,512,512,512,1,0,0,192,100,49,100,184,100,157,100,0,0,0,0,0,0,0,0,0,0,0,67,0,0,0,0,0,0,0,0,0,0,0,16,0,8,8,12,42,1171,1,423,20,70,3,35,83,59,2658,1,0,0,0,0,0,0,0,1,1,1,100,1,1,0,1,1,1,1,14,0,0,70,0,0,0,100,0,6,0,0,755,195,23,61,29,0,0,0,0,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,13,8,100,16,0,200,0,0,50,0,50,18,32,50,0,0,10,66,353,65,995,1,569,16,141,32,83,28,48,54,1,0,0,0,56,0,389,0,20,0,0,0,0,1,1,1,0,1,1,0,1,1,1,1,0,0,0,70,0,0,0,100,0,38,0,0,80,16,4,94,753,474,70,5,100,100,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,6,6,32,0,6,0,16,0,6,6,32,0,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
const char *bank_name[preset_number] = {"a.txt", "b.txt", "c.txt", "d.txt", "e.txt", "f.txt", "g.txt", "h.txt", "i.txt", "j.txt", "k.txt", "l.txt"};
int8_t current_bank_number = 0;
float bank_led_hue = 0;
// Reserved SYSEX adresses
// =0 is for the control command
// 1 is the bank ID
// >2 and <10 are protected
int8_t harp_volume_sysex = 2;
int8_t chord_volume_sysex = 3;
int8_t chord_pot_alternate_storage = 4;
int8_t harp_pot_alternate_storage = 5;
int8_t mod_pot_alternate_storage = 6;

// >10 and <21 are limited access, for example the potentiometer settings (we don't want a pot to control another pot sysex adress or range)
int8_t chord_pot_alternate_control = 10;
int8_t chord_pot_alternate_range = 11;
int8_t harp_pot_alternate_control = 12;
int8_t harp_pot_alternate_range = 13;
int8_t mod_pot_main_control = 14;
int8_t mod_pot_main_range = 15;
int8_t mod_pot_alternate_control = 16;
int8_t mod_pot_alternate_range = 17;
// 21-39 are global parameters (switching logic, global reverb etc.)
// 40-119 are harp parameters
// 120-219 are chord parameters
// 220-235 are rythm patterns
bool sysex_controler_connected=false; //bool to remember if there is a controller that is connected to avoid saving any change

//>>AUDIO OBJECT ARRAYS<<
// for the strings
AudioSynthWaveformModulated *string_waveform_array[12] = {&waveform_string_1, &waveform_string_2, &waveform_string_3, &waveform_string_4, &waveform_string_5, &waveform_string_6, &waveform_string_7, &waveform_string_8, &waveform_string_9, &waveform_string_10, &waveform_string_11, &waveform_string_12};
AudioEffectEnvelope *string_enveloppe_array[12] = {&envelope_string_1, &envelope_string_2, &envelope_string_3, &envelope_string_4, &envelope_string_5, &envelope_string_6, &envelope_string_7, &envelope_string_8, &envelope_string_9, &envelope_string_10, &envelope_string_11, &envelope_string_12};
AudioEffectEnvelope *string_enveloppe_filter_array[12] = {&envelope_filter_1, &envelope_filter_2, &envelope_filter_3, &envelope_filter_4, &envelope_filter_5, &envelope_filter_6, &envelope_filter_7, &envelope_filter_8, &envelope_filter_9, &envelope_filter_10, &envelope_filter_11, &envelope_filter_12};
AudioMixer4 *string_mixer_array[3] = {&string_mix_1, &string_mix_2, &string_mix_3};
AudioFilterStateVariable *string_filter_array[12] = {&filter_string_1, &filter_string_2, &filter_string_3, &filter_string_4, &filter_string_5, &filter_string_6, &filter_string_7, &filter_string_8, &filter_string_9, &filter_string_10, &filter_string_11, &filter_string_12};
AudioSynthWaveform *string_transient_waveform_array[12] = {&waveform_transient_1, &waveform_transient_2, &waveform_transient_3, &waveform_transient_4, &waveform_transient_5, &waveform_transient_6, &waveform_transient_7, &waveform_transient_8, &waveform_transient_9, &waveform_transient_10, &waveform_transient_11, &waveform_transient_12};
AudioEffectEnvelope *string_transient_envelope_array[12] = {&envelope_transient_1, &envelope_transient_2, &envelope_transient_3, &envelope_transient_4, &envelope_transient_5, &envelope_transient_6, &envelope_transient_7, &envelope_transient_8, &envelope_transient_9, &envelope_transient_10, &envelope_transient_11, &envelope_transient_12};
AudioMixer4 *transient_mixer_array[3] = {&transient_mix_1, &transient_mix_2, &transient_mix_3};
// for the chord
AudioEffectEnvelope *chord_vibrato_envelope_array[4] = {&voice1_vibrato_envelope, &voice2_vibrato_envelope, &voice3_vibrato_envelope, &voice4_vibrato_envelope};
AudioEffectEnvelope *chord_vibrato_dc_envelope_array[4] = {&voice1_vibrato_dc_envelope, &voice2_vibrato_dc_envelope, &voice3_vibrato_dc_envelope, &voice4_vibrato_dc_envelope};
AudioMixer4 *chord_vibrato_mixer_array[4] = {&voice1_vibrato_mixer, &voice2_vibrato_mixer, &voice3_vibrato_mixer, &voice4_vibrato_mixer};
AudioSynthWaveformModulated *chord_osc_1_array[4] = {&voice1_osc1, &voice2_osc1, &voice3_osc1, &voice4_osc1};
AudioSynthWaveformModulated *chord_osc_2_array[4] = {&voice1_osc2, &voice2_osc2, &voice3_osc2, &voice4_osc2};
AudioSynthWaveformModulated *chord_osc_3_array[4] = {&voice1_osc3, &voice2_osc3, &voice3_osc3, &voice4_osc3};
AudioSynthNoiseWhite *chord_noise_array[4] = {&voice1_noise, &voice2_noise, &voice3_noise, &voice4_noise};
AudioMixer4 *chord_voice_mixer_array[4] = {&voice1_mixer, &voice2_mixer, &voice3_mixer, &voice4_mixer};
AudioFilterStateVariable *chord_voice_filter_array[4] = {&voice1_filter, &voice2_filter, &voice3_filter, &voice4_filter};
AudioEffectEnvelope *chord_envelope_filter_array[4] = {&voice1_envelope_filter, &voice2_envelope_filter, &voice3_envelope_filter, &voice4_envelope_filter};
AudioEffectMultiply *chord_tremolo_mult_array[4] = {&voice1_tremolo_mult, &voice2_tremolo_mult, &voice3_tremolo_mult, &voice4_tremolo_mult};
AudioEffectEnvelope *chord_envelope_array[4] = {&voice1_envelope, &voice2_envelope, &voice3_envelope, &voice4_envelope};

// Timing window
#define CHORD_WINDOW_MS 50  // Window for initial chord detection
elapsedMillis chord_window_timer;
bool chord_window_active = false;
bool chord_window_processed = false;
bool chord_buttons_pressed[22] = {false};
static int active_line = -1; // Preserve across calls

//>>SYNTHESIS VARIABLE<<
// waveshaper shape
float wave_shape[257] = {};
float ws_sin_param = 1;
// waveform array 
int8_t waveform_array[12] = {
    0, //WAVEFORM_SINE
    1, //WAVEFORM_SAWTOOTH
    2, //WAVEFORM_SQUARE
    3, //WAVEFORM_TRIANGLE
    12, //WAVEFORM_BANDLIMIT_PULSE
    5, //WAVEFORM_PULSE
    6, //WAVEFORM_SAWTOOTH_REVERSE
    7, //WAVEFORM_SAMPLE_HOLD 
    8, //WAVEFORM_TRIANGLE_VARIABLE
    9, //WAVEFORM_BANDLIMIT_SAWTOOTH
    10,//WAVEFORM_BANDLIMIT_SAWTOOTH_REVERSE
    11, //WAVEFORM_BANDLIMIT_SQUARE
}; 
// shuffling arrays and index for the harp
int8_t harp_shuffling_array[7][12] = {
    //each number indicates the note for the string 0-6 are taken within the chord pattern. 
    //the /10 number indicates the octave
    {0, 1, 2, 10, 11, 12, 20, 21, 22, 30, 31, 32},
    {4, 1, 0, 2, 14, 11, 10, 12, 24, 21, 20, 22}, //add the seconds
    {5, 2, 0, 1, 15, 12, 10, 11, 25, 22, 20, 21}, //add the fourth
    {6, 2, 0, 1, 16, 12, 10, 11, 26, 22, 20, 21}, //add the sixth
    {0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23}, //replaced octave by barry_harris shuffling array 
    {0, 4, 1, 5, 2, 6, 10, 14, 11, 15, 12, 16}, //chromatic
    {0, 10, 20, 1, 11, 21, 2, 12, 22, 3, 13, 23}}; //special array for keymaster/barry_harris combo
int8_t harp_shuffling_selection = 0;
int8_t transient_note_level=0; //level of the note of the transient in the scale;
int8_t chord_shuffling_array[6][7] = {
    //each number indicates the note for the voice 0-6 are taken within the chord pattern. In normal mode, only 0-3 is used, and 4-6 is available in rythm mode 
    //the /10 number indicates the octave
    {0, 1, 2, 3, 4, 5, 6}, //normal 
    {10, 11, 12, 13, 14, 15, 16},//one octave up with chromatics
    {10, 11, 12, 13, 0, 2, 3},//one octave up with low chord notes
    {10, 11, 12, 13, 2, 5, 6},//one octave up with low fifth and low chromatics
    {10, 11, 12, 13, 2, 15, 16},//one octave up with low fifth and high chromatics
    {20, 21, 22, 23, 24, 25, 26}};//two octave up
int8_t chord_shuffling_selection = 0;
// retrigger release for chord delayed note

int chord_retrigger_release=0;
// strings filter parameters
float string_filter_keytrack = 0;
int string_filter_base_freq = 0;
// lfo for chord parameters
float chord_vibrato_base_freq = 0;
float chord_vibrato_keytrack = 0;
float chord_tremolo_base_freq = 0;
float chord_tremolo_keytrack = 0;
float chord_filter_base_freq = 0;
float chord_filter_keytrack = 0;
// frequency mutlipliers for chord
float osc_1_freq_multiplier = 1;
float osc_2_freq_multiplier = 1;
float osc_3_freq_multiplier = 1;
// string delay_parameter
u_int32_t inter_string_delay = 30000;
u_int32_t random_delay = 10000;
// pan for audio output 
float pan=1;
float reverb_dry_proportion=0.6; //to avoid drop in volume in full reverb, keep some part of the dry signal in

//>>AUTO RYTHM<<
u_int8_t rythm_pattern[16] = {};
float rythm_bpm = 80;
u_int8_t rythm_current_step = 0;
u_int16_t note_pushed_duration = 30;
float shuffle = 1;
u_int32_t long_timer_period = shuffle * (60 * 1000 * 1000) / (2 * rythm_bpm);
u_int32_t short_timer_period = 2 * (60 * 1000 * 1000) / (2 * rythm_bpm) - long_timer_period;
bool current_long_period = true;
bool rythm_timer_running = false;
IntervalTimer rythm_timer;       // that gives the general rythm
IntervalTimer note_off_timer[4]; // timers for delayed chord enveloppe
IntervalTimer led_timer;
IntervalTimer color_led_blink_timer;
elapsedMillis note_off_timing[4];
elapsedMicros last_midi_clock_in;
int midi_clock_current_step=0;

uint8_t rythm_limit_change_to_every = 2; // when we allow the chord change
elapsedMillis since_last_button_push;
elapsedMillis last_key_change;

uint8_t rythm_freeze_current_chord_notes[7]; // this array is needed because we need to handle the situation when a
uint8_t rythm_loop_length = 16;
u_int8_t current_selected_voice=0; //increment at each voice steal to rotate amongst voices;

//-->>MIDI PARAMETERS
uint8_t chord_port=0;
uint8_t chord_attack_velocity=127;
uint8_t chord_release_velocity=20;
uint8_t chord_started_notes[4]={0,0,0,0};                   
uint8_t harp_port=1;
uint8_t harp_attack_velocity=127; 
uint8_t harp_release_velocity=20;
uint8_t harp_started_notes[12]={0,0,0,0,0,0,0,0,0,0,0,0};    
uint8_t midi_base_note=48; // for C3
uint8_t midi_base_note_transposed=midi_base_note; //to handle note transposition

//-->>FUNCTION THAT NEED ANNOUNCING
void save_config(int bank_number, bool default_save);
void load_config(int bank_number);
void recalculate_timer();
uint8_t calculate_note_harp(uint8_t string, bool slashed, bool sharp);
uint8_t calculate_note_chord(uint8_t voice, bool slashed, bool sharp);
void set_chord_voice_frequency(uint8_t i, uint16_t current_note);
void calculate_ws_array();
void rythm_tick_function();

//-->>LED HSV CALCULATION
// function to calculate led RGB value, thank you SO
void set_led_color(float h, float s, float v) {
  double hh, p, q, t, ff;
  long i;
  double r, g, b;
  if (s <= 0.0) {
    r = v;
    g = v;
    b = v;
    analogWrite(R_LED_PIN, 0);
    analogWrite(G_LED_PIN, 0);
    analogWrite(B_LED_PIN, 0);
    return;
  }
  hh = h;
  if (hh >= 360.0)
    hh = 0.0;
  hh /= 60.0;
  i = (long)hh;
  ff = hh - i;
  p = v * (1.0 - s);
  q = v * (1.0 - (s * ff));
  t = v * (1.0 - (s * (1.0 - ff)));
  switch (i) {
  case 0:
    r = v;
    g = t;
    b = p;
    break;
  case 1:
    r = q;
    g = v;
    b = p;
    break;
  case 2:
    r = p;
    g = v;
    b = t;
    break;
  case 3:
    r = p;
    g = q;
    b = v;
    break;
  case 4:
    r = t;
    g = p;
    b = v;
    break;
  case 5:
  default:
    r = v;
    g = p;
    b = q;
    break;
  }
  analogWrite(R_LED_PIN, r * 200);
  analogWrite(G_LED_PIN, g * 115);
  analogWrite(B_LED_PIN, b * 70);
  return;
}

//-->>UTILITIES FOR SYSEX HANDLING
void control_command(uint8_t command, uint8_t parameter) {
  switch (command) {
  case 0: // SIGNAL TO SEND BACK ALL DATA
    Serial.println("Reporting all data");
    int8_t midi_data_array[parameter_size * 2];
    for (int i = 0; i < parameter_size; i++) {
      midi_data_array[2 * i] = current_sysex_parameters[i] % 128;
      midi_data_array[2 * i + 1] = current_sysex_parameters[i] / 128;
    }
    usbMIDI.sendSysEx(parameter_size * 2, (const uint8_t *)&midi_data_array,0);
    break;
  case 1: // SIGNAL TO WIPE MEMORY
    Serial.println("Wiping memory");
    digitalWrite(_MUTE_PIN, LOW); // muting the DAC
    myfs.quickFormat();
    current_bank_number = 0;
    load_config(current_bank_number);
    digitalWrite(_MUTE_PIN, HIGH); // unmuting the DAC
    break;
  case 2: // saving bank
    Serial.print("Saving to bank: ");
    Serial.println(parameter);
    save_config(parameter, false);
    break;
  case 3: // setting bank to default
    Serial.print("Saving to bank: ");
    Serial.println(parameter);
    current_bank_number = parameter;
    save_config(parameter, true);
    break;

  default:
    break;
  }
}
// the autogenerated code (see ./generator for the script)
#include <sysex_handler.h>
void processMIDI(void) {
  byte type;
  type = usbMIDI.getType();
  if (type == usbMIDI.SystemExclusive && usbMIDI.getSysExArrayLength() == 6) {
    sysex_controler_connected=true; //we can say for sure a controller is connected
    const byte *data = usbMIDI.getSysExArray();
    int adress = data[1] + 128 * data[2];
    if (adress == 0) { // it is a control command
      control_command(data[3], data[4]);
    } else {
      Serial.print("Received instruction on adress:");
      Serial.print(adress);
      int value = data[3] + 128 * data[4];
      Serial.print(" with value:");
      Serial.println(value);
      current_sysex_parameters[adress] = value;
      apply_audio_parameter(adress, value);
    }
  }
  if(type==usbMIDI.Start && rythm_mode){
    rythm_current_step=0;
    midi_clock_current_step=0;
    rythm_tick_function();
    Serial.println("Start received");
    rythm_timer.end();
  }
  if(type==usbMIDI.Stop && rythm_mode){
    rythm_timer.begin(rythm_tick_function, short_timer_period);
  }


  if(type==usbMIDI.Clock && rythm_mode){
    //here we want half of the cycle to be synced with the midi clock, and half with the calculated internal clock, so we can still have shuffle
    //recalculate the BPM
    rythm_bpm=(rythm_bpm*10+(1000*1000*60/last_midi_clock_in)/24)/11.0;
    last_midi_clock_in=0;
    midi_clock_current_step+=1;
    recalculate_timer();   
    //once every two beat, we sync
    if(midi_clock_current_step==24){
      rythm_timer.begin(rythm_tick_function, short_timer_period);
      rythm_tick_function();
      midi_clock_current_step=0;
    }
    //We disable the timer to avoid having it trigger the tick too early when we arrive at the sync beat 
    if(midi_clock_current_step>18){
      rythm_timer.end();
    }

   
  
   
  }
}

//-->>TIMER FUNCTIONS
// function to handle the delayed chord activation
void play_single_note(int i, IntervalTimer *timer) {
    timer->end();
    if (chord_envelope_array[i]->isActive()) {
        chord_vibrato_envelope_array[i]->noteOff();
        chord_vibrato_dc_envelope_array[i]->noteOff();
        chord_envelope_array[i]->noteOff();
        chord_envelope_filter_array[i]->noteOff();
    }
    set_chord_voice_frequency(i, current_applied_chord_notes[i]);
    chord_vibrato_envelope_array[i]->noteOn();
    chord_vibrato_dc_envelope_array[i]->noteOn();
    chord_envelope_array[i]->noteOn();
    chord_envelope_filter_array[i]->noteOn();
    if (chord_started_notes[i] != 0) {
        usbMIDI.sendNoteOff(chord_started_notes[i], chord_release_velocity, 1, chord_port);
        chord_started_notes[i] = 0;
    }
    usbMIDI.sendNoteOn(midi_base_note_transposed + current_applied_chord_notes[i], chord_attack_velocity, 1, chord_port);
    chord_started_notes[i] = midi_base_note_transposed + current_applied_chord_notes[i];
}

void play_note_selected_duration(int i,int current_note){
  chord_vibrato_envelope_array[i]->noteOn();
  chord_vibrato_dc_envelope_array[i]->noteOn();
  chord_envelope_array[i]->noteOn();
  chord_envelope_filter_array[i]->noteOn();
  note_off_timing[i]=0;
  if(chord_started_notes[i]!=0){
    usbMIDI.sendNoteOff(chord_started_notes[i],chord_release_velocity,1,chord_port);
    chord_started_notes[i]=0;}
  usbMIDI.sendNoteOn(midi_base_note_transposed+current_note,chord_attack_velocity,1,chord_port);
  chord_started_notes[i]=midi_base_note_transposed+current_note;
}

void turn_off_led(IntervalTimer *timer) {
  timer->end();
  analogWrite(RYTHM_LED_PIN, 0);
}

//-->>AUDIO HELPER FUNCTIONS
// calculationg the ws array
void calculate_ws_array() {
  for (int i = 0; i < 257; i++) {
    float current_x = (i / 256.0 - 0.5) * 2.0 * PI;
    wave_shape[i] = sin(current_x);
    for (int j = 0; j < ws_sin_param; j++) {
      wave_shape[i] = sin(wave_shape[i] * PI);
    }
  }
}
// setting the pad_frequency
void set_chord_voice_frequency(uint8_t i, uint16_t current_note) {
  float note_freq = pow(2,chord_octave_change)*c_frequency/8 * pow(2, (current_note+transpose_semitones) / 12.0); //down one octave to let more possibilities with the shuffling array
  AudioNoInterrupts();
  chords_vibrato_lfo.frequency(chord_vibrato_base_freq + chord_vibrato_keytrack * current_chord_notes[0]);
  chords_tremolo_lfo.frequency(chord_tremolo_base_freq + chord_tremolo_keytrack * current_chord_notes[0]);
  // hord_vibrato_lfo_array[i]->frequency(chord_vibrato_base_freq);
  // chord_tremolo_lfo_array[i]->frequency(chord_tremolo_base_freq);
  chord_voice_filter_array[i]->frequency(note_freq * chord_filter_keytrack + chord_filter_base_freq);
  chord_osc_1_array[i]->frequency(osc_1_freq_multiplier * note_freq);
  chord_osc_2_array[i]->frequency(osc_2_freq_multiplier * note_freq);
  chord_osc_3_array[i]->frequency(osc_3_freq_multiplier * note_freq);
  // chord_voice_filter_array[i]->frequency(1*freq);
  AudioInterrupts();
  if(chord_started_notes[i]!=0 && chord_started_notes[i]!=midi_base_note_transposed+current_note){
    //we need to change the note without triggering the change, ie a pitch bend
    usbMIDI.sendNoteOff(chord_started_notes[i],chord_release_velocity,1,chord_port);
    chord_started_notes[i]=0;
    usbMIDI.sendNoteOn(midi_base_note_transposed+current_note,chord_attack_velocity,1,chord_port);
    chord_started_notes[i]=midi_base_note_transposed+ current_note;
  }
}
// setting the harp
void set_harp_voice_frequency(uint8_t i, uint16_t current_note) {
  float note_freq =  pow(2,harp_octave_change)*c_frequency/4 * pow(2, (current_note+transpose_semitones) / 12.0);
  float transient_freq =  64.0*c_frequency/4 *pow(2, ((current_note+transpose_semitones)%12+transient_note_level) / 12.0);
  AudioNoInterrupts();
  string_waveform_array[i]->frequency(note_freq);
  string_transient_waveform_array[i]->frequency(transient_freq);
  string_filter_array[i]->frequency(string_filter_base_freq + note_freq * string_filter_keytrack);
  // string_vibrato_1.offset(0);
  AudioInterrupts();
}
// Function to compute MIDI note offset dynamically with circular frame shift
int8_t get_root_button(uint8_t key, uint8_t shift, uint8_t button) {
  int8_t note = base_notes[button]; // Start with base note in C (e.g., B=11, E=4, A=9, D=2, G=7, C=0, F=5)
  
  // Map button to musical note index (C=0, D=1, E=2, F=3, G=4, A=5, B=6)
  int8_t musical_index;
  switch (button) {
    case BTN_B: musical_index = 6; break; // B
    case BTN_E: musical_index = 2; break; // E
    case BTN_A: musical_index = 5; break; // A
    case BTN_D: musical_index = 1; break; // D
    case BTN_G: musical_index = 4; break; // G
    case BTN_C: musical_index = 0; break; // C
    case BTN_F: musical_index = 3; break; // F
    default: musical_index = 0; // Should not happen
  }
  
  // Apply circular frame shift: move notes up an octave if needed
  if (musical_index < shift) {
    note += 12; // Move up one octave if the note is shifted "on top"
  }
  
  // Apply key signature (sharps or flats)
  int8_t num_accidentals = key_signatures[key];
  if (key <= KEY_SIG_B || (key >= KEY_SIG_Csharp && key <= KEY_SIG_Bsharp)) { // Sharp keys: C, G, D, A, E, B, C#, D#, E#, F#, G#, A#, B#
    int8_t max_accidentals = (num_accidentals > 7) ? 7 : num_accidentals;
    for (int i = 0; i < max_accidentals; i++) {
      if (button == sharp_notes[num_accidentals - 1][i]) {
        note += 1; // Add sharp
        // Double sharps for C# (index 12)
        if (key == KEY_SIG_Csharp && (button == BTN_C || button == BTN_F || button == BTN_A)) {
          note += 1; // C##, F##, A##
        }
        // Double sharps for F# (index 15)
        if (key == KEY_SIG_Fsharp && (button == BTN_B || button == BTN_E)) {
          note += 1; // B##, E##
        }
        // Double sharps for E# (index 14, treat as 7 sharps for practicality)
        if (key == KEY_SIG_Esharp && (button == BTN_F || button == BTN_C || button == BTN_G || button == BTN_D || button == BTN_A || button == BTN_E)) {
          note += 1; // F##, C##, G##, D##, A##, E##
        }
      }
    }
  } else { // Flat keys: F, Bb, Eb, Ab, Db, Gb, Fb, Cb
    int8_t max_accidentals = (-num_accidentals > 7) ? 7 : -num_accidentals;
    for (int i = 0; i < max_accidentals; i++) {
      if (button == flat_notes[max_accidentals - 1][i]) {
        note -= 1; // Add flat
      }
    }
  }
  
  return note; // No need to constrain here
}
// function to calculate the frequency of individual chord notes
uint8_t calculate_note_chord(uint8_t voice, bool slashed, bool sharp) {
  uint8_t note = 0;
  uint8_t level = chord_shuffling_array[chord_shuffling_selection][voice];
  if (slashed && level % 10 == note_slash_level) {
    if (!flat_button_modifier) {
      note = (12 * int(level / 10) + get_root_button(key_signature_selection, chord_frame_shift, slash_value) + sharp * 1.0);
    } else {
      note = (12 * int(level / 10) + get_root_button(key_signature_selection, chord_frame_shift, slash_value) - sharp * 1.0);
    }
  } else {
    if (!flat_button_modifier) {
      note = (12 * int(level / 10) + get_root_button(key_signature_selection, chord_frame_shift, fundamental) + sharp * 1.0 + (*current_chord)[level % 10]);
    } else {
      note = (12 * int(level / 10) + get_root_button(key_signature_selection, chord_frame_shift, fundamental) - sharp * 1.0 + (*current_chord)[level % 10]);
    }
  }
  return note;
    Serial.print("calculate_note_chord: voice="); Serial.print(voice);
    Serial.print(", slashed="); Serial.print(slashed);
    Serial.print(", sharp="); Serial.print(sharp);
    Serial.print(", fundamental="); Serial.print(fundamental);
    Serial.print(", slash_value="); Serial.print(slash_value);
    Serial.print(", note="); Serial.println(note + midi_base_note_transposed);
}
// function to calculate the level of individual harp touch
uint8_t calculate_note_harp(uint8_t string, bool slashed, bool sharp) {
    uint8_t effective_fundamental = (current_line >= 0) ? current_line : fundamental;
    if (effective_fundamental > 6) {
        effective_fundamental = 0;
        Serial.printf("Invalid fundamental=%d, defaulting to 0\n", fundamental);
    }
    if (scalar_harp_selection >= 1 && scalar_harp_selection <= 7) {
        // Scale modes 1-7: Use predefined scales
        uint8_t scale_index = scalar_harp_selection - 1;
        uint8_t scale_length = scale_lengths[scale_index];
        uint8_t octave = (string / scale_length) > 0 ? (string / scale_length) : 0;
        uint8_t scale_degree = string % scale_length;
        uint8_t root_note = key_offsets[key_signature_selection];
        if (scalar_harp_selection >= 5) {
            root_note = (root_note - 3) % 12;
            if (root_note > 127) root_note += 12;
        }
        uint8_t note = root_note + scale_intervals[scale_index][scale_degree] + (octave * 12);
        Serial.printf("Scale mode %d, string %d, note=%d\n", scalar_harp_selection, string, note + 12);
        return note + 12;
    } else if (scalar_harp_selection == 8 || scalar_harp_selection == 9) {
        // Chord-based modes 8, 9: Use current_chord with chord scales
        uint8_t scale_index;
        bool use_pentatonic = (scalar_harp_selection == 9);
        if (current_chord == &major) {
            scale_index = use_pentatonic ? 0 : 10;
        } else if (current_chord == &maj_seventh) {
            scale_index = use_pentatonic ? 1 : 12;
        } else if (current_chord == &minor) {
            scale_index = use_pentatonic ? 2 : 14;
        } else if (current_chord == &seventh) {
            scale_index = use_pentatonic ? 3 : 13;
        } else if (current_chord == &min_seventh) {
            scale_index = use_pentatonic ? 4 : 11;
        } else if (current_chord == &dim) {
            scale_index = 5;
        } else if (current_chord == &aug) {
            scale_index = 6;
        } else if (current_chord == &maj_sixth) {
            scale_index = 7;
        } else if (current_chord == &min_sixth) {
            scale_index = 8;
        } else if (current_chord == &full_dim) {
            scale_index = 9;
        } else {
            scale_index = use_pentatonic ? 0 : 10;
            Serial.println("Unknown current_chord, defaulting to major scale");
        }
        uint8_t scale_length = chord_scale_lengths[scale_index];
        uint8_t octave = (string / scale_length) > 0 ? (string / scale_length) : 0;
        uint8_t scale_degree = string % scale_length;
        uint8_t root_note;
        if (slashed && note_slash_level == 0) {
            root_note = get_root_button(key_signature_selection, chord_frame_shift, slash_value);
            root_note += sharp * (flat_button_modifier ? -1 : 1);
        } else {
            root_note = get_root_button(key_signature_selection, chord_frame_shift, effective_fundamental);
            root_note += sharp * (flat_button_modifier ? -1 : 1);
        }
        uint8_t note = root_note + chord_scale_intervals[scale_index][scale_degree] + (octave * 12);
        Serial.printf("Chord scale mode %d, string %d, current_chord=%p, scale_index=%d, note=%d\n",
                      scalar_harp_selection, string, current_chord, scale_index, note);
        return note;
    } else if (chromatic_harp_mode) {
        // Chromatic mode
        uint8_t note = string + 24;
        Serial.printf("Chromatic mode, string %d, note=%d\n", string, note);
        return note;
    } else {
        // Mode 0 (or invalid mode): Use current_chord with harp_shuffling_array
        uint8_t note = 0;
        uint8_t level = harp_shuffling_array[harp_shuffling_selection][string];
        if (slashed && level % 10 == note_slash_level) {
            if (!flat_button_modifier) {
                note = (12 * int(level / 10) + get_root_button(key_signature_selection, chord_frame_shift, slash_value) + sharp * 1.0);
            } else {
                note = (12 * int(level / 10) + get_root_button(key_signature_selection, chord_frame_shift, slash_value) - sharp * 1.0);
            }
        } else {
            if (!flat_button_modifier) {
                note = (12 * int(level / 10) + get_root_button(key_signature_selection, chord_frame_shift, effective_fundamental) + sharp * 1.0 + (*current_chord)[level % 10]);
            } else {
                note = (12 * int(level / 10) + get_root_button(key_signature_selection, chord_frame_shift, effective_fundamental) - sharp * 1.0 + (*current_chord)[level % 10]);
            }
        }
        Serial.printf("Chord mode 0, string %d, current_chord=%p, fundamental=%d, note=%d\n",
                      string, current_chord, effective_fundamental, note);
        return note;
    }
}

void stop_all_note_timers() {
    for (int i = 0; i < 4; i++) {
        note_timer[i].end();
    }
}
//-->>RYTHM MODE UTILITIES
void rythm_tick_function() {
  //this function seems a bit long for a timed one. Maybe try to offload some logic somewhere else? 
  if (rythm_current_step % rythm_limit_change_to_every == 0) {
    for (int i = 0; i < 7; i++) {
      rythm_freeze_current_chord_notes[i] = current_applied_chord_notes[i];
    }
  }
  // handling the led pattern
  uint8_t active_modulus = 1;
  uint8_t possible_pattern[4] = {3, 2};
  for (uint8_t i = 0; i < sizeof(possible_pattern) / sizeof(uint8_t); i++) {
    if (rythm_loop_length % possible_pattern[i] == 0) {
      active_modulus = possible_pattern[i];
      break;
    }
  }
  analogWrite(RYTHM_LED_PIN, (220 * (rythm_current_step % rythm_limit_change_to_every == 0) + 15) * (rythm_current_step % active_modulus == 0));
  led_timer.priority(255);
  led_timer.begin([] { turn_off_led(&led_timer); }, 200000); 
  if (current_long_period) {
    rythm_timer.update(short_timer_period);
    current_long_period = false;
  } else {
    rythm_timer.update(long_timer_period);
    current_long_period = true;
  }
  u_int8_t result;
  result = rythm_pattern[rythm_current_step];
  for (int i = 6; i >= 0; i--) {
    if (result & (1 << i)) {
      int current_voice=0;
      if(i<4){
        current_voice=i;
      }else{
        current_voice=i-3;
      }
      set_chord_voice_frequency(current_voice, rythm_freeze_current_chord_notes[i]);
      play_note_selected_duration(current_voice, rythm_freeze_current_chord_notes[i]);
    }
  }
  rythm_current_step = (rythm_current_step + 1) % rythm_loop_length;
}

void recalculate_timer() {
  long_timer_period = shuffle * (60 * 1000 * 1000) / (2 * rythm_bpm);
  short_timer_period = 2 * (60 * 1000 * 1000) / (2 * rythm_bpm) - long_timer_period;
}

//--->>FILE HANDLING UTILITIES
String serialize(int16_t data_array[], u_int16_t array_size) {
  String dataString = "0,";
  dataString += String(current_bank_number); // to save the number of the bank for the online display
  dataString += ",";
  for (u_int16_t i = 2; i < array_size; i++) {
    dataString += String(data_array[i]);
    dataString += ",";
  }
  return dataString;
}

void deserialize(String input, int16_t data_array[]) {
  int len = input.length() + 1;
  char string[len];
  char *p;
  input.toCharArray(string, len);
  p = strtok(string, ",");
  int i = 0;
  while (p && i < parameter_size) {
    data_array[i] = atoi(p);
    p = strtok(NULL, ",");
    i++;
  }
}

void save_config(int bank_number, bool default_save) {
  digitalWrite(_MUTE_PIN, LOW); // muting the DAC
  current_bank_number=bank_number; //save to correctly write in the memory 
  AudioNoInterrupts();
  // myfs.quickFormat();  // performs a quick format of the created di
  myfs.remove(bank_name[bank_number]);
  File dataFile = myfs.open(bank_name[bank_number], FILE_WRITE);

  if (default_save) {
    // if we need to put the default in memory
    Serial.println("Writing the default file");
    Serial.println(bank_name[bank_number]);
    String return_data = serialize(default_bank_sysex_parameters[bank_number], parameter_size);
    dataFile.println(return_data);
  } else {
    Serial.println("Saving current settings");
    for (u_int16_t i = 0; i < parameter_size; i++) {
          Serial.println(current_sysex_parameters[i]);
    }
    dataFile.println(serialize(current_sysex_parameters, parameter_size));
  }
  Serial.print("Saved preset: ");
  Serial.println(dataFile.name());
  dataFile.close();

  load_config(current_bank_number); //we do a full reload to initialise values
  
  // add something to set config_bit in the parameters to zero
  AudioInterrupts();
  digitalWrite(_MUTE_PIN, HIGH); // unmuting the DAC
}

void load_config(int bank_number) {
  //digitalWrite(_MUTE_PIN, LOW); // muting the DAC
  //Turn off chords notes
  for (int i = 0; i < 4; i++) {
    chord_vibrato_envelope_array[i]->noteOff();
    chord_vibrato_dc_envelope_array[i]->noteOff();
    chord_envelope_array[i]->noteOff();
    chord_envelope_filter_array[i]->noteOff();
  }
  trigger_chord = true; //to be ready to retrigger if needed

  File entry = myfs.open(bank_name[bank_number]);
  if (entry) {
    String data_string = "";
    while (entry.available()) {
      data_string += char(entry.read());
    }
    deserialize(data_string, current_sysex_parameters);
    Serial.print("Loaded preset: ");
    Serial.println(entry.name());
    entry.close();
  } else {
    entry.close();
    Serial.print("No preset, writing factory default");
    save_config(bank_number, true); // reboot with default value
  }
  // Loading the potentiometer
  chord_pot.setup(chord_volume_sysex, 100, current_sysex_parameters[chord_pot_alternate_control], current_sysex_parameters[chord_pot_alternate_range], current_sysex_parameters,current_sysex_parameters[chord_pot_alternate_storage],apply_audio_parameter,chord_pot_alternate_storage);
  harp_pot.setup(harp_volume_sysex, 100, current_sysex_parameters[harp_pot_alternate_control], current_sysex_parameters[harp_pot_alternate_range], current_sysex_parameters,current_sysex_parameters[harp_pot_alternate_storage],apply_audio_parameter,harp_pot_alternate_storage);
  mod_pot.setup(current_sysex_parameters[mod_pot_main_control], current_sysex_parameters[mod_pot_main_range], current_sysex_parameters[mod_pot_alternate_control], current_sysex_parameters[mod_pot_alternate_range], current_sysex_parameters,current_sysex_parameters[mod_pot_alternate_storage],apply_audio_parameter,mod_pot_alternate_storage);
  Serial.println("pot setup done");
  for (int i = 1; i < parameter_size; i++) {
    apply_audio_parameter(i, current_sysex_parameters[i]);
  }
  control_command(0, 0); // tell itself to update the remote controller if present
  chord_pot.force_update();
  harp_pot.force_update();
  mod_pot.force_update();
  flag_save_needed=false;
  //digitalWrite(_MUTE_PIN, HIGH); // unmuting the DAC
}

void setup() {
    Serial.begin(115200);
    Serial.println("Initialising audio parameters");
    AudioMemory(1200);
    //>>STATIC AUDIO PARAMETERS
    // the waveshaper
    calculate_ws_array();
    chord_waveshape.shape(wave_shape, 257);
    string_waveshape.shape(wave_shape, 257);
    //the base DC value for strings
    filter_dc.amplitude(1);
    // the delay passthrough
    string_delay_mix.gain(0, 1);
    chord_delay_mix.gain(0, 1);
    // simple mixers
    string_vibrato_mixer.gain(0,0.5);
    string_vibrato_mixer.gain(1,0.5);
    envelope_string_vibrato_dc.sustain(0);
    for (int i = 0; i < 3; i++) {
        string_mixer_array[i]->gain(0, 1);
        string_mixer_array[i]->gain(1, 1);
        string_mixer_array[i]->gain(2, 1);
        string_mixer_array[i]->gain(3, 1);
        transient_mixer_array[i]->gain(0, 1);
        transient_mixer_array[i]->gain(1, 1);
        transient_mixer_array[i]->gain(2, 1);
        transient_mixer_array[i]->gain(3, 1);
    }
    for (int i = 0; i < 4; i++) {
        chord_voice_mixer_array[i]->gain(0, 1);
        chord_voice_mixer_array[i]->gain(1, 1);
        chord_voice_mixer_array[i]->gain(2, 1);
        chord_noise_array[i]->amplitude(0.5);
        chord_vibrato_mixer_array[i]->gain(0,0.5);
        chord_vibrato_mixer_array[i]->gain(1,0.5);
        chord_vibrato_dc_envelope_array[i]->sustain(0); //for the pitch bend no need for sustain
        transient_full_mix.gain(i, 1);
        all_string_mix.gain(i, 1);
    }
    for(int i=0;i<12;i++){
        string_transient_envelope_array[i]->sustain(0);//don't need sustain for the transient
    }
    all_string_mix.gain(3,0.02); //for the transient

    // initialising the rest of the hardware
    chord_matrix.setup();
    harp_sensor.setup();
    harp_sensor.recalibrate();
    pinMode(BATT_LBO_PIN, INPUT);
    pinMode(DOWN_PGM_PIN, INPUT);
    pinMode(UP_PGM_PIN, INPUT);
    pinMode(HOLD_BUTTON_PIN, INPUT);
    if (continuous_chord) {
        analogWrite(RYTHM_LED_PIN, 255);
    }
    // loading the preset
    Serial.println("Initialising filesystem");
    if (!myfs.begin(1024 * 1024)) { // Need to check that size
        Serial.printf("Error starting %s\n", "Program flash DISK");
        while (1) {
            set_led_color(0, 1.0, 1.0); // turn red light
        }
    }
    Serial.println("Loading the preset");
    load_config(current_bank_number);
    // initializing the strings
    for (int i = 0; i < 12; i++) {
        current_harp_notes[i] = calculate_note_harp(i, slash_chord, chord_sharpened);
    }
    //Checking the battery 
    LBO_flag.set(digitalRead(BATT_LBO_PIN));
    uint8_t LBO_value = LBO_flag.read_value();
    if (LBO_value == 0) {
        led_blinking_flag=true;
    }

    Serial.println("Initialisation complete");
    digitalWrite(_MUTE_PIN, HIGH);
}


// Helper functions (unchanged from previous version)
void updateNotes(bool withSlashChord, bool isSharpened) {
    for (int i = 0; i < 7; i++) {
        current_chord_notes[i] = calculate_note_chord(i, withSlashChord, isSharpened);
        current_applied_chord_notes[i] = current_chord_notes[i];
    }
    for (int i = 0; i < 12; i++) {
        current_harp_notes[i] = calculate_note_harp(i, withSlashChord, isSharpened);
        if (change_held_strings && string_enveloppe_array[i]->isSustain()) {
            if (harp_started_notes[i] != 0) {
                usbMIDI.sendNoteOff(harp_started_notes[i], harp_release_velocity, 1, harp_port);
                harp_started_notes[i] = 0;
                usbMIDI.sendNoteOn(midi_base_note_transposed + current_harp_notes[i], harp_attack_velocity, 1, harp_port);
                harp_started_notes[i] = midi_base_note_transposed + current_harp_notes[i];
            }
            set_harp_voice_frequency(i, current_harp_notes[i]);
        }
    }
}

void triggerChord(bool retrigger) {
    AudioNoInterrupts();
    if (retrigger) {
        stop_all_note_timers();
        for (int i = 0; i < 4; i++) {
            set_chord_voice_frequency(i, current_chord_notes[i]);
            chord_vibrato_envelope_array[i]->noteOn();
            chord_vibrato_dc_envelope_array[i]->noteOn();
            chord_envelope_array[i]->noteOn();
            chord_envelope_filter_array[i]->noteOn();
            if (chord_started_notes[i] != 0) {
                usbMIDI.sendNoteOff(chord_started_notes[i], chord_release_velocity, 1, chord_port);
                chord_started_notes[i] = 0;
            }
            usbMIDI.sendNoteOn(midi_base_note_transposed + current_chord_notes[i], chord_attack_velocity, 1, chord_port);
            chord_started_notes[i] = midi_base_note_transposed + current_chord_notes[i];
        }
    } else {
        for (int i = 0; i < 4; i++) {
            set_chord_voice_frequency(i, current_chord_notes[i]);
        }
    }
    AudioInterrupts();
}

void stopChord() {
    AudioNoInterrupts();
    for (int i = 0; i < 4; i++) {
        if (chord_envelope_array[i]->isActive()) {
            chord_vibrato_envelope_array[i]->noteOff();
            chord_vibrato_dc_envelope_array[i]->noteOff();
            chord_envelope_array[i]->noteOff();
            chord_envelope_filter_array[i]->noteOff();
            if (chord_started_notes[i] != 0) {
                usbMIDI.sendNoteOff(chord_started_notes[i], chord_release_velocity, 1, chord_port);
                chord_started_notes[i] = 0;
            }
        }
    }
    AudioInterrupts();
}

void handleKeySignatureChange() {
  up_button.set(digitalRead(UP_PGM_PIN));
  down_button.set(digitalRead(DOWN_PGM_PIN));
  bool up_held = up_button.read_value();
  bool down_held = down_button.read_value();
  bool preset_combo = up_held && down_held; // Require both Up and Down

  // Compute any_chord_pressed locally
  bool any_chord_pressed = false;
  for (int i = 1; i < 22; i++) { // Check all chord buttons
    if (chord_matrix_array[i].read_value()) {
      any_chord_pressed = true;
      break;
    }
  }

  if (!preset_combo || !any_chord_pressed) return;

  // Map button indices (1–21) to KeySig and key_names index
  const struct {
    int8_t key_sig; // KeySig enum value
    int8_t key_name_idx; // Index into key_names array
  } button_key_map[22] = {
    {-1, -1}, // Button 0 (unused, sharp button)
    {KEY_SIG_Bsharp, 18}, // Button 1: B Top (B#)
    {KEY_SIG_B, 5},      // Button 2: B Middle (B)
    {KEY_SIG_Bb, 7},     // Button 3: B Bottom (Bb)
    {KEY_SIG_Esharp, 14}, // Button 4: E Top (E#)
    {KEY_SIG_E, 4},      // Button 5: E Middle (E)
    {KEY_SIG_Eb, 8},     // Button 6: E Bottom (Eb)
    {KEY_SIG_Asharp, 17}, // Button 7: A Top (A#)
    {KEY_SIG_A, 3},      // Button 8: A Middle (A)
    {KEY_SIG_Ab, 9},     // Button 9: A Bottom (Ab)
    {KEY_SIG_Dsharp, 13}, // Button 10: D Top (D#)
    {KEY_SIG_D, 2},      // Button 11: D Middle (D)
    {KEY_SIG_Db, 10},    // Button 12: D Bottom (Db)
    {KEY_SIG_Gsharp, 16}, // Button 13: G Top (G#)
    {KEY_SIG_G, 1},      // Button 14: G Middle (G)
    {KEY_SIG_Gb, 11},    // Button 15: G Bottom (Gb)
    {KEY_SIG_Csharp, 12}, // Button 16: C Top (C#)
    {KEY_SIG_C, 0},      // Button 17: C Middle (C)
    {KEY_SIG_Cb, 20},    // Button 18: C Bottom (Cb)
    {KEY_SIG_Fsharp, 15}, // Button 19: F Top (F#)
    {KEY_SIG_F, 6},      // Button 20: F Middle (F)
    {KEY_SIG_Fb, 19}     // Button 21: F Bottom (Fb)
  };

  for (int i = 1; i < 22; i++) { // Check chord buttons (1–21)
    bool button_pressed = (chord_matrix_array[i].read_transition() > 1);
    if (!button_pressed) {
      chord_matrix.update(chord_matrix_array);
      button_pressed = (chord_matrix_array[i].read_transition() > 1 || chord_matrix_array[i].read_value());
    }

    if (button_pressed) {
      elapsedMillis hold_timer = 0;
      bool still_held = chord_matrix_array[i].read_value();
      bool up_held_confirmed = up_held;
      bool down_held_confirmed = down_held;

      // Confirm hold for 50ms
      while (hold_timer < 50 && still_held) {
        up_button.set(digitalRead(UP_PGM_PIN));
        down_button.set(digitalRead(DOWN_PGM_PIN));
        chord_matrix.update(chord_matrix_array);
        up_held_confirmed = up_held_confirmed && up_button.read_value();
        down_held_confirmed = down_held_confirmed && down_button.read_value();
        still_held = chord_matrix_array[i].read_value();
      }

      // Debug output
      int row = (i - 1) % 3; // 0=Top, 1=Middle, 2=Bottom
      int column = (i - 1) / 3; // Hardware column index
      Serial.print("Button detected: "); Serial.print(i);
      Serial.print(" (still held: "); Serial.print(still_held);
      Serial.print(" up_held: "); Serial.print(up_held_confirmed);
      Serial.print(" down_held: "); Serial.print(down_held_confirmed);
      Serial.print(" row: "); Serial.print(row == 0 ? "Top" : row == 1 ? "Middle" : "Bottom");
      Serial.print(" column: "); Serial.print(column_names[column]); Serial.println(")");

      if (!still_held || !up_held_confirmed || !down_held_confirmed) continue;

      // Get key signature from mapping
      if (button_key_map[i].key_sig == -1) continue; // Invalid button

      // Apply key signature
      key_signature_selection = button_key_map[i].key_sig;
      apply_audio_parameter(35, button_key_map[i].key_sig); // Use KeySig index
      Serial.print("Applied key signature: "); Serial.print(key_names[button_key_map[i].key_name_idx]);
      Serial.print(" (KeySig: "); Serial.print(button_key_map[i].key_sig); Serial.println(")");

      // Calculate LED hue (based on key_name_idx, 0–20, mapped to 0–360 degrees)
      float key_hue = (float)button_key_map[i].key_name_idx / 21.0 * 360.0;
      set_led_color(key_hue, 1.0, 1.0);

      // Blink LED
      color_led_blink_timer.begin([] {
        set_led_color(bank_led_hue, 1.0, 1.0 - led_attenuation);
        color_led_blink_timer.end();
      }, 200000);

      // Stop active chords
      AudioNoInterrupts();
      for (int j = 0; j < 4; j++) {
        if (chord_envelope_array[j]->isSustain()) {
          chord_vibrato_envelope_array[j]->noteOff();
          chord_vibrato_dc_envelope_array[j]->noteOff();
          chord_envelope_array[j]->noteOff();
          chord_envelope_filter_array[j]->noteOff();
          if (chord_started_notes[j] != 0) {
            usbMIDI.sendNoteOff(chord_started_notes[j], chord_release_velocity, 1, chord_port);
            chord_started_notes[j] = 0;
          }
        }
      }
      AudioInterrupts();

      // Reset state
      current_line = -1;
      trigger_chord = true;
      button_pushed = false;
      inhibit_button = false;
      if (!sysex_controler_connected) {
        flag_save_needed = true;
      }

      // Update harp notes to reflect new key signature
      updateNotes(slash_chord, chord_sharpened);
    }

    if (chord_matrix_array[i].read_transition() == 1) {
      inhibit_button = false;
    }
  }
}

void handleChordButtons() {
      // Check for key signature change first
  handleKeySignatureChange();
    // Node A: Start checking chord buttons
    // Node B: Read button states
    chord_matrix.update(chord_matrix_array);

    // Node C: Log settings every half-second
/*     static unsigned long last_log = 0;
    if (millis() - last_log > 500) {
        Serial.print("Preset: continuous="); Serial.print(continuous_chord);
        Serial.print(", rhythm="); Serial.print(rythm_mode);
        Serial.print(", trigger="); Serial.print(trigger_chord);
        Serial.print(", retrigger="); Serial.print(retrigger_chord);
        Serial.print(", scalar_harp_selection="); Serial.print(scalar_harp_selection);
        Serial.print(", current_chord=");
        Serial.println((current_chord == &major) ? "major" : (current_chord == &minor) ? "minor" : (current_chord == &seventh) ? "seventh" : (current_chord == &maj_seventh) ? "maj_seventh" : (current_chord == &min_seventh) ? "min_seventh" : (current_chord == &dim) ? "dim" : (current_chord == &full_dim) ? "full_dim" : (current_chord == &aug) ? "aug" : (current_chord == &maj_sixth) ? "maj_sixth" : (current_chord == &min_sixth) ? "min_sixth" : "unknown");
        last_log = millis();
    } */

    // Node D: Sharp button just pressed while a chord is active?
    bool sharp_active = chord_matrix_array[0].read_value();
    int sharp_transition = chord_matrix_array[0].read_transition();
    if (sharp_transition == 2 && current_line >= 0) {
        // Node E: Apply sharp, update notes, adjust sound, log
        chord_sharpened = true;
        Serial.println("Sharp button pressed, sharpening current chord");
        if (scalar_harp_selection == 0) {
            change_held_strings = true; // Update sustained harp notes
        }
        updateNotes(slash_chord, chord_sharpened);
        triggerChord(retrigger_chord);
        if (retrigger_chord) {
            Serial.println("Retriggered chord with sharp");
        } else {
            Serial.println("Updated chord frequencies with sharp");
        }
    }

    // Node F: Normal mode (not holding chord or playing rhythm)?
    if (!continuous_chord && !rythm_mode) {
        bool new_press = false;
        int active_button_count = 0;
        // Node G: Look for new button presses
        // Node H: Any button in grid just pressed?
        for (int i = 1; i < 22; i++) {
            int value = chord_matrix_array[i].read_transition();
            if (value > 1) {
                // Node I: Mark button, flag new press, pick row, log
                new_press = true;
                chord_buttons_pressed[i] = true;
                active_button_count++;
                int line = (i - 1) / 3;
                if (active_line == -1) {
                    active_line = line;
                }
                Serial.print("Button pressed: i="); Serial.print(i); Serial.print(", row="); Serial.println(line);

                // Handle slash chord only if not in active row
                if (current_line >= 0 && line != current_line) {
                    slash_chord = true;
                    slash_value = line;
                    Serial.print("Slash chord applied, bass row: "); Serial.println(slash_value);
                    if (scalar_harp_selection == 0) {
                        change_held_strings = true; // Update sustained harp notes
                    }
                    updateNotes(slash_chord, chord_sharpened);
                    triggerChord(retrigger_chord);
                    if (retrigger_chord) {
                        Serial.println("Retriggered chord with slash");
                    } else {
                        Serial.println("Updated chord frequencies with slash");
                    }
                    button_pushed = false;
                }
            }
        }

        // Node J: New press, no invalid buttons, no chord window, no chord active?
        if (new_press && !inhibit_button && !chord_window_active && current_line == -1) {
            // Node K: Start 50ms chord window, reset timer, log
            chord_window_active = true;
            chord_window_timer = 0;
            Serial.println("Chord window started");
        }

        // Node L: Too many buttons pressed in one column?
        int line_accumulator[3] = {0, 0, 0};
        for (int i = 1; i < 22; i++) {
            if (chord_matrix_array[i].read_value()) {
                line_accumulator[i % 3]++;
            }
        }
        if (line_accumulator[0] > 2 || line_accumulator[1] > 2 || line_accumulator[2] > 2) {
            // Node M: Clear active chord, block buttons, clear states, log
            current_line = -1;
            inhibit_button = true;
            chord_window_active = false;
            chord_window_processed = false;
            memset(chord_buttons_pressed, 0, sizeof(chord_buttons_pressed));
            active_line = -1;
            slash_chord = false;
            slash_value = -1;
            chord_sharpened = false;
            if (scalar_harp_selection == 0) {
                change_held_strings = true; // Update sustained harp notes
            }
            Serial.println("Invalid input: too many buttons in a column");
            updateNotes(slash_chord, chord_sharpened);
            return;
        } else {
            // Node N: Allow button processing
            inhibit_button = false;
        }

        // Node O: Main chord buttons released while chord active?
        bool main_chord_active = false;
        if (current_line >= 0) {
            for (int i = 1 + current_line * 3; i <= 3 + current_line * 3; i++) {
                main_chord_active |= chord_matrix_array[i].read_value();
            }
        }
        if (!main_chord_active && current_line >= 0) {
            // Node P: Stop all MIDI notes, clear states, log
            Serial.println("Chord buttons released, stopping chord");
            stopChord();
            current_line = -1;
            chord_window_active = false;
            chord_window_processed = false;
            memset(chord_buttons_pressed, 0, sizeof(chord_buttons_pressed));
            active_line = -1;
            slash_chord = false;
            slash_value = -1;
            chord_sharpened = false;
            if (scalar_harp_selection == 0) {
                change_held_strings = true; // Update sustained harp notes
            }
            Serial.println("Chord states reset");
            updateNotes(slash_chord, chord_sharpened);
        }

        // Node Q: Chord type change (chord active, no chord window)?
        if (current_line >= 0 && !chord_window_active) {
            // Node R: New button pressed or held in active chord row?
            bool chord_type_changed = false;
            for (int i = 1 + current_line * 3; i <= 3 + current_line * 3; i++) {
                if (chord_matrix_array[i].read_transition() > 1 || (chord_matrix_array[i].read_value() && !chord_buttons_pressed[i])) {
                    chord_buttons_pressed[i] = true;
                    chord_type_changed = true;
                }
            }
            if (chord_type_changed) {
                // Node S: Update chord type, notes, sound, log
                bool button_maj = chord_matrix_array[1 + current_line * 3].read_value();
                bool button_min = chord_matrix_array[2 + current_line * 3].read_value();
                bool button_seventh = chord_matrix_array[3 + current_line * 3].read_value();
                current_chord = barry_harris_mode ? &maj_sixth : &major; // Default
                if (button_maj && !button_min && !button_seventh) {
                    current_chord = barry_harris_mode ? &maj_sixth : &major;
                } else if (!button_maj && button_min && !button_seventh) {
                    current_chord = barry_harris_mode ? &min_sixth : &minor;
                } else if (!button_maj && !button_min && button_seventh) {
                    current_chord = &seventh;
                } else if (button_maj && !button_min && button_seventh) {
                    current_chord = &maj_seventh;
                } else if (!button_maj && button_min && button_seventh) {
                    current_chord = &min_seventh;
                } else if (button_maj && button_min && !button_seventh) {
                    current_chord = barry_harris_mode ? &full_dim : &dim;
                } else if (button_maj && button_min && button_seventh) {
                    current_chord = &aug;
                }
                if (scalar_harp_selection == 0) {
                    change_held_strings = true; // Update sustained harp notes
                }
                Serial.print("Chord type updated: "); 
                Serial.println((current_chord == &major) ? "major" : (current_chord == &minor) ? "minor" : (current_chord == &seventh) ? "seventh" : (current_chord == &maj_seventh) ? "maj_seventh" : (current_chord == &min_seventh) ? "min_seventh" : (current_chord == &dim) ? "dim" : (current_chord == &full_dim) ? "full_dim" : (current_chord == &aug) ? "aug" : "unknown");
                updateNotes(slash_chord, chord_sharpened);
                triggerChord(retrigger_chord);
            }
        }

        // Node T: Chord window active, 50ms passed, not processed?
        if (chord_window_active && chord_window_timer >= CHORD_WINDOW_MS && !chord_window_processed) {
            // Node U: Collect pressed buttons, identify rows
            bool main_chord_press = false;
            slash_chord = false;
            slash_value = -1;
            int detected_lines[7] = {-1, -1, -1, -1, -1, -1, -1};
            int line_count = 0;
            Serial.println("Processing chord window:");
            for (int i = 1; i < 22; i++) {
                if (chord_buttons_pressed[i]) {
                    int line = (i - 1) / 3;
                    bool line_already_detected = false;
                    for (int j = 0; j < line_count; j++) {
                        if (detected_lines[j] == line) {
                            line_already_detected = true;
                            break;
                        }
                    }
                    if (!line_already_detected) {
                        detected_lines[line_count++] = line;
                        Serial.print("Detected row: "); Serial.println(line);
                    }
                    if (line == active_line && (i == 1 + active_line * 3 || i == 2 + active_line * 3 || i == 3 + active_line * 3)) {
                        main_chord_press = true;
                    }
                    if (active_line != -1 && line != active_line && !slash_chord) {
                        slash_chord = true;
                        slash_value = line;
                        Serial.print("Slash chord detected, bass row: "); Serial.println(slash_value);
                    }
                }
            }

            // Node V: Valid chord row and no invalid buttons?
            if (active_line != -1 && !inhibit_button) {
                // Node W: Set chord, apply sharp/bass, play chord, log
                current_line = active_line;
                fundamental = current_line;
                chord_sharpened = sharp_active;
                bool button_maj = chord_buttons_pressed[1 + current_line * 3];
                bool button_min = chord_buttons_pressed[2 + current_line * 3];
                bool button_seventh = chord_buttons_pressed[3 + current_line * 3];
                current_chord = barry_harris_mode ? &maj_sixth : &major; // Default
                if (button_maj && !button_min && !button_seventh) {
                    current_chord = barry_harris_mode ? &maj_sixth : &major;
                } else if (!button_maj && button_min && !button_seventh) {
                    current_chord = barry_harris_mode ? &min_sixth : &minor;
                } else if (!button_maj && !button_min && button_seventh) {
                    current_chord = &seventh;
                } else if (button_maj && !button_min && button_seventh) {
                    current_chord = &maj_seventh;
                } else if (!button_maj && button_min && button_seventh) {
                    current_chord = &min_seventh;
                } else if (button_maj && button_min && !button_seventh) {
                    current_chord = barry_harris_mode ? &full_dim : &dim;
                } else if (button_maj && button_min && button_seventh) {
                    current_chord = &aug;
                }
                if (scalar_harp_selection == 0) {
                    change_held_strings = true; // Update sustained harp notes
                }
                Serial.print("Chord set: "); 
                Serial.println((current_chord == &major) ? "major" : (current_chord == &minor) ? "minor" : (current_chord == &seventh) ? "seventh" : (current_chord == &maj_seventh) ? "maj_seventh" : (current_chord == &min_seventh) ? "min_seventh" : (current_chord == &dim) ? "dim" : (current_chord == &full_dim) ? "full_dim" : (current_chord == &aug) ? "aug" : "unknown");
                updateNotes(slash_chord, chord_sharpened);
                triggerChord(true);
                Serial.println("Chord played");
            } else {
                // Node X: Clear active chord, reset states
                Serial.println("No valid chord detected");
                current_line = -1;
                active_line = -1;
                slash_chord = false;
                slash_value = -1;
                chord_sharpened = false;
                if (scalar_harp_selection == 0) {
                    change_held_strings = true; // Update sustained harp notes
                }
                Serial.println("Chord states reset");
                updateNotes(slash_chord, chord_sharpened);
            }

            // Node Y: Close chord window, reset states, log
            chord_window_active = false;
            chord_window_processed = false;
            if (current_line == -1) {
                memset(chord_buttons_pressed, 0, sizeof(chord_buttons_pressed));
            }
            Serial.println("Chord window processed");
        }
    } else {
        // Continuous or rhythm mode
        static unsigned long last_slash_log = 0;
        if (current_line >= 0) {
            fundamental = current_line;
            slash_chord = false;
            slash_value = -1;
            for (int i = 1; i < 22; i++) {
                if (chord_matrix_array[i].read_value()) {
                    int slash_line = (i - 1) / 3;
                    if (slash_line != current_line) {
                        slash_chord = true;
                        slash_value = slash_line;
                        if (millis() - last_slash_log > 500) {
                            Serial.print("Slash chord detected, bass row: "); Serial.println(slash_value);
                            if (scalar_harp_selection == 0) {
                                change_held_strings = true; // Update sustained harp notes
                            }
                            last_slash_log = millis();
                        }
                        break;
                    }
                }
            }
            // Check for chord type (extension) changes in current row
            bool chord_type_changed = false;
            for (int i = 1 + current_line * 3; i <= 3 + current_line * 3; i++) {
                if (chord_matrix_array[i].read_transition() > 1 || (chord_matrix_array[i].read_value() && !chord_buttons_pressed[i])) {
                    chord_buttons_pressed[i] = true;
                    chord_type_changed = true;
                }
            }
            bool button_maj = chord_matrix_array[1 + current_line * 3].read_value();
            bool button_min = chord_matrix_array[2 + current_line * 3].read_value();
            bool button_seventh = chord_matrix_array[3 + current_line * 3].read_value();
            // Only stop chord in rhythm mode or if explicitly requested
            if (!(button_maj || button_min || button_seventh) && rythm_mode) {
                Serial.println("No chord buttons active in rhythm mode, stopping chord");
                stopChord();
                current_line = -1;
                chord_sharpened = false;
                slash_chord = false;
                slash_value = -1;
                if (scalar_harp_selection == 0) {
                    change_held_strings = true; // Update sustained harp notes
                }
                Serial.println("Chord states reset");
                updateNotes(slash_chord, chord_sharpened);
            } else {
                // Update chord type if changed or buttons are active
                if (chord_type_changed || button_maj || button_min || button_seventh) {
                    current_chord = barry_harris_mode ? &maj_sixth : &major; // Default
                    if (button_maj && !button_min && !button_seventh) {
                        current_chord = barry_harris_mode ? &maj_sixth : &major;
                    } else if (!button_maj && button_min && !button_seventh) {
                        current_chord = barry_harris_mode ? &min_sixth : &minor;
                    } else if (!button_maj && !button_min && button_seventh) {
                        current_chord = &seventh;
                    } else if (button_maj && !button_min && button_seventh) {
                        current_chord = &maj_seventh;
                    } else if (!button_maj && button_min && button_seventh) {
                        current_chord = &min_seventh;
                    } else if (button_maj && button_min && !button_seventh) {
                        current_chord = barry_harris_mode ? &full_dim : &dim;
                    } else if (button_maj && button_min && button_seventh) {
                        current_chord = &aug;
                    }
                    if (chord_type_changed) {
                        if (scalar_harp_selection == 0) {
                            change_held_strings = true; // Update sustained harp notes
                        }
                        Serial.print("Chord extension updated: ");
                        Serial.println((current_chord == &major) ? "major" : (current_chord == &minor) ? "minor" : (current_chord == &seventh) ? "seventh" : (current_chord == &maj_seventh) ? "maj_seventh" : (current_chord == &min_seventh) ? "min_seventh" : (current_chord == &dim) ? "dim" : (current_chord == &full_dim) ? "full_dim" : (current_chord == &aug) ? "aug" : "unknown");
                        updateNotes(slash_chord, chord_sharpened);
                        triggerChord(retrigger_chord);
                    }
                }
                if ((button_pushed || sharp_transition == 2) && (button_maj || button_min || button_seventh)) {
                    Serial.println("Updating chord in continuous/rhythm mode");
                    triggerChord(!rythm_mode && !trigger_chord && !retrigger_chord);
                }
                if ((trigger_chord || (button_pushed && retrigger_chord)) && !rythm_mode) {
                    Serial.println("Triggering chord in continuous mode");
                    for (int i = 0; i < 4; i++) {
                        note_timer[i].priority(253);
                        note_timer[i].begin([i] { play_single_note(i, &note_timer[i]); }, 10 + chord_retrigger_release * 1000 + i * inter_string_delay + random(random_delay));
                    }
                    trigger_chord = false;
                }
            }
        }
        // Handle new button presses to start or change a chord
        bool new_chord_triggered = false;
        for (int i = 1; i < 22; i++) {
            if (chord_matrix_array[i].read_transition() > 1 && !inhibit_button) {
                button_pushed = true;
                Serial.print("Pushed button: "); Serial.println(i);
                int new_line = (i - 1) / 3;
                if (current_line == -1 || new_line != current_line) {
                    // Start new chord
                    current_line = new_line;
                    chord_sharpened = sharp_active;
                    new_chord_triggered = true;
                    Serial.print("New chord started, row: "); Serial.println(current_line);
                    if (!continuous_chord) {
                        trigger_chord = true;
                    }
                }
            }
        }
        // Trigger new chord if detected
        if (new_chord_triggered && (continuous_chord || rythm_mode)) {
            bool button_maj = chord_matrix_array[1 + current_line * 3].read_value();
            bool button_min = chord_matrix_array[2 + current_line * 3].read_value();
            bool button_seventh = chord_matrix_array[3 + current_line * 3].read_value();
            current_chord = barry_harris_mode ? &maj_sixth : &major; // Default
            if (button_maj && !button_min && !button_seventh) {
                current_chord = barry_harris_mode ? &maj_sixth : &major;
            } else if (!button_maj && button_min && !button_seventh) {
                current_chord = barry_harris_mode ? &min_sixth : &minor;
            } else if (!button_maj && !button_min && button_seventh) {
                current_chord = &seventh;
            } else if (button_maj && !button_min && button_seventh) {
                current_chord = &maj_seventh;
            } else if (!button_maj && button_min && button_seventh) {
                current_chord = &min_seventh;
            } else if (button_maj && button_min && !button_seventh) {
                current_chord = barry_harris_mode ? &full_dim : &dim;
            } else if (button_maj && button_min && button_seventh) {
                current_chord = &aug;
            }
            if (scalar_harp_selection == 0) {
                change_held_strings = true; // Update sustained harp notes
            }
            Serial.print("New chord triggered: ");
            Serial.println((current_chord == &major) ? "major" : (current_chord == &minor) ? "minor" : (current_chord == &seventh) ? "seventh" : (current_chord == &maj_seventh) ? "maj_seventh" : (current_chord == &min_seventh) ? "min_seventh" : (current_chord == &dim) ? "dim" : (current_chord == &full_dim) ? "full_dim" : (current_chord == &aug) ? "aug" : "unknown");
            updateNotes(slash_chord, chord_sharpened);
            triggerChord(true);
        }
        button_pushed = false;
    }
    // Node Z: Finish
}

void handleHarp() {
    harp_sensor.update(harp_array);
    for (int i = 0; i < 12; i++) {
        int value = harp_array[i].read_transition();
        if (value == 2) {
            set_harp_voice_frequency(i, current_harp_notes[i]);
            AudioNoInterrupts();
            envelope_string_vibrato_lfo.noteOn();
            envelope_string_vibrato_dc.noteOn();
            string_enveloppe_filter_array[i]->noteOn();
            string_enveloppe_array[i]->noteOn();
            string_transient_envelope_array[i]->noteOn();
            AudioInterrupts();
            if (harp_started_notes[i] != 0) {
                usbMIDI.sendNoteOff(harp_started_notes[i], harp_release_velocity, 1, harp_port);
                harp_started_notes[i] = 0;
            }
            usbMIDI.sendNoteOn(midi_base_note_transposed + current_harp_notes[i], harp_attack_velocity, 1, harp_port);
            harp_started_notes[i] = midi_base_note_transposed + current_harp_notes[i];
        } else if (value == 1) {
            AudioNoInterrupts();
            string_enveloppe_array[i]->noteOff();
            string_transient_envelope_array[i]->noteOff();
            string_enveloppe_filter_array[i]->noteOff();
            AudioInterrupts();
            if (harp_started_notes[i] != 0) {
                usbMIDI.sendNoteOff(harp_started_notes[i], harp_release_velocity, 1, harp_port);
                harp_started_notes[i] = 0;
            }
        }
    }
}

void updatePotentiometers(bool alternate) {
    flag_save_needed = chord_pot.update_parameter(alternate) || flag_save_needed;
    flag_save_needed = harp_pot.update_parameter(alternate) || flag_save_needed;
    flag_save_needed = mod_pot.update_parameter(alternate) || flag_save_needed;
}

void handleRhythmMode() {
    if (rythm_mode) {
        for (int i = 0; i < 4; i++) {
            if (note_off_timing[i] > note_pushed_duration && chord_envelope_array[i]->isSustain()) {
                chord_vibrato_envelope_array[i]->noteOff();
                chord_vibrato_dc_envelope_array[i]->noteOff();
                chord_envelope_array[i]->noteOff();
                chord_envelope_filter_array[i]->noteOff();
                if (chord_started_notes[i] != 0) {
                    usbMIDI.sendNoteOff(chord_started_notes[i], chord_release_velocity, 1, chord_port);
                    chord_started_notes[i] = 0;
                }
            }
        }
    }
}






void loop() {
    // Handle incoming MIDI messages
    if (usbMIDI.read()) {
        processMIDI();
    }

    // Check Sysex controller connection
    if (sysex_controler_connected && (USB1_PORTSC1, 7)) {
        sysex_controler_connected = false;
    }

    // Update debouncers
    hold_button.set(digitalRead(HOLD_BUTTON_PIN));
    up_button.set(digitalRead(UP_PGM_PIN));
    down_button.set(digitalRead(DOWN_PGM_PIN));
    LBO_flag.set(digitalRead(BATT_LBO_PIN));
    chord_matrix.update(chord_matrix_array);

    // Handle low battery indicator
    uint8_t LBO_transition = LBO_flag.read_transition();
    if (LBO_transition == 1) {
        led_blinking_flag = true;
    } else if (LBO_transition == 2) {
        led_blinking_flag = false;
        set_led_color(bank_led_hue, 1.0, 1 - led_attenuation);
    }
    if (led_blinking_flag) {
        set_led_color(bank_led_hue, 1.0, 0.6 + 0.4 * sin(color_led_blink_val));
        color_led_blink_val += 0.005;
    }

    // Handle hold button functions
    uint8_t hold_transition = hold_button.read_transition();
    if (hold_transition == 2) {
        if (!rythm_mode) {
            Serial.println("Switching mode");
            bool prev_continuous_chord = continuous_chord;
            continuous_chord = !continuous_chord;
            analogWrite(RYTHM_LED_PIN, 255 * continuous_chord);
            if (current_line == -1) {
                trigger_chord = true;
            }
            // Stop chord when turning off continuous mode
            if (prev_continuous_chord && !continuous_chord) {
                Serial.println("Stopping chord on continuous mode disable");
                AudioNoInterrupts();
                for (int i = 0; i < 4; i++) {
                    if (chord_envelope_array[i]->isActive()) {
                        chord_vibrato_envelope_array[i]->noteOff();
                        chord_vibrato_dc_envelope_array[i]->noteOff();
                        chord_envelope_array[i]->noteOff();
                        chord_envelope_filter_array[i]->noteOff();
                        if (chord_started_notes[i] != 0) {
                            usbMIDI.sendNoteOff(chord_started_notes[i], chord_release_velocity, 1, chord_port);
                            chord_started_notes[i] = 0;
                        }
                    }
                }
                stop_all_note_timers(); // Stop any pending note timers
                AudioInterrupts();
                current_line = -1;
                chord_window_active = false;
                chord_window_processed = false;
                memset(chord_buttons_pressed, 0, sizeof(chord_buttons_pressed));
                slash_chord = false;
                slash_value = 0;
            }
        } else {
            if (since_last_button_push > 100 && since_last_button_push < 2000) {
                rythm_bpm = (rythm_bpm * 5.0 + 60 * 1000 / since_last_button_push) / 6.0;
                Serial.print("Updating the BPM to:");
                Serial.println(rythm_bpm);
                recalculate_timer();
                if (current_long_period) {
                    rythm_timer.update(long_timer_period);
                } else {
                    rythm_timer.update(short_timer_period);
                }
            }
        }
        since_last_button_push = 0;
    }
    if (hold_transition == 1) {
        if (since_last_button_push > 800) {
            Serial.println("Long push, switching rhythm mode");
            rythm_mode = !rythm_mode;
            continuous_chord = false;
            analogWrite(RYTHM_LED_PIN, 255 * continuous_chord);
            if (rythm_mode) {
                rythm_current_step = 0;
                Serial.println("Starting rhythm timers");
                rythm_timer.priority(254);
                rythm_timer.begin(rythm_tick_function, short_timer_period);
                rythm_timer_running = true;
                rythm_timer.update(long_timer_period);
                current_long_period = true;
            } else {
                Serial.println("Stopping rhythm timers");
                rythm_timer.end();
                rythm_timer_running = false;
            }
        }
    }

    // Handle preset changes
    if (up_button.read_transition() > 1) {
        Serial.println("Switching to next preset");
        if (!sysex_controler_connected && flag_save_needed) {
            save_config(current_bank_number, false);
        }
        current_bank_number = (current_bank_number + 1) % 12;
        load_config(current_bank_number);
    }
    if (down_button.read_transition() > 1) {
        Serial.println("Switching to last preset");
        if (!sysex_controler_connected && flag_save_needed) {
            save_config(current_bank_number, false);
        }
        current_bank_number = (current_bank_number - 1 + 12) % 12;
        load_config(current_bank_number);
    }

    // Call helper functions
    handleRhythmMode();
    bool alternate = chord_matrix_array[0].read_value();
    updatePotentiometers(alternate);
    handleChordButtons();
    handleHarp();
}
