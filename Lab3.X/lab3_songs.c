
/*  Scoring for three songs
 *  Author: Matthew Watkins
 *
 *  *_base_dur : contains duration in ms of base note for song
 *  *_note1[]  : contains notes for top line of song. Notes assume macros define appropriate value for note
 *               example for macro:  #define C4 200 //replace 200 with appropriate value for frequency generation
 *               a macro with form xxS indicates node xx sharp (ex., C4S is C4#)
 *  *_dur1[]   : duration of each note in _note1
 *               ex., a _base_dur of 500 and a _dur1 of 2 would mean that the note should play for 1000 ms
 *  *_note2[]  : similar to note1 but for second line of song
 *  *_dur2p[]  : duration of each note in _note2
 *
 *  End of song indicated by a 0 duration in *_dur1[]
 */

//#include <stdint.h>
//#include "notes.h"
// Twinkle Twinkle
uint16_t twinkle_base_dur = 500; //duration in ms for base note
uint32_t twinkle_note1[] = {C4, C4, G4, G4, A4, A4, G4, F4, F4, E4, E4, D4, D4, C4, G4, G4, F4, F4, E4, E4, D4, G4, G4, F4, F4, E4, E4, D4,
                            C4, C4, G4, G4, A4, A4, G4, F4, F4, E4, E4, D4, D4, C4};
uint8_t twinkle_dur1[] = {1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 0};
uint32_t twinkle_note2[] = {C3, E3, F3, E3, D3, C3, F3, G3, E3, C3, E3, F3, G3, G3, E3, F3, G3, G3, C3, E3, F3, E3, D3, C3, F3, G3, C3};
uint8_t twinkle_dur2[] = {2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 0}; //0 ends song

// Super Mario
uint16_t mario_base_dur = 150; //150 //duration in ms for base note
uint32_t mario_note1[] = {E5, E5, R, E5, R, C5, E5, R, G5, R, G4, R, C5, R, G4, R, E4, R, A4, R, B4, R, A4S, A4, R,
                          G4, E5, G5, A5, R, F5, G5, R, E5, R, C5, D5, B4, R,
                          C5, R, G4, R, E4, R, A4, R, B4, R, A4S, A4, R,
                          G4, E5, G5, A5, R, F5, G5, R, E5, R, C5, D5, B4, R,
                          R, G5, F5S, F5, D5S, R, E5, R, G4S, A4, C5, R, A4, C5, D5,
                          R, G5, G5S, F5, D5S, R, E5, R, C6, R, C6, C6, R,
                          R, G5, F5S, F5, D5S, R, E5, R, G4S, A4, C5, R, A4, C5, D5,
                          R, D5S, R, D5, R, C5};
uint8_t mario_dur1[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 3, 1, 2, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1,
                        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1, 2, 
                        1, 2, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 
                        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1, 2,
                        2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
                        2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 
                        2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
                        2, 1, 2, 1, 2, 1, 0};
uint32_t mario_note2[] = {F4S, F4S, R, F4S, R, F4S, F4S, R, B4, R, G3, R, E4, R, C4, R, G3, R, C4, R, D4, R, C4S, C4, R,
                          C4, G4, B4, C5, R, A4, B4, R, A4, R, E4, F4, D4, R,
                          E4, R, C4, R, G3, R, C4, R, D4, R, C4S, C4, R,
                          C4, G4, B4, C5, R, A4, B4, R, A4, R, E4, F4, D4, R,
                          C3, R, E5, D5S, D5, B4, C4, C5, F3, E4, F4, G4, C4, C4, E4, F4,
                          C3, R, E5, D5S, D5, B4, G3, C5, R, F5, R, F5, F5, R, G3, R,
                          C3, R, E5, D5S, D5, B4, C4, C5, F3, E4, F4, G4, C4, C4, E4, F4,
                          C3, R, G4S, R, F4, R, E4};
uint8_t mario_dur2[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 3, 1, 2, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1,
                        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1, 2, 
                        1, 2, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 
                        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1, 2,
                        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
                        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                        1, 1, 1, 2, 1, 2, 1, 0}; //0 ends song

//Amazing Grace
uint16_t amazing_base_dur = 270; //duration in ms for eighth note
uint32_t amazing_note1[] = {D4, G4, B4, G4, B4, A4, G4, E4, D4, D4, G4, B4, G4, B4, A4, D5,
                            B4, D5, B4, D5, B4, G4, D4, E4, G4, G4, E4, D4, D4, G4, B4, G4, B4, A4, G4};
uint8_t amazing_dur1[] = { 2, 4, 1, 1, 4, 2, 4, 2, 4, 2, 4, 1, 1, 4, 2, 10, 
                           2, 3, 1, 1, 1, 4, 2, 3, 1, 1, 1, 4, 2, 4, 1, 1, 4, 2, 10, 0};
uint32_t amazing_note2[] = {B3, B3, D4, D4, C4, B3, C4, B3, B3, B3, D4, D4, D4, G3,
                            D4, D4, D4, D4, D4, C4, D4, C4, B3, G3, B3, D4, D4, C4, G3};
uint8_t amazing_dur2[] = { 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 10, 
                           2, 4, 2, 4, 2, 3, 1, 2, 4, 2, 4, 2, 4, 2, 10, 0}; //0 ends song