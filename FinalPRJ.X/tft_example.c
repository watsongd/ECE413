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

#define NoPush      1 
#define MaybePushed 2 
#define Pushed      3 
#define MaybeNoPush 4

#define F_CPU 40000000
#define F_dac 550000
const short PR = F_CPU/F_dac - 1;

volatile uint32_t accum_amtA, accum_amtB;

// string buffer
char buffer[60];

int PushState = 1;

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_keypad, pt_notes;

// system 1 second interval tick
int sys_time_seconds ;

// === Keypad Thread =============================================
// connections:
// A0 -- row 1 -- thru 300 ohm resistor -- avoid short when two buttons pushed
// A1 -- row 2 -- thru 300 ohm resistor
// A2 -- row 3 -- thru 300 ohm resistor
// A3 -- row 4 -- thru 300 ohm resistor
// B7 -- col 1 -- 10k internal pulldown resistor -- avoid open circuit input when no button pushed
// B8 -- col 2 -- 10k internal pulldown resistor
// B9 -- col 3 -- 10k internal pulldown resistor

//values set by keypad
int selection = 0, num_notes = 0;
int playing = 0;
int note_selection = 0, song_selection = 0;
int go_play = 0;
int num_presses = 0;
//Keeps track of how many times the user has pressed the button (1,2,or 3)
int input_num = 0;
static PT_THREAD (protothread_keypad(struct pt *pt))
{
    PT_BEGIN(pt);
    static int pad, i, pattern, press;
    // order is 0 thru 9 then * ==10 and # ==11
    // no press = -1
    // table is decoded to natural digit order (except for * and #)
    // 0x80 for col 1 ; 0x100 for col 2 ; 0x200 for col 3
    // 0x01 for row 1 ; 0x02 for row 2; etc
    static int keytable[12]={0x108, 0x81, 0x101, 0x201, 0x82, 0x102, 0x202, 0x84, 0x104, 0x204, 0x88, 0x208};
    // init the keypad pins A0-A3 and B7-B9
    // PortA ports as digital outputs
    mPORTASetPinsDigitalOut(BIT_0 | BIT_1 | BIT_2 | BIT_3);    //Set port as output
    // PortB as inputs
    mPORTBSetPinsDigitalIn(BIT_7 | BIT_8 | BIT_9);    //Set port as input
    
     while(1) {        
        // read each row sequentially
        mPORTAClearBits(BIT_0 | BIT_1 | BIT_2 | BIT_3);
        pattern = 1; mPORTASetBits(pattern);
        
        // yield time
        PT_YIELD_TIME_msec(30);
        //mPORTAClearBits(BIT_0 | BIT_1 | BIT_2 | BIT_3);
        //pattern = 1; mPORTASetBits(pattern);
        for (i=0; i<4; i++) {
            //if any of the buttons are pressed, pad goes high
            pad  = mPORTBReadBits(BIT_7 | BIT_8 | BIT_9);
            //if a singular button has been pressed, pad != 0 
            if(pad!=0) {pad |= pattern; break;}
            mPORTAClearBits(pattern);
            pattern <<= 1;
            mPORTASetBits(pattern);
        }

        // search for keycode
        if (pad > 0){ // then button is pushed
            for (i=0; i<12; i++){
                if (keytable[i]==pad) break;
            }
        }
        else i = -1; // no button pushed
        
        //define press as a button is pressed
        if (i != -1) {
            press = 1;
        }
        else {
            press = 0;
        }
        // Debouncer
        if (!playing) {
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
            }
        }
        // NEVER exit while
     } // END WHILE(1)
    
  PT_END(pt);
} // keypad thread

int k = 0;
int j = 0;
uint16_t base_dur;
uint8_t  noteA_dur, noteB_dur;
uint32_t noteA, noteB;
uint8_t noteA_count = 0, noteB_count = 0;
//Note Thread ===================================================
static PT_THREAD (protothread_notes(struct pt *pt))
{
    PT_BEGIN(pt);
        while(1) {
            if (selection == 0){
                base_dur = 100;
                noteA = 0;
                noteB = 0;
                noteA_dur = 1;
                noteB_dur = 1;
                playing = 0;
                j = 0;
                k = 0;
                //noteA_count = 0;
                //noteB_count = 0;
                tft_fillRect(0,25,29,200, ILI9341_BLACK);
            }
            else if(selection == 1){
                base_dur = mario_base_dur;

                noteA = mario_note1[k];
                noteA_dur = mario_dur1[k];

                noteB = mario_note2[j];
                noteB_dur = mario_dur2[j];

                playing = 1;
                tft_fillCircle(15,35,10, ILI9341_GREEN);
            }
            else if(selection == 3){
                base_dur = amazing_base_dur;

                noteA = amazing_note1[k];
                noteA_dur = amazing_dur1[k];

                noteB = amazing_note2[j];
                noteB_dur = amazing_dur2[j];

                playing = 1;
                tft_fillCircle(15,125,10, ILI9341_GREEN);
            }

            else if(selection == 2){
                base_dur = twinkle_base_dur;

                noteA = twinkle_note1[k];
                noteA_dur = twinkle_dur1[k];

                noteB = twinkle_note2[j];
                noteB_dur = twinkle_dur2[j];

                playing = 1;
                tft_fillCircle(15,65,10, ILI9341_GREEN);
            }
            else if(selection == 4){
                base_dur = 100;

                noteA = C6;
                noteA_dur = 1;

                noteB = C6;
                noteB_dur = 1;
                num_notes = 1;

                playing = 1;
                tft_fillCircle(15,65,10, ILI9341_RED);
            }

            accum_amtA = noteA;
            accum_amtB = noteB;

            PT_YIELD_TIME_msec(base_dur);
            if (playing == 1) {
                noteA_count++;
                noteB_count++;
            }
            if (noteA_count == noteA_dur) {
                noteA_count = 0;
                k++;
            }
            if (noteB_count == noteB_dur) {
                noteB_count = 0;
                j++;
            }
            if((noteB_dur == 0 && noteA_dur == 0) || (k >= num_notes)){
                selection = 0;
                k = 0;
                j = 0;
                playing = 0;
            }
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
   
   unint16_t fingerPosition = left_thumb || left_index || left_middle || left_ring || left_pinky
           || right_index || right_middle || right_ring || right_pinky;
   
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

uint32_t accumA = 0;
uint32_t accumB = 0;
void __ISR(_TIMER_1_VECTOR, ipl2) T1Int(void){
    accumA += accum_amtA;
    accumB += accum_amtB;
    int16_t sine_valA = 2048+sineTable[(accumA>>24)&0xff];
    int16_t sine_valB = 2048+sineTable[(accumB>>24)&0xff];
    
    int16_t sine_valTotal = (sine_valA>>1) + (sine_valB>>1);
    
    writeDAC(0x3000 | sine_valTotal); // write to channel A, gain = 1
    //writeDAC(0xB000 | sine_valB); // write to channel B, gain = 1
    
    if (((accumA>>24)&0xff) == TABLE_SIZE) accumA = 0;
    if (((accumB>>24)&0xff) == TABLE_SIZE) accumB = 0;
    LATAINV = 1;
    mT1ClearIntFlag();
}
// === Main  ======================================================
void main(void) {
    SYSTEMConfigPerformance(PBCLK);
    ANSELA = 0; ANSELB = 0; CM1CON = 0; CM2CON = 0;
    //TRISB = 0x4000; //TRISA = 0x0020;
 
    //internal pull downs for keypad
    EnablePullDownB( BIT_7 | BIT_8 | BIT_9);
  
    // === config threads ==========
    // turns OFF UART support and debugger pin
    PT_setup(); 
  
    //================================================
   
    initDAC();
    TRISACLR = 1;

    initTimers();
    INTEnableSystemMultiVectoredInt();
    
    //===============================================  
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
