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

volatile uint32_t accum_amt;

volatile uint8_t counter = 0;

// string buffer
char buffer[60];

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

static PT_THREAD (protothread_keypad(struct pt *pt))
{
    PT_BEGIN(pt);
    static int pad, i, pattern;
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
            pad  = mPORTBReadBits(BIT_7 | BIT_8 | BIT_9);
            if(pad!=0) {pad |= pattern ; break;}
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

        // draw key number
        tft_fillRoundRect(30,200, 100, 28, 1, ILI9341_BLACK);// x,y,w,h,radius,color
        tft_setCursor(30, 200);
        tft_setTextColor(ILI9341_YELLOW); tft_setTextSize(4);
        sprintf(buffer,"%d", i);
        if (i==10)sprintf(buffer,"*");
        if (i==11)sprintf(buffer,"#");
        tft_writeString(buffer);

        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // keypad thread

//Note Thread ===================================================
static PT_THREAD (protothread_notes(struct pt *pt))
{
    PT_BEGIN(pt);
        while(1) {

            accum_amt = C4;
            tft_fillCircle(40, 40, 20, ILI9341_BLACK);
            PT_YIELD_TIME_msec(500);
            accum_amt = G6;
            tft_fillCircle(40, 40, 20, ILI9341_WHITE);
            PT_YIELD_TIME_msec(500);
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
}

uint32_t accum = 0;
void __ISR(_TIMER_1_VECTOR, ipl2) T1Int(void){
    //accum_amt = G4;
    accum += accum_amt;
    //uint32_t f_int  = accum_amt & int_mask;
    //uint32_t f_frac = accum_amt & frac_mask;

    int16_t sine_val = 2048+sineTable[(accum>>24)&0xff];
    //int32_t accum = 0x3000;
    
    
    writeDAC(0x3000 | sine_val); // write to channel A, gain = 1
    //writeDAC(0xB000 | triangleTable[counter]); // write to channel B, gain = 1
    //counter += (f_int + f_frac);
    //counter += 3;
    if (((accum>>24)&0xff) == TABLE_SIZE) accum = 0;
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
  
  // Setup Timers
 OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1 , 238);
  //================================================
    int i,j;
   
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
  
  // round-robin scheduler for threads
  while (1){
        PT_SCHEDULE(protothread_keypad(&pt_keypad));
        PT_SCHEDULE(protothread_notes(&pt_notes));
        }
  } // main
// === end  ======================================================
//*/
