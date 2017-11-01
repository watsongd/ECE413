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
#include "registers.h"

#define SAMPLE_RATE     15000        // 200us interval: Enough time for 1440 instructions at 72Mhz.
#define MAX_PWM         PBCLK/SAMPLE_RATE // eg 2667 at 15000hz.

int counter = 0;
static struct pt pt_display, pt_input, pt_controller, pt_UART;
        
static PT_THREAD (protothread_UART(struct pt *pt))
{
    PT_BEGIN(pt);  
    while(1) {
        PT_SPAWN(pt, &pt_input, PT_GetSerialBuffer(&pt_input) );
    } // END WHILE(1)
  PT_END(pt);
} // keypad thread


char stats1[30];
int desiredRPM = 0;
int currentRPM;
int integral;
int error;
int last_error;
int derivative;
int kp;
int ki;
int kd;
int pwm;
int PWMvalue1;
int PWMvalue2;
int samplerate;
int rtRPM = 0;
int t = 0;
int tprev, elapsedTime;

static PT_THREAD (protothread_display(struct pt *pt))
{
    PT_BEGIN(pt);  
    while(1) {
        tft_setCursor(20, 5);
        tft_setTextSize(1);
        //erase old text
        tft_setTextColor(ILI9341_BLACK);
        tft_writeString(stats1);
        
        //Draw new stats
        tft_setCursor(20, 5);
        tft_setTextColor(ILI9341_WHITE); tft_setTextSize(1);
        sprintf(stats1,"Desired RPM: %d, Real Time RPM: %d,", desiredRPM, rtRPM);
        tft_writeString(stats1);
        
        PT_YIELD_TIME_msec(200);
        

    } // END WHILE(1)
  PT_END(pt);
} // display thread

static PT_THREAD (protothread_controller(struct pt *pt))
{
    PT_BEGIN(pt);  
    while(1) {
        //from the IC, determine the current speed of the motor
        currentRPM = elapsedTime/40000000;
        
        //calculate how much correction is needed to reach the desired speed
        error = desiredRPM - currentRPM;
        
        //calculate the integral
        integral = integral + error;
        
        //calculate the derivative
        derivative = error - last_error;
        
        //calculate control variable
        pwm = (kp*error) + (ki*integral) + (kd*derivative);
        
        //if control variable is positive, speed up
        if (pwm > 0) {
            //speed up logic
        }
        
        //if control variable is negative, slow down
        if (pwm < 0) {
            //slow down logic
        }
        
        //set the current error as the last error for future computation
        last_error = error;
        PT_YIELD_TIME_msec(200);
    } // END WHILE(1)
  PT_END(pt);
} // controller thread




//Interrupt ISR =================================================
void __ISR(_INPUT_CAPTURE_1_VECTOR, ipl2soft) InputCapture1_Handler(void){
    tprev = t;
    t = mIC1ReadCapture();
    if (t > tprev)
        elapsedTime = t - tprev;
    //clear the interrupt flag
    INTClearFlag(INT_IC1);
}

void initTimers(void){
    //refresh rate
    OpenTimer23(T23_ON | T23_32BIT_MODE_ON | T23_PS_1_1 , 0xffffffff);
    ConfigIntTimer23(T23_INT_ON | T23_INT_PRIOR_1);
}

void capture_init(){
    INTEnable(INT_IC1, INT_ENABLED);
    INTSetVectorPriority(INT_INPUT_CAPTURE_1_VECTOR,
                         INT_PRIORITY_LEVEL_2);
}

/*
 * 
 */
int main(int argc, char** argv) {
    SYSTEMConfigPerformance(PBCLK);
    ANSELA = 0; ANSELB = 0; CM1CON = 0; CM2CON = 0;

    //Comparator Output 
    PPSOutput(1, RPA0, C2OUT);

    //Input Capture Input
    PPSInput(3, IC1, RPB13);
    mPORTBSetPinsAnalogIn(BIT_3);
    
    // Enable Input Capture Module 1
    OpenCapture1(IC_ON | IC_INT_1CAPTURE | IC_TIMER2_SRC | IC_EVERY_FALL_EDGE |
                 IC_CAP_32BIT | IC_FEDGE_FALL);
    
    //PPSInput
    PPSInput(2,U2RX, RPB11);
    PPSOutput(4, RPB10, U2TX);
    //UART Setup
    UARTConfigure(UART2, UART_ENABLE_PINS_TX_RX_ONLY);;
    UARTSetLineControl(UART2,UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    UARTSetDataRate(UART2, PBCLK, 9600);
    UARTEnable(UART2, UART_ENABLE_FLAGS | UART_PERIPHERAL | UART_RX | UART_TX);
    
    initTimers();
    capture_init();

    samplerate = SAMPLE_RATE;
    PR2 = PBCLK/samplerate-1;
    
    //OCRS
    PWMvalue1 = 3600;    // 50% modulation
    //OCR
    PWMvalue2 = 1800;    // 25% modulation
    
    INTEnableSystemMultiVectoredInt();         // make separate interrupts possible

    OpenOC1(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE,0,0); // init OC1 module, T2 =source 
    OpenOC2(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE,0,0); // init OC2 module, T2 =source
    
    //PT_INIT(&pt_input);
    PT_INIT(&pt_display);
    PT_INIT(&pt_UART);
    
    //initialize screen
    tft_init_hw();
    tft_begin();
    tft_setRotation(1); 
    tft_fillScreen(ILI9341_BLACK);  

    while(1){
        //PT_SCHEDULE(protothread_input(&pt_input));
        //PT_SCHEDULE(protothread_UART(&pt_UART));
        PT_SCHEDULE(protothread_display(&pt_display));
    }
    
    return (EXIT_SUCCESS);
}

////---------------------------------------------------------------------------------------------------------------
//void __ISR( _TIMER_2_VECTOR, ipl7) T2Interrupt(void) {
//    SetDCOC1PWM(PWMvalue1);     // these functions send a new value (0 to 7200) to PWM modulation with some PWM value.
//    SetDCOC2PWM(PWMvalue2);    // interrupt will fire again 7200 counts later than now  = 200us. (at 5 khz)
//    mT2ClearIntFlag();                  // clear this interrupt .
//} 
////-------------------------------------------------------------------------------------------------------------- 