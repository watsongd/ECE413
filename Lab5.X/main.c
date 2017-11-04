/* 
 * File:   main.c
 * Author: Connor Nace & Geoff Watson
 *
 * Created on September 18, 2015, 10:11 AM
 */

//#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "plib.h"
#include "xc.h"
#include "tft_master.h"
#include "tft_gfx.h"
#include "pt_cornell_1_2.h"
#include <math.h>

#define use_uart_serial

int counter = 0;
static struct pt pt_display, pt_input, pt_controller, pt_UART;

// Block o' globals
int desiredRPM = 2500;
int rtRPM = 0;
char c;
float value;
char stats1[30];
char stats2[30];
int currentRPM;
int integral = 0;
int error = 0;
int last_error = 0;
int derivative = 0;
float kp = 1;
float ki = 1;
float kd = 1;
int samplerate;
int t = 0;
int elapsedTime = 0;
int possibleElapsedTime = 0;
int tprev;

// proto-thread for UART setup
static PT_THREAD (protothread_UART(struct pt *pt))
{
    PT_BEGIN(pt);  
    while(1) {
        //printf('Printing to the terminal /n/r');
        PT_SPAWN(pt, &pt_input, PT_GetSerialBuffer(&pt_input));
        printf("%s\n\r", PT_term_buffer);
        sscanf(PT_term_buffer, "%c %f", &c, &value);
        if (c == 's') desiredRPM = (int)value;
        if (c == 'p') kp = value;
        if (c == 'i') ki = value;
        if (c == 'd') kd = value;
        printf ("s = %d, p = %f, i = %f, d = %f\n\r", desiredRPM, kp, ki, kd);
        PT_YIELD_TIME_msec(30);
    } // END WHILE(1)
  PT_END(pt);
} // UART thread

//Proto-thread that controls the display.  currently displayed is the desired RPM,
// current RPM, KP, KI, and KD control variables
static PT_THREAD (protothread_display(struct pt *pt))
{
    PT_BEGIN(pt);  
    while(1) {
        tft_setCursor(20, 5);
        tft_setTextSize(1);
        //erase old text
        tft_setTextColor(ILI9341_BLACK);
        tft_writeString(stats1);
        
        tft_setCursor(20, 15);
        tft_setTextSize(1);
        //erase old text
        tft_setTextColor(ILI9341_BLACK);
        tft_writeString(stats2);
        
        //Draw new stats
        tft_setCursor(20, 5);
        tft_setTextColor(ILI9341_WHITE); tft_setTextSize(1);
        sprintf(stats1,"desired RPM: %d, current RPM: %d,", desiredRPM, currentRPM);
        sprintf(stats2,"Kp: %f, Ki: %f, Kd: %f,", kp,ki,kd);
        tft_writeString(stats1);
        
        tft_setCursor(20,15);
        tft_setTextColor(ILI9341_WHITE); tft_setTextSize(1);
        tft_writeString(stats2);
        PT_YIELD_TIME_msec(100);

    } // END WHILE(1)
  PT_END(pt);
} // display thread

int *sub = 0;

// This is the PID controller for the motor.  derivative, integral, and error are
//all calculated based on the most recent output
static PT_THREAD (protothread_controller(struct pt *pt))
{
    PT_BEGIN(pt);  
    while(1) {
        //from the IC, determine the current speed of the motor
        if (elapsedTime > 0) {
            currentRPM = (int)(2400000000/elapsedTime);
        }
        else {
            currentRPM = 0;
        }
        //calculate how much correction is needed to reach the desired speed
        error = desiredRPM - currentRPM;
        
        //calculate the integral
        integral = integral + error;
        
        //calculate the derivative
        derivative = error - last_error;
        
        //calculate control variable
        int kpTerm = (kp*error); 
        int kiTerm = (ki*integral);
        int kdTerm = (kd*derivative);
        int pwm = kpTerm + kdTerm;
        
        /*This is where we think the error occurs */
        //change the duty cycle based on the control variable
        if ((pwm < -100) || (pwm > 100)) {
            sub = sub + pwm;
            SetDCOC1PWM(sub);
        }
        else {
            SetDCOC1PWM(sub);
        }
        if (sub >= 65535) {
            sub = 65535;
        }
        
        if (sub <= 0) {
            sub = 0;
        }
     
        //set the current error as the last error for future computation
        last_error = error;
        PT_YIELD_TIME_msec(100);
    } // END WHILE(1)
  PT_END(pt);
} // controller thread

//Interrupt ISR =================================================
void __ISR(_INPUT_CAPTURE_1_VECTOR) InputCapture1_Handler(void){
    tprev = t;
    t = mIC1ReadCapture();
    if (t > tprev) {
        possibleElapsedTime = t - tprev;
        //needed for filtering out bad input capture reads
        if ((currentRPM > 100) && (possibleElapsedTime < 800000)) {
            elapsedTime = elapsedTime;
        }
        else if ((currentRPM > 100) && (possibleElapsedTime > 4800000)) {
            elapsedTime = elapsedTime;
        }
        else {
            elapsedTime = possibleElapsedTime;            
        }
    }
    else {
        elapsedTime = elapsedTime;
    }
    //clear the interrupt flag
    INTClearFlag(INT_IC1);
}

//Initialize the timer in 32 bit mode
void initTimers(void){
    //refresh rate
    OpenTimer23(T23_ON | T23_32BIT_MODE_ON | T23_PS_1_1 , 0xffffffff);
    //ConfigIntTimer23(T23_INT_ON | T23_INT_PRIOR_1);
}

//Setup the input capture interrupt
void capture_init(){
    INTEnable(INT_IC1, INT_ENABLED);
    INTSetVectorPriority(INT_INPUT_CAPTURE_1_VECTOR,
                         INT_PRIORITY_LEVEL_2);
}

///*
// * 
// */
int main(int argc, char** argv) {
    SYSTEMConfigPerformance(PBCLK);
 
    // Enable Input Capture Module 1
    OpenCapture1(IC_ON | IC_INT_1CAPTURE | IC_TIMER2_SRC | IC_EVERY_FALL_EDGE |
                 IC_CAP_32BIT | IC_FEDGE_FALL);
    
    initTimers();
    capture_init();
    
    // turns OFF UART support and debugger pin
    PT_setup();
    
    INTEnableSystemMultiVectoredInt();         // make separate interrupts possible

    OpenOC1(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE, 0, 0); // init OC1 module, T2 =source 
    
    // Output Compare
    PPSOutput(1, RPA0, OC1);
    
    //Input Capture Input
    PPSInput(3, IC1, RPB13);
    
    PT_INIT(&pt_controller);
    PT_INIT(&pt_display);
    PT_INIT(&pt_UART);
    
    //initialize screen
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9341_BLACK);  
    //240x320 vertical display
    tft_setRotation(3); // Use tft_setRotation(1) for 320x240

    while(1){
        PT_SCHEDULE(protothread_controller(&pt_controller));
        PT_SCHEDULE(protothread_UART(&pt_UART));
        PT_SCHEDULE(protothread_display(&pt_display));
    }
    
    return (EXIT_SUCCESS);
}