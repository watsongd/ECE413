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

#define TABLE_SIZE 256

int16_t sineTable[TABLE_SIZE] = {0,50,100,151,201,251,300,350,399,449,497,546,594,642,690,737,783,830,875,920,965,
        1009,1052,1095,1137,1179,1219,1259,1299,1337,1375,1411,1447,1483,1517,1550,
        1582,1614,1644,1674,1702,1729,1756,1781,1805,1828,1850,1871,1891,1910,1927,
        1944,1959,1973,1986,1997,2008,2017,2025,2032,2037,2041,2045,2046,2047,2046,
        2045,2041,2037,2032,2025,2017,2008,1997,1986,1973,1959,1944,1927,1910,1891,
        1871,1850,1828,1805,1781,1756,1729,1702,1674,1644,1614,1582,1550,1517,1483,
        1447,1411,1375,1337,1299,1259,1219,1179,1137,1095,1052,1009,965,920,875,830,
        783,737,690,642,594,546,497,449,399,350,300,251,201,151,100,50,0,-50,-100,
        -151,-201,-251,-300,-350,-399,-449,-497,-546,-594,-642,-690,-737,-783,-830,
        -875,-920,-965,-1009,-1052,-1095,-1137,-1179,-1219,-1259,-1299,-1337,-1375,
        -1411,-1447,-1483,-1517,-1550,-1582,-1614,-1644,-1674,-1702,-1729,-1756,
        -1781,-1805,-1828,-1850,-1871,-1891,-1910,-1927,-1944,-1959,-1973,-1986,
        -1997,-2008,-2017,-2025,-2032,-2037,-2041,-2045,-2046,-2047,-2046,-2045,
        -2041,-2037,-2032,-2025,-2017,-2008,-1997,-1986,-1973,-1959,-1944,-1927,
        -1910,-1891,-1871,-1850,-1828,-1805,-1781,-1756,-1729,-1702,-1674,-1644,
        -1614,-1582,-1550,-1517,-1483,-1447,-1411,-1375,-1337,-1299,-1259,-1219,
        -1179,-1137,-1095,-1052,-1009,-965,-920,-875,-830,-783,-737,-690,-642,-594,
        -546,-497,-449,-399,-350,-300,-251,-201,-151,-100,-50};

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

int16_t triangleTable[TABLE_SIZE];

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
        switch (PushState) {
            case NoPush:
                if (press) { PushState = MaybePushed;}
                else { PushState = NoPush; }
                break;
            case MaybePushed:
                if (press) {
                    //code to perform an action or record a digit
                    
                    // draw key number
//                    tft_fillRoundRect(30,140, 100, 28, 1, ILI9341_BLACK);// x,y,w,h,radius,color
//                    tft_setCursor(30, 140);
//                    tft_setTextColor(ILI9341_YELLOW); tft_setTextSize(2);
//                    sprintf(buffer,"%d", i);
//                    if (i==10)sprintf(buffer,"*");
//                    if (i==11)sprintf(buffer,"#");
//                    tft_writeString(buffer);
//                    PushState = Pushed;
//                    if (!playing) {
//                        selection = i;
//                    }
                    if (num_presses == 0) {
                        song_selection = i;
                        num_presses++;
                    }
                    if (num_presses > 0) {
                        if (i < 11) {
                            if (note_selection > 0) {
                                //concatenate logic
                                num_presses++;
                            }
                            else {
                            note_selection = i;
                            num_presses++;
                            }
                        }
                        else {
                            selection = song_selection;
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
            if((noteB_dur == 0 && noteA_dur == 0) || (k >= note_slection)){
                selection = 0;
                k = 0;
                j = 0;
                playing = 0;
            }
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // note thread

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
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1 , 238);
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


    // init the display
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9341_BLACK);
    //240x320 vertical display
    tft_setRotation(3); // Use tft_setRotation(1) for 320x240
    
    //UI
    tft_fillRoundRect(0,0, 200, 50, 1, ILI9341_BLACK);// x,y,w,h,radius,color
    tft_setCursor(0, 0);
    tft_setTextColor(ILI9341_WHITE); tft_setTextSize(2);
    tft_writeString("Press a key to play a song!");
    tft_setCursor(30, 30);
    tft_setTextColor(ILI9341_WHITE); tft_setTextSize(2);
    tft_writeString("1) Mario Theme Song");
    tft_setCursor(30, 60);
    tft_setTextColor(ILI9341_WHITE); tft_setTextSize(2);
    tft_writeString("2) Twinkle-Twinkle");
    tft_setCursor(30, 90);
    tft_setTextColor(ILI9341_WHITE); tft_setTextSize(2);
    tft_writeString("   Little Star");
    tft_setCursor(30, 120);
    tft_setTextColor(ILI9341_WHITE); tft_setTextSize(2);
    tft_writeString("3) Amazing Grace");
    
    //selection = 3;
    num_notes = 200;
    
    // round-robin scheduler for threads
    while (1){
        PT_SCHEDULE(protothread_keypad(&pt_keypad));
        PT_SCHEDULE(protothread_notes(&pt_notes));
    }
  } // main
// === end  ======================================================
//*/
