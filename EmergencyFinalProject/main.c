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
//Include Notes
#include "notes.h"

// threading library
// config.h sets 40 MHz
#define	SYS_FREQ 40000000
#include "pt_cornell_1_2.h"

// PORT B
#define EnablePullDownB(bits) CNPUBCLR=bits; CNPDBSET=bits;
#define DisablePullDownB(bits) CNPDBCLR=bits;

//Notes for a basic scale
#define G1 0x0380
#define A1 0x0300
#define B1 0x0200
#define C1 0x0100
#define D1 0x1387

//Note Done Yet
//#define E1 0x1EC
//#define F1 0x1E8
//#define G2 0x1E0

#define SS      LATBbits.LATB13
#define dirSS   TRISBbits.TRISB13
#define anSS    ANSELBbits.ANSB13

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

int16_t triangleTable[TABLE_SIZE];

#define F_CPU 40000000
#define F_dac 550000
const short PR = F_CPU/F_dac - 1;

volatile uint32_t accum_amtA, accum_amtB;

char hot_cross_buns[17] = {'B', 'A', 'G', 'B', 'A', 'G', 'B', 'B', 'B', 'B', 'A', 'A', 'A', 'A', 'B', 'A', 'G'};
int hcb_idx = 0;

// string buffer
char buffer[60];
char buffer1[60];
char buffer2[60];
char buffer3[60];
char buffer4[60];
char buffer5[60];

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_finger_pos, pt_UI;

uint32_t accumA = 0;
uint32_t accumB = 0;
uint32_t noteA, noteB;
int counter = 0;
int rightNote = 0;

////Note Thread ===================================================
static PT_THREAD (protothread_finger_pos(struct pt *pt))
{
    PT_BEGIN(pt);
    while(1) {
        uint16_t fingerPos = getFingerPosition();
        uint16_t footpedal = footPedalPushed();
//        noteA = A3;
//        noteB = A3;
        switch (fingerPos) {
            case 519:
                noteA = B3;
                noteB = B3;
                break;
            case 263:
                noteA = A3;
                noteB = A3;
                break;
            case 903:
                noteA = C4;
                noteB = C4;
                break;
            case 775:
                noteA = G3;
                noteB = G3;
                break;
            case 7:
                noteA = 0;
                noteB = 0;
                break;
            case D1:
                break;  
        }
        
        if (footpedal) {
            accum_amtA = noteA;
            accum_amtB = noteB;
        }
        else {
            accum_amtA = 0;
            accum_amtB = 0;
        }
        PT_YIELD_TIME_msec(10);
        // NEVER exit while
    } // END WHILE(1)
  PT_END(pt);
} // note thread

// proto-thread for UI setup
static PT_THREAD (protothread_UI(struct pt *pt))
{
    PT_BEGIN(pt);  
    while(1) {
        
        tft_setCursor(170, 120);tft_setTextSize(5);
        
//        //erase old text
        tft_setTextColor(ILI9341_BLACK);
        tft_writeString(buffer);
        tft_setCursor(120, 100);
        tft_writeString(buffer1);
        tft_setCursor(170, 100);tft_setTextSize(4);
        tft_writeString(buffer2);
        tft_setCursor(220, 100);tft_setTextSize(2);
        tft_writeString(buffer3);
        tft_setCursor(0, 0);tft_setTextSize(2);
        tft_writeString(buffer4);
        tft_setCursor(80, 120);tft_setTextSize(3);
        tft_writeString(buffer5);
        
        if (counter < 10) {
            tft_setCursor(0, 0);tft_setTextSize(2);
            tft_setTextColor(ILI9341_WHITE);
            sprintf(buffer4,"Time For Hot Cross Buns!");
            tft_writeString(buffer4);
        }
        else if (counter > 10 && counter < 15) {
            tft_setCursor(170, 120);tft_setTextSize(5);
            tft_setTextColor(ILI9341_WHITE);
            sprintf(buffer,"3!");
            tft_writeString(buffer);
        }
        else if (counter > 15 && counter < 20) {
            tft_setCursor(170, 120);tft_setTextSize(5);
            tft_setTextColor(ILI9341_WHITE);
            sprintf(buffer,"2!");
            tft_writeString(buffer);
        }
        else if (counter > 20 && counter < 25) {
            tft_setCursor(170, 120);tft_setTextSize(5);
            tft_setTextColor(ILI9341_WHITE);
            sprintf(buffer,"1!");
            tft_writeString(buffer);
        }
        else if (counter > 25 && counter < 30) {
            tft_setCursor(80, 120);tft_setTextSize(3);
            tft_setTextColor(ILI9341_WHITE);
            sprintf(buffer,"Follow Along!");
            tft_writeString(buffer5);
        }
        else if (counter > 30) {
            if (hcb_idx > 0 && hcb_idx <17) {
                sprintf(buffer1,"%c", hot_cross_buns[hcb_idx - 1]);
                sprintf(buffer2,"%c", hot_cross_buns[hcb_idx]);
                sprintf(buffer3,"%c", hot_cross_buns[hcb_idx + 1]);
            }
            else if (hcb_idx == 17) {
                sprintf(buffer1,"%c", hot_cross_buns[hcb_idx - 1]);
                sprintf(buffer2,"%c", hot_cross_buns[hcb_idx]);
            }
            else if (hcb_idx == 0) {
                sprintf(buffer2,"%c", hot_cross_buns[hcb_idx]);
                sprintf(buffer3,"%c", hot_cross_buns[hcb_idx + 1]);
            }
            if (counter % 5 == 0) {
                hcb_idx++;
            }
        }
 
        tft_setCursor(120, 100);tft_setTextSize(2);
        tft_setTextColor(ILI9341_WHITE);
        tft_writeString(buffer1);
        
        tft_setCursor(170, 100);tft_setTextSize(4);
        if (correctNote(hot_cross_buns[hcb_idx])) {
            tft_setTextColor(ILI9341_GREEN);
        }
        else {
            tft_setTextColor(ILI9341_RED);
        }
        tft_writeString(buffer2);
        
        tft_setCursor(220, 100);tft_setTextSize(2);
        tft_setTextColor(ILI9341_WHITE);
        tft_writeString(buffer3);
        
        counter = counter + 1;
        PT_YIELD_TIME_msec(50);
    } // END WHILE(1)
  PT_END(pt);
} // UART thread

int getFingerPosition(){
    uint16_t left_thumb   = mPORTBReadBits(BIT_12);
    uint16_t left_index   = mPORTBReadBits(BIT_9);
    uint16_t left_middle  = mPORTBReadBits(BIT_8);
    uint16_t left_ring    = mPORTBReadBits(BIT_7);
    
    uint16_t right_index  = mPORTBReadBits(BIT_0);
    uint16_t right_middle = mPORTBReadBits(BIT_1);
    uint16_t right_ring   = mPORTBReadBits(BIT_2);
    uint16_t right_pinky  = mPORTBReadBits(BIT_3);
    
    uint16_t fingerPosition = (left_thumb | left_index | left_middle | left_ring | right_index | right_middle | right_ring | right_pinky);
    //uint16_t fingerPosition = left_index;
    
    return fingerPosition; 
}
int footPedalPushed() {
    return mPORTAReadBits(BIT_1) >> 1;
}

int correctNote(char *charNote) {
    if (charNote == 'B') {
        if (getFingerPosition() == 519) {
            return 1;
        }
        else {
            return 0;
        }
    }
    else if (charNote == 'A') {
        if (getFingerPosition() == 263) {
            return 1;
        }
        else {
            return 0;
        }
    }
    else if (charNote == 'G') {
        if (getFingerPosition() == 775) {
            return 1;
        }
        else {
            return 0;
        }
    }
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
   mPORTBSetPinsDigitalIn(BIT_12 | BIT_10 | BIT_8 | BIT_7 | BIT_3 | BIT_2 | BIT_1 | BIT_0);
   mPORTASetPinsDigitalIn(BIT_1);
    // === config threads ==========
    // turns OFF UART support and debugger pin
    PT_setup(); 
  
    //================================================
    CNPDB = (BIT_12 | BIT_10 | BIT_8 | BIT_7 | BIT_3 | BIT_2 | BIT_1 | BIT_0);
    //CNPDA = (BIT_0);
    
    initDAC();
    TRISACLR = 1;

    initTimers();
    INTEnableSystemMultiVectoredInt();
    
    //===============================================  
    // init the threads
    PT_INIT(&pt_finger_pos);
    PT_INIT(&pt_UI);
    
    // init the display
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9341_BLACK);
    //240x320 vertical display
    tft_setRotation(1); // Use tft_setRotation(1) for 320x240
    
    //UI
//    tft_fillRoundRect(0,0, 200, 50, 1, ILI9341_BLACK);// x,y,w,h,radius,color
//    tft_setCursor(0, 0);
//    tft_setTextColor(ILI9341_WHITE); tft_setTextSize(2);
//    tft_writeString("Press a key to play a song!");
//    tft_setCursor(30, 30);
//    tft_setTextColor(ILI9341_WHITE); tft_setTextSize(2);
//    tft_writeString("1) Mario Theme Song");
//    tft_setCursor(30, 60);
//    tft_setTextColor(ILI9341_WHITE); tft_setTextSize(2);
//    tft_writeString("2) Twinkle-Twinkle");
//    tft_setCursor(30, 90);
//    tft_setTextColor(ILI9341_WHITE); tft_setTextSize(2);
//    tft_writeString("   Little Star");
//    tft_setCursor(30, 120);
//    tft_setTextColor(ILI9341_WHITE); tft_setTextSize(2);
//    tft_writeString("3) Amazing Grace");
    
    // round-robin scheduler for threads
    while (1){
        PT_SCHEDULE(protothread_finger_pos(&pt_finger_pos));
        PT_SCHEDULE(protothread_UI(&pt_UI));
    }
  } // main
// === end  ======================================================
//*/
