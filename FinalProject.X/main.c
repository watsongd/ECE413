/* Authors: Phil Bedoukian, Joe Sluke, Austin Wiles
 *
 * Target PIC:  PIC32MX250F128B
 *
 * Controls the entire system. Calls method from this class and other classes
 * to integrate all parts of the Musical Fingerprint-Scanning Doorbell.
 * Methods specifically in this class control the interface of the touch
 * display, including drawing the screens and detecting touch overlap.
 * State machines in this class determine what part of the system is active.
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
struct file note_files[30];
struct file temp_note_files[30];
struct file selected_note;

uint32_t music_sector;
//total number of notes
uint32_t num_notes = 0;
//number of songs on enroll_song_select
uint32_t num_notes_1 = 0;
//number of songs on enroll_song_select_2
uint32_t num_notes_2 = 0;
// string buffer
uint8_t buffer[60];
//number of notes in history
uint32_t history_count = 0;


int play_note = 0;
int prev_note = 0;

static struct pt pt_note;

// system 1 second interval tick
int sys_time_seconds;

/* -----------------------------------------------------------------------------
    Methods for initializing buttons and drawing buttons
   ---------------------------------------------------------------------------*/

//Reads the SD card, searching for notes, and creates files for each notes found on the SD card.
void init_notes() {
    initSongs(&temp_note_files);
    int i;
    //Finds notes on SD card and puts them in an array (some blocks aren't songs)
    for (i=0; i<30; i++) {
        //if cluster == 0, then ignore (not a song)
        if (temp_note_files[i].cluster != 0) {
            note_files[num_notes] = temp_note_files[i];
            num_notes++;
        }
    }
    //sets song counts for each song select page
    if (num_notes >= 10) {
        num_notes_1 = 10;
        num_notes_2 = num_notes - 10;
    }
    else {
        num_notes_1 = num_notes;
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
        if (play_note == 1) {
            playSong(&selected_note);
            play_note = 0;
        }
    }//END WHILE(1)
    PT_END(pt);
}

/* -----------------------------------------------------------------------------
    Main
   ---------------------------------------------------------------------------*/
void main(void) {
    SYSTEMConfigPerformance(PBCLK);

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
      PT_SCHEDULE(protothread_song(&pt_note));

      }
  }