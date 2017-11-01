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
static struct pt pt_display, pt_input, pt_controller;
        
static PT_THREAD (protothread_input(struct pt *pt))
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
int PWMOCRS;
int PWMOCR;
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
        
        //if control variable is positive, slow down
        if (pwm > 0) {
            //slow down logic (change duty cycle)
            //PWMOCRS = PWMOCRS - 
            SetDCOC2PWM(PWMOCRS);
        }
        
        //if control variable is negative, speed up 
        if (pwm < 0) {
            //speed up logic (change duty cycle)
            //SetDCOC2PWM(PWMOCRS);
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
    
    initTimers();
    capture_init();

    samplerate = SAMPLE_RATE;
    PR2 = 65536-1;
    
    //OCRS
    PWMOCRS = 32768;    // 50% modulation
    //OCR
    PWMOCR = 32768;    // 50% modulation
    
    INTEnableSystemMultiVectoredInt();         // make separate interrupts possible

    OpenOC2(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE, PWMOCRS, PWMOCR); // init OC2 module, T3 =source 
    //OpenOC1(OC_ON | OC_TIMER3_SRC | OC_PWM_FAULT_PIN_DISABLE,PWMOCRS,PWMOCR); // init OC1 module, T3 =source
    
    //PT_INIT(&pt_input);
    PT_INIT(&pt_display);
    
    //initialize screen
    tft_init_hw();
    tft_begin();
    tft_setRotation(1); 
    tft_fillScreen(ILI9341_BLACK);  

    while(1){
        //PT_SCHEDULE(protothread_input(&pt_input));
        PT_SCHEDULE(protothread_display(&pt_display));
        // you send new values to PWMs  from here based on controller
        // via variables PWMOCRS & PWMOCR 
        // e.g.
        // PWMOCRS = some new value;
        // PWMOCR = some new other value;     (Interrupt will pick them up from global vars).
    }
    
    return (EXIT_SUCCESS);
}