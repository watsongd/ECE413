// Basic setup for keeping track of moles (when to remove and add)
// and their coordinates
#include "mole.h"
#include "plib.h"
#include "xc.h"
#include "adc_intf.h"

void tft_downArc(short x0, short y0, short r, short w, unsigned short color) {
/* Draw a circle outline with center (x0,y0) and radius r, with given color
 * Parameters:
 *      x0: x-coordinate of center of arc. The top-left of the screen
 *          has x-coordinate 0 and increases to the right
 *      y0: y-coordinate of center of arc. The top-left of the screen
 *          has y-coordinate 0 and increases to the bottom
 *      r:  radius of arc
 *      w:  width from center of arc
 *      color: 16-bit color value for the circle. Note that the circle
 *          isn't filled. So, this is the color of the outline of the circle
 * Returns: Nothing
 */
  short f = 1 - r;
  short ddF_x = 1;
  short ddF_y = -2 * r;
  short x = 0;
  short y = r;

  tft_drawPixel(x0  , y0-r, color);

  while (x<y && x < w) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    tft_drawPixel(x0 + x, y0 - y, color);
    tft_drawPixel(x0 - x, y0 - y, color);
  }
}

void addMole(int16_t x, int16_t y, int16_t duration) {
    
    //draw the mole
    tft_fillCircle(x, y, MOLERADIUS, ILI9341_BROWN);
    tft_fillCircle(x, y+(MOLERADIUS/2-4), MOLERADIUS/2-2, ILI9341_TAN);
    tft_fillCircle(x, y, MOLERADIUS/6, ILI9341_BLACK);
    tft_fillCircle(x-(MOLERADIUS/3+1), y-(MOLERADIUS/3+1), MOLERADIUS/6, ILI9341_BLACK); //left eye
    tft_fillCircle(x+(MOLERADIUS/3+1), y-(MOLERADIUS/3+1), MOLERADIUS/6, ILI9341_BLACK); //right eye
    tft_downArc(x, y+(3*MOLERADIUS/4), MOLERADIUS/2-2, MOLERADIUS/3-1, ILI9341_BLACK);
    
    int i;
    //add the mole to the array
    for(i =0; i < 16; i++) {
        //find the first available spot in the array
        if (list[i].duration == 0) {
            list[i].duration = duration + 1;
            list[i].y = y;
            list[i].x = x;
            break;
        }    
    }
}
void initArray(){
    int i;
    //add the mole to the array
    for(i =0; i < 16; i++) {
        list[i].duration = 0;
        list[i].y = 0;
        list[i].x = 0;
        
    }
}
int countMoles() {
    
    int count = 0;
    int i;
    for(i = 0; i < 16; i++) {
        if (list[i].duration != 0) {
            count++;
        }
    }
    return count;
}

bool checkNoOverlap(int16_t x, int16_t y) {
    
    //First, make sure the entire mole can be drawn on the screen
    if (x < MOLERADIUS+1 || x > x-MOLERADIUS+1 ) {
        return false;
    }
    if (y < MOLERADIUS+1 || y > y-MOLERADIUS+1 ) {
        return false;
    }
    //Next, check that it doesn't overlap with the other moles
    if (countMoles() > 0) {
        int i;
        for(i = 0; i < 16; i++) {
            
            if (list[i].duration != 0) {
                int xTerm = (x - list[i].x)*(x - list[i].x);
                int yTerm = (y - list[i].y)*(y - list[i].y);
                int rTerm = (MOLERADIUS*2)*(MOLERADIUS*2);
                if ((xTerm + yTerm) < rTerm){
                    return false;
                }
            }
        }
    }
    else {
        return true;
    }
}

void decrementDurations() {
    
    int i;
    for(i = 0; i < 16; i++) {
        if (list[i].duration != 0) {
            list[i].duration = list[i].duration - 1;
        }
    }
}

bool checkDurations() {
    
    int i;
    for(i = 0; i < 16; i++) {
        if (list[i].duration == 1) {
            removeMole(&list[i]);
            return true;
        }
    }
    return false;
}

bool checkIfTouched(int16_t x, int16_t y) {
    
    int i;
    for(i = 0; i < 16; i++) {
        if (list[i].duration != 0) {
            int xTerm = (x - list[i].x)*(x - list[i].x);
            int yTerm = (y - list[i].y)*(y - list[i].y);
            int rTerm = (MOLERADIUS+1)*(MOLERADIUS+1);
            if ((xTerm + yTerm) < rTerm){
                removeMole(&list[i]);
                return true;
            }
        }
    }
    return false;
}

void removeMole(struct mole * m) {
    
    int i;
    for(i = 0; i < 16; i++) {
        if (list[i].duration == m->duration && list[i].x == m->x && list[i].y == m->y) {
            //remove mole from the screen
            tft_fillCircle(m->x, m->y, MOLERADIUS, ILI9341_BLACK);
            //remove mole from the list
            list[i].duration = 0;
            list[i].x        = 0;
            list[i].y        = 0;
        }
    }
}

void removeAllMoles() {
    
    int i;
    for(i = 0; i < 16; i++) {
        //remove mole from the screen
        tft_fillCircle(list[i].x, list[i].y, MOLERADIUS, ILI9341_BLACK);
        //remove mole from the list
        list[i].duration = 0;
        list[i].x        = 0;
        list[i].y        = 0;
    }
}