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
#include "adc_intf.h"
#include "TouchScreen.h"
#include "tft_master.h"
#include "tft_gfx.h"
#include "pt_cornell_1_2.h"
#include "mole.h"
#include "stdbool.h"


#define XM AN0
#define YP AN1

//.4 in fixed point 10.6
#define xStep 0x0019
#define yStep 0x0016

struct TSPoint p;
int misses, hits, maxMoles, moleDur;
int prev_misses, prev_hits;
uint16_t time = 0;
uint16_t gameTime = 0;
bool changes;
int touched;

static struct pt pt_game_ctr;
        
        
static PT_THREAD (protothread_game_ctr(struct pt *pt))
{
    PT_BEGIN(pt);  
    while(1) {
        if (gameTime > 60){
            tft_drawRect(100, 100, 120, 20, ILI9341_GREEN);
        }
        PT_YIELD_TIME_msec(1);
        // NEVER exit while
    } // END WHILE(1)
    
  PT_END(pt);
} // keypad thread

int checkDurationPot(){
    mPORTBSetPinsAnalogIn(BIT_13);
    int moleDur = readADC(11);
    return moleDur;
 }

int xScale(int16_t xTouchScreen){
    uint16_t xScreenPos;
    
    if (xTouchScreen == 0) return 0;
    //(TouchScreen cord - 100) * .4
    xScreenPos = (xTouchScreen - 100)*xStep;
    xScreenPos = (xScreenPos>>6)&0x03ff;
    return xScreenPos;
}

int yScale(int16_t yTouchScreen){
    uint16_t yScreenPos;
    
    if (yTouchScreen == 0) return 0;
    //(TouchScreen cord - 100) * .4
    yScreenPos = (yTouchScreen - 180)*yStep;
    yScreenPos = (yScreenPos>>6)&0x03ff;
    return yScreenPos;
}

uint32_t seed = 7;
uint16_t num;

//Returns a number 0-512
static uint32_t random()
{
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    
    num = seed & 0x01ff;
    //x needs scaled by factor of .625
    //y needs scaled by factor of .469
    return num;
}
char stats[30];
void updateUI(){
    if (changes == true){
        tft_setCursor(20, 5);
        tft_setTextSize(1);
        //erase old text
        tft_setTextColor(ILI9341_BLACK);
        tft_writeString(stats);         
        //Draw new stats
        tft_setCursor(20, 5);
        tft_setTextColor(ILI9341_WHITE); tft_setTextSize(1);
        sprintf(stats,"Time: %d, Hits: %d, Misses: %d", gameTime, hits, misses);
        tft_writeString(stats);
        changes = false;
     }
 }

void __ISR(_TIMER_23_VECTOR, ipl2) T23Int(void){
    //Refresh code here. Runs at 60Hz
    getPoint(&p);
    time++;
    if (time == 60){
        gameTime++;
        time = 0;
        changes = true;
    }
   if(p.z>0 && touched == 0) {
        touched = 1;
        if (checkIfTouched(xScale(p.x), yScale(p.y))) {
            hits++;
            changes = true;
        }
    }
    
    if(p.z==0) touched = 0;
    
    if (checkDurations() == true){
        changes = true;
        misses++;
        //addMole(100, 100, 10000);
    }
    
    updateUI();
    mT23ClearIntFlag();
}
    
void __ISR(_TIMER_1_VECTOR, ipl2) T1Int(void){
    //Decrement Mole durations here. Runs at 1000Hz
//    
    decrementDurations(); 
    checkDurations();
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
/*
 * 
 */
int main(int argc, char** argv) {
    mPORTBSetPinsAnalogIn(BIT_15);
    char buffer[30];
    char buffer1[30];
    char buffer2[30];
    SYSTEMConfigPerformance(PBCLK);
            
    initArray();
    initTimers();
    INTEnableSystemMultiVectoredInt();
    
    configureADC();
    PT_INIT(&pt_game_ctr);
    
    //initialize screen
    tft_init_hw();
    tft_begin();
    tft_setRotation(1); 
    tft_fillScreen(ILI9341_BLACK);  
    addMole(30,30, 10000);
    addMole(90, 30, 15000);
    addMole(150,30, 15000);
    while(1){
        PT_SCHEDULE(protothread_game_ctr(&pt_game_ctr));
        
        //tft_fillScreen(ILI9341_BLACK);
//        tft_setCursor(20, 100);
//        tft_setTextColor(ILI9341_WHITE); tft_setTextSize(2);
//
//        //erase old text
//        tft_setTextColor(ILI9341_BLACK);
//        tft_writeString(buffer);
//        tft_setCursor(20, 120);
//        tft_writeString(buffer1);
//        tft_setCursor(20, 140);
//        tft_writeString(buffer2);
//        
//        p.x = 0;
//        p.y = 0;
//        p.z = 0;
//        getPoint(&p);
//        tft_setCursor(20, 100);
//        tft_setTextColor(ILI9341_WHITE);
//        sprintf(buffer,"x: %d, y: %d, z: %d", p.x, p.y, p.z);
//        sprintf(buffer1,"x: %d, y: %d, rand: %d", xScale(p.x), yScale(p.y), random());
//        sprintf(buffer2,"Time: %d, MOLECOUNT: %d", gameTime, countMoles());
//        tft_writeString(buffer);
//        
//        tft_setCursor(20, 120);
//        tft_setTextColor(ILI9341_WHITE);
//        tft_writeString(buffer1);
//        
//        tft_setCursor(20, 140);
//        tft_setTextColor(ILI9341_WHITE);
//        tft_writeString(buffer2);
        
        delay_ms(100);
    }
    
    return (EXIT_SUCCESS);
}

