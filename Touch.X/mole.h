/* 
 * File:   mole.h
 * Author: watsongd
 *
 * Created on October 5, 2017, 2:42 PM
 */

#ifndef MOLE_H
#define	MOLE_H
#include <stdint.h>
#include <stdbool.h>

#define ILI9341_BROWN       0xBBCA   //0x82A7
#define ILI9341_TAN         0xEF36
#define ILI9341_BLACK       0x0000
#define MOLERADIUS 24

typedef short int16_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

// an individual mole has a duration(remaining), x and y coordinate, and 
// an index(may not need later if we are smart about removing from array)
struct mole {
  int16_t duration, x, y;
};

// array of moles
struct mole list[16];


void initArray();

// check if the random x and y value overlap with a current mole and
// check if the entire mole can draw a mole on the screen
bool checkNoOverlap(int16_t x, int16_t y);

// add a mole to screen (assuming that overlap is already dealt with)
void addMole(int16_t x0, int16_t y0, int16_t duration);

//remove a mole from the screen and black out where it previously was
// and draw a black circle on the mole
void removeMole(struct mole * m);

// Count the number of moles in the array
int countMoles();

// subtracts one from every moles duration
void decrementDurations();

// Check all moles to see if any in the array have a duration of 0 
// and call removeMole when one is found
bool checkDurations();

// Takes in the x and y values from the touch and see if a mole was hit
bool checkIfTouched(int16_t x, int16_t y);

// Professor Watkin's down arc code for the frown on the mole
void tft_downArc(short x0, short y0, short r, short w, unsigned short color);


#endif	/* MOLE_H */

