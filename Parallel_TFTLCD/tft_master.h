/*  Author: Matthew Watkins
 *  Basic LCD functions for TFT LCD for PIC32
 *
 *  Modified from Adafruit Parallel 2.4" TFT LCD targetting Arduino
 *
 */

// Graphics library by ladyada/adafruit with init code from Rossum
// MIT license

#ifndef _ADAFRUIT_TFTLCD_H_
#define _ADAFRUIT_TFTLCD_H_

typedef int int32_t;
typedef unsigned int uint32_t;

#include <stdint.h> //for 16 and 8 bit types
#include "plib.h"
#include "pin_magic.h"

#define ILI9341_TFTWIDTH  240
#define ILI9341_TFTHEIGHT 320

// Color definitions
#define ILI9341_BLACK       0x0000      /*   0,   0,   0 */
#define ILI9341_NAVY        0x000F      /*   0,   0, 128 */
#define ILI9341_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define ILI9341_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define ILI9341_MAROON      0x7800      /* 128,   0,   0 */
#define ILI9341_PURPLE      0x780F      /* 128,   0, 128 */
#define ILI9341_OLIVE       0x7BE0      /* 128, 128,   0 */
#define ILI9341_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define ILI9341_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define ILI9341_BLUE        0x001F      /*   0,   0, 255 */
#define ILI9341_GREEN       0x07E0      /*   0, 255,   0 */
#define ILI9341_CYAN        0x07FF      /*   0, 255, 255 */
#define ILI9341_RED         0xF800      /* 255,   0,   0 */
#define ILI9341_MAGENTA     0xF81F      /* 255,   0, 255 */
#define ILI9341_YELLOW      0xFFE0      /* 255, 255,   0 */
#define ILI9341_WHITE       0xFFFF      /* 255, 255, 255 */
#define ILI9341_ORANGE      0xFD20      /* 255, 165,   0 */
#define ILI9341_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define ILI9341_PINK        0xF81F
#define ILI9341_BROWN       0xBBCA   //0x82A7
#define ILI9341_TAN         0xEF36


#define PBCLK 40000000 // peripheral bus clock
#define SPI_freq    20000000

#define tabspace 4 // number of spaces for a tab

#define dTime_ms PBCLK/2000
#define dTime_us PBCLK/2000000

  void     tft_begin(); //assuming ID of 0x9341
  void     tft_begin_id(uint16_t id);
  void     tft_init_hw(void);
  void     tft_drawPixel(int16_t x, int16_t y, uint16_t color);
  void     tft_drawFastHLine(int16_t x0, int16_t y0, int16_t w, uint16_t color);
  void     tft_drawFastVLine(int16_t x0, int16_t y0, int16_t h, uint16_t color);
  void     tft_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c);
  void     tft_fillScreen(uint16_t color);
  uint16_t tft_color565(uint8_t r, uint8_t g, uint8_t b);
  void     tft_setRotation(uint8_t x);

  void     tft_reset(void);
  void     tft_setRegisters8(uint8_t *ptr, uint8_t n);
  void     tft_setRegisters16(uint16_t *ptr, uint8_t n);

       // These methods are public in order for BMP examples to work:
  inline void     tft_setAddrWindow(int x1, int y1, int x2, int y2);
  void     tft_pushColors(uint16_t *data, uint8_t len, int first);


  //uint16_t tft_readPixel(int16_t x, int16_t y);
  uint16_t tft_readID();
  uint32_t tft_readReg(uint8_t r);

uint16_t _width, _height;
uint8_t rotation;

void delay_ms(unsigned long);
void delay_us(unsigned long);

#ifndef tft_writeRegister8
           void tft_writeRegister8(uint8_t a, uint8_t d);
#endif
#ifndef tft_writeRegister16
           void tft_writeRegister16(uint16_t a, uint16_t d);
#endif
    void tft_writeRegister24(uint8_t a, uint32_t d);
    void tft_writeRegister32(uint8_t a, uint32_t d);
#ifndef tft_writeRegisterPair
    void tft_writeRegisterPair(uint8_t aH, uint8_t aL, uint16_t d);
#endif
    void tft_setLR();
    void tft_flood(uint16_t color, uint32_t len);
  uint8_t  driver;

#ifndef read8
  uint8_t  read8fn(void);
  #define  read8isFunctionalized
#endif


#endif
