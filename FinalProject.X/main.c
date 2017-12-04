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


//Notes for a basic scale
#define G1 0x0E0
#define A1 0x0C0
#define B1 0x080
#define C1 0x040
#define D1 0x1EE
#define E1 0x1EC
#define F1 0x1E8
#define G2 0x1E0

/*

 *
 ****************************************************/

// PORT B
#define EnablePullDownB(bits) CNPUBCLR=bits; CNPDBSET=bits;
#define DisablePullDownB(bits) CNPDBCLR=bits;

/* -----------------------------------------------------------------------------
    Structs
   ---------------------------------------------------------------------------*/

typedef struct History {
    int id;
    char note_title[8];
} History;

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


int play_note = 0;

static struct pt pt_note, pt_finger_pos;

// system 1 second interval tick
int sys_time_seconds;

/* -----------------------------------------------------------------------------
    Methods for initializing buttons and drawing buttons
   ---------------------------------------------------------------------------*/

//Reads the SD card, searching for notes, and creates files for each notes found on the SD card.
void init_notes() {
    initNotes(&temp_note_files);
    int i;
    //Finds notes on SD card and puts them in an array (some blocks aren't songs)
    for (i=0; i<30; i++) {
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

int j = 0;

//This proto-thread checks for a play_note flag and plays the selected note when high
static PT_THREAD (protothread_note(struct pt *pt))
{
    PT_BEGIN(pt);
      while(1) {
        //check every millisecond to play song
        PT_YIELD_TIME_msec(1);
        //if play_note flag is high, play song and then reset the flag
        if (play_note == 1) {
            playSong(&selected_note);
            play_note = 0;
        }
        counter++;
        if(counter == 2000) {
            play_note = 1;
            selected_note = note_files[j];
            if (j > 10) {
                j = 0;
            }
            else {
                j++;
            }
            counter = 0;
        }
    }//END WHILE(1)
    PT_END(pt);
}

//This proto-thread checks the data from the sensors and selects a note to play 
//depending on which sensors are bent.
static PT_THREAD (protothread_finger_pos(struct pt *pt))
{
    PT_BEGIN(pt);
      while(1) {
        //check finger positions every 20 milliseconds
        PT_YIELD_TIME_msec(20);
        uint16_t fingerPos = getFingerPosition();
        switch (fingerPos) {
            case (0x:080):
                //Note == B1
                break;
            case (0x0C0):
                //Note == A1
                break;
            case (0x040):
                //Note == C1
                break;
            case (0x0E0):
                //Note == G1
                break;
            case(0x1EE):
                //NOte == D1
                break;
                
        }       
        
    }//END WHILE(1)
    PT_END(pt);
}
//Method for getting finger positions
//Returns an int with finger positions encoded
int getFingerPosition(){
   uint16_t left_thumb   = mPORTBReadBits(BIT_12)<< 9;
   uint16_t left_index   = mPORTBReadBits(BIT_10)<< 8;
   uint16_t left_middle  = mPORTBReadBits(BIT_8) << 7;
   uint16_t left_ring    = mPORTBReadBits(BIT_7) << 6;
   uint16_t left_pinky   = mPORTBReadBits(BIT_6) << 4;
   
   uint16_t right_index  = mPORTBReadBits(BIT_0) << 3;
   uint16_t right_middle = mPORTBReadBits(BIT_1) << 2;
   uint16_t right_ring   = mPORTBReadBits(BIT_2) << 1;
   uint16_t right_pinky  = mPORTBReadBits(BIT_3);
   
   uint16_t fingerPosition = (left_thumb || left_index || left_middle || left_ring || left_pinky || right_index || right_middle || right_ring || right_pinky);
   
   return fingerPosition; 
}
/* -----------------------------------------------------------------------------
    Main
   ---------------------------------------------------------------------------*/
void main(void) {
    SYSTEMConfigPerformance(sys_clock);

    ANSELA = 0; ANSELB = 0; CM1CON = 0; CM2CON = 0;

    // === configure threads ==========
    PT_setup();

    // initialize the threads
    PT_INIT(&pt_note);

    //setup gloves!!
    
    INTEnableSystemMultiVectoredInt();
    init_notes();
    //
    delay_ms(10);
    
    


    while (1){
        PT_SCHEDULE(protothread_note(&pt_note));

    }
}