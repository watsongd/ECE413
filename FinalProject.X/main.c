/*  
 * Authors: Connor Nace and Geoff Watson
 * Target PIC:  PIC32MX250F128B
 *
 * Controls the entire system. Calls method from this class and other classes
 * to integrate all parts of the Virtual Instrument.
 * Methods specifically in this class control initial reading of the SD card
 *
 */
#include "config.h"
// need for rand function
#include <stdlib.h>

// threading library
#include <plib.h>
// config.h sets 40 MHz
#define	SYS_FREQ 40000000
#include "pt_cornell_1_2.h"
#include <stdio.h>
#include "math.h"
#include "structs.h"
#include <string.h>

#define use_uart_serial

//Notes for a basic scale
#define G1 0x2C0
#define A1 0x280
#define B1 0x200
#define C1 0x040
#define D1 0xac7

//Note Done Yet
//#define E1 0x1EC
//#define F1 0x1E8
//#define G2 0x1E0

/*

 *
 ****************************************************/

// PORT B
//#define EnablePullDownB(bits) CNPUBCLR=bits; CNPDBSET=bits;
//#define DisablePullDownB(bits) CNPDBCLR=bits;
//#define DisablePullDownA(bits) CNPDACLR=bits;


/* -----------------------------------------------------------------------------
    Structs
   ---------------------------------------------------------------------------*/

typedef struct Note {
    uint8_t id;
    uint8_t fileIndex;
    char name[7];
} Note;

/* -----------------------------------------------------------------------------
    Initializing variables
   ---------------------------------------------------------------------------*/

//file struct array containing notes
struct file notes[201];

//music file arrays
struct file note_files[10];
struct file temp_note_files[10];
struct file selected_note;

//total number of notes
uint32_t num_notes = 0;
uint32_t counter = 0;
// string buffer
uint8_t buffer[60];

//notes
Note possible_notes[10];


int play_note = 0;

static struct pt pt_note, pt_finger_pos, pt_UART, pt_input;

// system 1 second interval tick
int sys_time_seconds;

/* -----------------------------------------------------------------------------
    Methods for initializing buttons and drawing buttons
   ---------------------------------------------------------------------------*/

//Reads the SD card, searching for notes, and creates files for each notes found on the SD card.
void init_notes() {
    initNotes(&temp_note_files);
    int i;
    int j;
    //Finds notes on SD card and puts them in an array (some blocks aren't songs)
    //10 is size of temp_note_files, so change range if more notes are added
    for (i=0; i<10; i++) {
        //if cluster == 0, then ignore (not a song)
        if (temp_note_files[i].cluster != 0) {
            note_files[num_notes] = temp_note_files[i];
            num_notes++;
        }
    }
}


/* -----------------------------------------------------------------------------
      Proto-threads
   ---------------------------------------------------------------------------*/

//This proto-thread checks for a play_note flag and plays the selected note when high
static PT_THREAD (protothread_note(struct pt *pt))
{
    PT_BEGIN(pt);
      while(1) {
        //check every millisecond to play song
        PT_YIELD_TIME_msec(1);
        //if play_note flag is high, play song and then reset the flag
//        if (play_note == 1) {
//            playSong(&selected_note);
//        }
        playSong(&selected_note);

    }//END WHILE(1)
    PT_END(pt);
}
char c;
float value;
float kp = 1;
float ki = 1;
float kd = 1;
int desiredRPM = 2500;

static PT_THREAD (protothread_UART(struct pt *pt))
{
    PT_BEGIN(pt);  
    while(1) {
        //printf('Printing to the terminal /n/r');
//        PT_SPAWN(pt, &pt_input, PT_GetSerialBuffer(&pt_input));
//        printf("%s\n\r", PT_term_buffer);
//        sscanf(PT_term_buffer, "%c %f", &c, &value);
        printf("hello world");
        //PT_SPAWN(pt, &pt_input, PutSerialBuffer(&pt_input));
        PT_YIELD_TIME_msec(30);
    } // END WHILE(1)
  PT_END(pt);
} // UART thread

////This proto-thread checks for the foot pedal to be pressed and sets a flag to
////play the note corresponding the finger position
//static PT_THREAD (protothread_footpedal(struct pt *pt))
//{
//    PT_BEGIN(pt);
//      while(1) {
//        //check every millisecond to play note
//        PT_YIELD_TIME_msec(10);
//        //if foot pedal is pressed, set play note to 1
//        if (1) {
//            play_note = 1;
//        }
//        else {
//            play_note = 0;
//        }
//    }//END WHILE(1)
//    PT_END(pt);
//}

//This proto-thread checks the data from the sensors and selects a note to play 
//depending on which sensors are bent.
static PT_THREAD (protothread_finger_pos(struct pt *pt))
{
    PT_BEGIN(pt);
      while(1) {
        //check finger positions every 10 milliseconds
          //
        PT_YIELD_TIME_msec(10);
        int i;
        //uint16_t fingerPos = getFingerPosition();
        uint16_t fingerPos = 0;
        switch (fingerPos) {
            case B1:
                //Note == B1
                selected_note = note_files[0];
                for (i = 0; i < sizeof(note_files)/sizeof(note_files[0]); i++) {
                    char* firstChar  = &(note_files[i].fileName[0]);
                    const char* secondChar = &(note_files[i].fileName[1]);
                    //const char* note = "B1";
                    if (strcmp("B1",strcat(firstChar, secondChar)) == 0) {
                        selected_note = note_files[1];
                    }
                }
                break;
            case A1:
                //Note == A1
                selected_note = note_files[1];
//                for (i = 0; i < sizeof(note_files)/sizeof(note_files[0]); i++) {
//                    char* firstChar  = &(note_files[i].fileName[0]);
//                    const char* secondChar = &(note_files[i].fileName[1]);
//                    const char* note = "A1";
//                    if (strcmp(note,strcat(firstChar, secondChar)) == 0) {
//                        selected_note = note_files[1];
//                    }
//                }
                break;
            case C1:
                //Note == C1
                selected_note = note_files[2];
//                for (i = 0; i < sizeof(note_files)/sizeof(note_files[0]); i++) {
//                    char* firstChar  = &(note_files[i].fileName[0]);
//                    const char* secondChar = &(note_files[i].fileName[1]);
//                    const char* note = "C1";
//                    if (strcmp(note,strcat(firstChar, secondChar)) == 0) {
//                        selected_note = note_files[1];
//                    }
//                }
                break;
            case G1:
                //Note == G1
                selected_note = note_files[3];
//                for (i = 0; i < sizeof(note_files)/sizeof(note_files[0]); i++) {
//                    char* firstChar  = &(note_files[i].fileName[0]);
//                    const char* secondChar = &(note_files[i].fileName[1]);
//                    const char* note = "G1";
//                    if (strcmp(note,strcat(firstChar, secondChar)) == 0) {
//                        selected_note = note_files[1];
//                    }
//                }
                break;
            case D1:
                //NOte == D1
                selected_note = note_files[4];
//                for (i = 0; i < sizeof(note_files)/sizeof(note_files[0]); i++) {
//                    char* firstChar  = &(note_files[i].fileName[0]);
//                    const char* secondChar = &(note_files[i].fileName[1]);
//                    const char* note = "D1";
//                    if (strcmp(note,strcat(firstChar, secondChar)) == 0) {
//                        selected_note = note_files[1];
//                    }
//                }
                break;               
        }       
        
    }//END WHILE(1)
    PT_END(pt);
}
//Method for getting finger positions
//Returns an int with finger positions encoded
//int getFingerPosition(){
//   uint16_t left_thumb   = mPORTBReadBits(BIT_12);
//   uint16_t left_index   = mPORTBReadBits(BIT_10);
//   uint16_t left_middle  = mPORTBReadBits(BIT_8);
//   uint16_t left_ring    = mPORTBReadBits(BIT_7);
//   uint16_t left_pinky   = mPORTBReadBits(BIT_6);
//   
//   uint16_t right_index  = mPORTBReadBits(BIT_0);
//   uint16_t right_middle = mPORTBReadBits(BIT_1);
//   uint16_t right_ring   = mPORTBReadBits(BIT_2);
//   uint16_t right_pinky  = mPORTBReadBits(BIT_3);
//   
//   uint16_t fingerPosition = (left_thumb || left_index || left_middle || left_ring || left_pinky || right_index || right_middle || right_ring || right_pinky);
//   
//   return fingerPosition; 
//}
//
//int footPedalPushed() {
//    return mPORTBReadBits(BIT_0);
//}
/* -----------------------------------------------------------------------------
    Main
   ---------------------------------------------------------------------------*/
void main(void) {
    SYSTEMConfigPerformance(sys_clock);

    ANSELA = 0; ANSELB = 0; CM1CON = 0; CM2CON = 0;
    
    //need to disable pull up / down resistor
//    CNPUA = 0;
//    CNPDA = 0;
//    CNPUB = 0;
//    CNPDB = 0;
//    DisablePullDownA(0);
//    DisablePullDownB(0);

    
    //mPORTACloseBits(BIT_0);
    //mPORTASetPinsDigitalIn(BIT_0);
    
    // 1 0101 1100 1111
    //mPORTBSetPinsDigitalIn(0x15CF);

    // === configure threads ==========
    PT_setup();

    // initialize the threads
    //PT_INIT(&pt_note);
    //PT_INIT(&pt_finger_pos);
    //PT_INIT(&pt_footpedal);
    PT_INIT(&pt_UART);
    
    INTEnableSystemMultiVectoredInt();
    init_notes();
    
    //delay_ms(20);
    
//    for (i = 0; i < sizeof(note_files)/sizeof(note_files[0]); i++) {
//        playSong(note_files[i]);
//    }
    while (1){
        //PT_SCHEDULE(protothread_note(&pt_note));
        PT_SCHEDULE(protothread_UART(&pt_UART));
        //PT_SCHEDULE(protothread_finger_pos(&pt_finger_pos));
        //PT_SCHEDULE(protothread_footpedal(&pt_footpedal));
    }
}