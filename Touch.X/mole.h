/* 
 * File:   mole.h
 * Author: watsongd
 *
 * Created on October 5, 2017, 2:42 PM
 */

#ifndef MOLE_H
#define	MOLE_H
#include <stdint.h>

typedef short int16_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

// an individual mole has a duration(remaining), x and y coordinate, and 
// an index(may not need later if we are smart about removing from array)
struct mole {
  int16_t duration, x, y, index;
};

// array of moles
struct mole list[16];

// add a mole to screen (assuming that overlap is already dealt with)
void addMole(int16_t x0, int16_t y0, int16_t duration);

//remove a mole from the screen and black out where it previously was
void removeMole(struct mole * m);

#endif	/* MOLE_H */

