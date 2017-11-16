/*
 *              main.c by
 * Author:      Connor Nace & Geoff Watson
 * Target PIC:  PIC32MX250F128B
 */
///*
// graphics libraries
#include "config.h"
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>
// for concatenating strings
#include <string.h>

//Include Notes
#include "notes.h"
//#include "lab3_songs.c"

#include "math.h"

// threading library
// config.h sets 40 MHz
#define	SYS_FREQ 40000000
#include "pt_cornell_1_2.h"

// PORT B
#define EnablePullDownB(bits) CNPUBCLR=bits; CNPDBSET=bits;
#define DisablePullDownB(bits) CNPDBCLR=bits;

#define SS      LATBbits.LATB13
#define dirSS   TRISBbits.TRISB13
#define anSS    ANSELBbits.ANSB13

#define G1 0x0E0
#define A1 0x0C0
#define B1 0x080
#define C1 0x040
#define D1 0x1EE
#define E1 0x1EC
#define F1 0x1E8
#define G2 0x1E0

#define F_CPU 40000000
#define F_dac 550000
const short PR = F_CPU/F_dac - 1;

// string buffer
char buffer[60];

int PushState = 1;

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_keypad, pt_notes;

static PT_THREAD (protothread_keypad(struct pt *pt))
{
    PT_BEGIN(pt);
    while(1) {      
        uint16_t fingerPosition = getFingerPosition();
        // Debouncer
        switch (PushState) {
            case NoPush:
                if (press) { PushState = MaybePushed;}
                else { PushState = NoPush; }
                break;
            case MaybePushed:
                if (press) {
                    PushState = Pushed;

                    if (num_presses == 0) {
                        song_selection = i;
                        num_presses++;
                    }
                    else if (num_presses > 0) {
                        if (i < 11) {
                            if (note_selection > 0) {
                                //concatenate logic
                                note_selection = note_selection * 10 + i;
                                num_presses++;
                            }
                            else {
                                note_selection = i;
                                num_presses++;
                            }
                        }
                        else {
                            if (note_selection != 0){
                                num_notes = note_selection;
                            }
                            else {
                                num_notes = 300;
                            }
                            selection = song_selection;
                            num_presses = 0;
                            note_selection = 0;
                            song_selection = 0;
                        }
                    }
                }

                else { PushState = NoPush; }
                break;
            case Pushed:
                if (press) { PushState = Pushed; }
                else { PushState = MaybeNoPush; }
                break;                
            case MaybeNoPush:
                if (press) { PushState = Pushed; }
                else { PushState = NoPush; }
                break;        
            }
        }
        // NEVER exit while
     } // END WHILE(1)
    
  PT_END(pt);
} // keypad thread

//Note Thread ===================================================
static PT_THREAD (protothread_notes(struct pt *pt))
{
    PT_BEGIN(pt);
        while(1) {
            switch (PushState) {
                case NoPush:
                    if (press) { PushState = MaybePushed;}
                    else { PushState = NoPush; }
                    break;
                case MaybePushed:
                    if (press) {
                        //code to perform an action or record a digit

                        // draw key number
                        tft_fillRoundRect(30,140, 100, 28, 1, ILI9341_BLACK);// x,y,w,h,radius,color
                        tft_setCursor(30, 140);
                        tft_setTextColor(ILI9341_YELLOW); tft_setTextSize(2);
                        sprintf(buffer,"%d", i);
                        if (i==10)sprintf(buffer,"*");
                        if (i==11)sprintf(buffer,"#");
                        tft_writeString(buffer);
                        PushState = Pushed;

                        if (num_presses == 0) {
                            song_selection = i;
                            num_presses++;
                        }
                        else if (num_presses > 0) {
                            if (i < 11) {
                                if (note_selection > 0) {
                                    //concatenate logic
                                    note_selection = note_selection * 10 + i;
                                    num_presses++;
                                }
                                else {
                                    note_selection = i;
                                    num_presses++;
                                }
                            }
                            else {
                                if (note_selection != 0){
                                    num_notes = note_selection;
                                }
                                else {
                                    num_notes = 300;
                                }
                                selection = song_selection;
                                num_presses = 0;
                                note_selection = 0;
                                song_selection = 0;
                            }
                        }
                    }

                    else { PushState = NoPush; }
                    break;
                case Pushed:
                    if (press) { PushState = Pushed; }
                    else { PushState = MaybeNoPush; }
                    break;                
                case MaybeNoPush:
                    if (press) { PushState = Pushed; }
                    else { PushState = NoPush; }
                    break;      
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // note thread

int getFingerPosition(){
   uint16_t left_thumb   = mPORTBReadBits(BIT_10) << 9;
   uint16_t left_index   = mPORTBReadBits(BIT_11) << 8;
   uint16_t left_middle  = mPORTBReadBits(BIT_13) << 7;
   uint16_t left_ring    = mPORTBReadBits(BIT_14) << 6;
   uint16_t left_pinky   = mPORTBReadBits(BIT_15) << 4;
   
   uint16_t right_index  = mPORTBReadBits(BIT_7) << 3;
   uint16_t right_middle = mPORTBReadBits(BIT_8) << 2;
   uint16_t right_ring   = mPORTBReadBits(BIT_9) << 1;
   uint16_t right_pinky  = mPORTBReadBits(BIT_5);
   
   unint16_t fingerPosition = (left_thumb || left_index || left_middle || left_ring || left_pinky
           || right_index || right_middle || right_ring || right_pinky);
   
   return fingerPostion; 
}

void initDAC(void){
    anSS = 0;
    dirSS = 0;
    SS = 1;
    //RPB11R = 3; // SDO
    PPSOutput(2, RPB5, SDO2);
    SpiChnOpen(2, SPI_OPEN_MSTEN | SPI_OPEN_MODE16 | SPI_OPEN_ON |
            SPI_OPEN_DISSDI | SPI_OPEN_CKE_REV , 2);
    // Clock at 20MHz
    
}

inline void writeDAC(uint16_t data){
    SS = 0;
    while (TxBufFullSPI2());
    WriteSPI2(data);
    while (SPI2STATbits.SPIBUSY); // wait for it to end of transaction
    SS = 1;
}

void initTimers(void){
    OpenTimer1(T1_ON | T1_PS_1_1, PR);
    // Configure T1 for DAC update frequency
    ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_2); 
    
      // Setup Timers
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1 , 236);
}

void __ISR(_TIMER_1_VECTOR, ipl2) T1Int(void){
    mT1ClearIntFlag();
}
// === Main  ======================================================
void main(void) {
    SYSTEMConfigPerformance(PBCLK);
    mPORTBSetPinsDigitalIn(BIT_5 | BIT_7 | BIT_8 | BIT_9 | BIT_10 | BIT_11 | BIT_13 | BIT_14 | BIT_15);
    PT_setup(); 
     
    initDAC();
    initTimers();
    INTEnableSystemMultiVectoredInt();

    // init the threads
    PT_INIT(&pt_keypad);
    PT_INIT(&pt_notes);
    
    // round-robin scheduler for threads
    while (1){
        PT_SCHEDULE(protothread_keypad(&pt_keypad));
        PT_SCHEDULE(protothread_notes(&pt_notes));
    }
  } // main
// === end  ======================================================
//*/
