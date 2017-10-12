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


#define XM AN0
#define YP AN1

//.4 in fixed point 10.6
#define xStep 0x0019
#define yStep 0x0016

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
uint16_t time = 0;
uint16_t gameTime = 0;
void __ISR(_TIMER_23_VECTOR, ipl2) T23Int(void){
    //Refresh code here. Runs at 60Hz
//    time++;
//    if (time = 60){
//        gameTime++;
//        time = 0;
//    }
    mT23ClearIntFlag();
}
    
void __ISR(_TIMER_1_VECTOR, ipl2) T1Int(void){
    //Decrement Mole durations here. Runs at 1000Hz
    time++;
    if (time == 1000){
        gameTime++;
        time = 0;
    }
    mT1ClearIntFlag();
}

void initTimers(void){
    OpenTimer1(T1_ON | T1_SOURCE_INT | T1_PS_1_1, 40000);
    // Configure T1 for ms count
    ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_2);
    
//      //refresh rate
//    OpenTimer23(T23_ON | T23_SOURCE_INT | T23_32BIT_MODE_ON | T23_PS_1_1 , 666667);
//    ConfigIntTimer23(T23_INT_ON | T23_INT_PRIOR_1);
}
/*
 * 
 */
int main(int argc, char** argv) {
    char buffer[30];
    char buffer1[30];
    char buffer2[30];
    SYSTEMConfigPerformance(PBCLK);
    
    initTimers();
    
    configureADC();
    
    //initialize screen
    tft_init_hw();
    tft_begin();
    tft_setRotation(1); 
    tft_fillScreen(ILI9341_BLACK);  
    

    
    while(1){
        //tft_fillScreen(ILI9341_BLACK);
        tft_setCursor(20, 100);
        tft_setTextColor(ILI9341_WHITE); tft_setTextSize(2);

        //erase old text
        tft_setTextColor(ILI9341_BLACK);
        tft_writeString(buffer);
        tft_setCursor(20, 120);
        tft_writeString(buffer1);
        tft_setCursor(20, 140);
        tft_writeString(buffer1);
        
        struct TSPoint p;
        p.x = 0;
        p.y = 0;
        p.z = 0;
        getPoint(&p);
        tft_setCursor(20, 100);
        tft_setTextColor(ILI9341_WHITE);
        sprintf(buffer,"x: %d, y: %d, z: %d", p.x, p.y, p.z);
        sprintf(buffer1,"x: %d, y: %d, rand: %d", xScale(p.x), yScale(p.y), random());
        sprintf(buffer2,"Time: %d", gameTime);
        tft_writeString(buffer);
        
        tft_setCursor(20, 120);
        tft_setTextColor(ILI9341_WHITE);
        tft_writeString(buffer1);
        
        tft_setCursor(20, 140);
        tft_setTextColor(ILI9341_WHITE);
        tft_writeString(buffer2);
        
        delay_ms(100);
    }
    
    return (EXIT_SUCCESS);
}
