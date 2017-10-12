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

#include "math.h"

// threading library
// config.h sets 40 MHz
#define	SYS_FREQ 40000000
#include "pt_cornell_1_2.h"
// string buffer
char buffer[60];

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_keypad, pt_notes;

// system 1 second interval tick
int sys_time_seconds ;

void initTimers(void){
    //OpenTimer1(T1_ON | T1_PS_1_1, PR);
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
    ANSELA = 0; ANSELB = 0; CM1CON = 0; CM2CON = 0;
    //TRISB = 0x4000; //TRISA = 0x0020;
  
    PT_setup(); 
  
    initTimers();
    INTEnableSystemMultiVectoredInt();

    // init the display
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9341_BLACK);
    //240x320 vertical display
    tft_setRotation(3); // Use tft_setRotation(1) for 320x240
    
    //UI
    
    // round-robin scheduler for threads
    while (1){
    }
  } // main
// === end  ======================================================
//*/
