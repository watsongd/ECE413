/* 
 * File:   touch_main.c
 * Author: watkinma
 *
 * Created on September 18, 2015, 10:11 AM
 */

//#include <stdio.h>
//#include <stdlib.h>
#include "config.h"
#include "plib.h"
#include "xc.h"
#include "tft_master.h"
#include "tft_gfx.h"
#include "pt_cornell_1_2.h"

int counter = 0;
static struct pt pt_game_ctr;
        
static PT_THREAD (protothread_game_ctr(struct pt *pt))
{
    PT_BEGIN(pt);  
    while(1) {

    } // END WHILE(1)
  PT_END(pt);
} // keypad thread

void __ISR(_TIMER_23_VECTOR, ipl2) T23Int(void){
    //Refresh code here. Runs at 60Hz
    mT23ClearIntFlag();
}
    
void __ISR(_TIMER_1_VECTOR, ipl2) T1Int(void){
    mT1ClearIntFlag();
}

void initTimers(void){
    OpenTimer1(T1_ON | T1_SOURCE_INT | T1_PS_1_1, 40000);
    // Configure T1 for ms count
    ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_2);
    
    //refresh rate
    OpenTimer23(T23_ON | T23_SOURCE_INT | T23_32BIT_MODE_ON | T23_PS_1_1 , 666667);
    ConfigIntTimer23(T23_INT_ON | T23_INT_PRIOR_1);
}

void capture_init(){
    INTEnable(INT_IC1, INT_ENABLED);
    INTSetVectorPriority(INT_INPUT_CAPTURE_1_VECTOR,
            INT_PRIORITY_LEVEL_2);
      // === setup system wide interrupts  ========
    INTEnableSystemMultiVectoredInt();
    
}
/*
 * 
 */
int main(int argc, char** argv) {
    SYSTEMConfigPerformance(PBCLK);
    ANSELA = 0; ANSELB = 0; CM1CON = 0; CM2CON = 0;
    //TRISB = 0x4000; //TRISA = 0x0020;
    //CNPDB = 0x0780;
  
    //Comparator Output 
    PPSOutput(1, RPA0, C2OUT);

    //Input Capture Input
    PPSInput(3, IC1, RPB13)
            
    initTimers();
    capture_init();
    INTEnableSystemMultiVectoredInt();
    
    PT_INIT(&pt_game_ctr);
    
    //initialize screen
    tft_init_hw();
    tft_begin();
    tft_setRotation(1); 
    tft_fillScreen(ILI9341_BLACK);  

    while(1){
        PT_SCHEDULE(protothread_game_ctr(&pt_game_ctr));
        delay_ms(100);
    }
    
    return (EXIT_SUCCESS);
}
