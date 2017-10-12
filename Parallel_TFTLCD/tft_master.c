/*  Author: Matthew Watkins
 *  Basic LCD functions for TFT LCD for PIC32
 *  Original code supported LCDs with different ids. To improve
 *  performance for the targetted 2.4" TFT, certain critical functions
 *  have been hardcoded to assume the ID_9341 protocol.
 *
 *  Modified from Adafruit Parallel 2.4" TFT LCD targetting Arduino
 *
 */

// Graphics library by ladyada/adafruit with init code from Rossum
// MIT license

#include "tft_master.h"
#include "pin_magic.h"

//#define TFTWIDTH   320
//#define TFTHEIGHT  480

#define TFTWIDTH   240
#define TFTHEIGHT  320

// LCD controller chip identifiers
#define ID_932X    0
#define ID_7575    1
#define ID_9341    2
#define ID_HX8357D    3
#define ID_UNKNOWN 0xFF

#include "registers.h"

// Initialization common to both shield & breakout configs
void tft_init_hw(void) {

    CS_IDLE;
    WR_IDLE;
    RD_IDLE;
    CD_DATA;
    RESET_HIGH;
    mPORTBSetPinsDigitalOut(RD_BIT | WR_BIT | CD_BIT | CS_BIT | RESET_BIT);
    
  setWriteDir(); // Set up LCD data port(s) for WRITE operations

  rotation  = 0;
  //cursor_y  = cursor_x = 0;
  //textsize  = 1;
  //textcolor = 0xFFFF;
  _width    = TFTWIDTH;
  _height   = TFTHEIGHT;
}

// Initialization command tables for different LCD controllers
#define TFTLCD_DELAY 0xFF
static const uint8_t HX8347G_regValues[] = {
  0x2E           , 0x89,
  0x29           , 0x8F,
  0x2B           , 0x02,
  0xE2           , 0x00,
  0xE4           , 0x01,
  0xE5           , 0x10,
  0xE6           , 0x01,
  0xE7           , 0x10,
  0xE8           , 0x70,
  0xF2           , 0x00,
  0xEA           , 0x00,
  0xEB           , 0x20,
  0xEC           , 0x3C,
  0xED           , 0xC8,
  0xE9           , 0x38,
  0xF1           , 0x01,

  // skip gamma, do later

  0x1B           , 0x1A,
  0x1A           , 0x02,
  0x24           , 0x61,
  0x25           , 0x5C,
  
  0x18           , 0x36,
  0x19           , 0x01,
  0x1F           , 0x88,
  TFTLCD_DELAY   , 5   , // delay 5 ms
  0x1F           , 0x80,
  TFTLCD_DELAY   , 5   ,
  0x1F           , 0x90,
  TFTLCD_DELAY   , 5   ,
  0x1F           , 0xD4,
  TFTLCD_DELAY   , 5   ,
  0x17           , 0x05,

  0x36           , 0x09,
  0x28           , 0x38,
  TFTLCD_DELAY   , 40  ,
  0x28           , 0x3C,

  0x02           , 0x00,
  0x03           , 0x00,
  0x04           , 0x00,
  0x05           , 0xEF,
  0x06           , 0x00,
  0x07           , 0x00,
  0x08           , 0x01,
  0x09           , 0x3F
};

static const uint8_t HX8357D_regValues[] = {
  HX8357_SWRESET, 0,
  HX8357D_SETC, 3, 0xFF, 0x83, 0x57,
  TFTLCD_DELAY, 250,
  HX8357_SETRGB, 4, 0x00, 0x00, 0x06, 0x06,
  HX8357D_SETCOM, 1, 0x25,  // -1.52V
  HX8357_SETOSC, 1, 0x68,  // Normal mode 70Hz, Idle mode 55 Hz
  HX8357_SETPANEL, 1, 0x05,  // BGR, Gate direction swapped
  HX8357_SETPWR1, 6, 0x00, 0x15, 0x1C, 0x1C, 0x83, 0xAA,
  HX8357D_SETSTBA, 6, 0x50, 0x50, 0x01, 0x3C, 0x1E, 0x08,
  // MEME GAMMA HERE
  HX8357D_SETCYC, 7, 0x02, 0x40, 0x00, 0x2A, 0x2A, 0x0D, 0x78,
  HX8357_COLMOD, 1, 0x55,
  HX8357_MADCTL, 1, 0xC0,
  HX8357_TEON, 1, 0x00,
  HX8357_TEARLINE, 2, 0x00, 0x02,
  HX8357_SLPOUT, 0,
  TFTLCD_DELAY, 150,
  HX8357_DISPON, 0, 
  TFTLCD_DELAY, 50,
};

static const uint16_t ILI932x_regValues[] = {
  ILI932X_START_OSC        , 0x0001, // Start oscillator
  TFTLCD_DELAY             , 50,     // 50 millisecond delay
  ILI932X_DRIV_OUT_CTRL    , 0x0100,
  ILI932X_DRIV_WAV_CTRL    , 0x0700,
  ILI932X_ENTRY_MOD        , 0x1030,
  ILI932X_RESIZE_CTRL      , 0x0000,
  ILI932X_DISP_CTRL2       , 0x0202,
  ILI932X_DISP_CTRL3       , 0x0000,
  ILI932X_DISP_CTRL4       , 0x0000,
  ILI932X_RGB_DISP_IF_CTRL1, 0x0,
  ILI932X_FRM_MARKER_POS   , 0x0,
  ILI932X_RGB_DISP_IF_CTRL2, 0x0,
  ILI932X_POW_CTRL1        , 0x0000,
  ILI932X_POW_CTRL2        , 0x0007,
  ILI932X_POW_CTRL3        , 0x0000,
  ILI932X_POW_CTRL4        , 0x0000,
  TFTLCD_DELAY             , 200,
  ILI932X_POW_CTRL1        , 0x1690,
  ILI932X_POW_CTRL2        , 0x0227,
  TFTLCD_DELAY             , 50,
  ILI932X_POW_CTRL3        , 0x001A,
  TFTLCD_DELAY             , 50,
  ILI932X_POW_CTRL4        , 0x1800,
  ILI932X_POW_CTRL7        , 0x002A,
  TFTLCD_DELAY             , 50,
  ILI932X_GAMMA_CTRL1      , 0x0000,
  ILI932X_GAMMA_CTRL2      , 0x0000,
  ILI932X_GAMMA_CTRL3      , 0x0000,
  ILI932X_GAMMA_CTRL4      , 0x0206,
  ILI932X_GAMMA_CTRL5      , 0x0808,
  ILI932X_GAMMA_CTRL6      , 0x0007,
  ILI932X_GAMMA_CTRL7      , 0x0201,
  ILI932X_GAMMA_CTRL8      , 0x0000,
  ILI932X_GAMMA_CTRL9      , 0x0000,
  ILI932X_GAMMA_CTRL10     , 0x0000,
  ILI932X_GRAM_HOR_AD      , 0x0000,
  ILI932X_GRAM_VER_AD      , 0x0000,
  ILI932X_HOR_START_AD     , 0x0000,
  ILI932X_HOR_END_AD       , 0x00EF,
  ILI932X_VER_START_AD     , 0X0000,
  ILI932X_VER_END_AD       , 0x013F,
  ILI932X_GATE_SCAN_CTRL1  , 0xA700, // Driver Output Control (R60h)
  ILI932X_GATE_SCAN_CTRL2  , 0x0003, // Driver Output Control (R61h)
  ILI932X_GATE_SCAN_CTRL3  , 0x0000, // Driver Output Control (R62h)
  ILI932X_PANEL_IF_CTRL1   , 0X0010, // Panel Interface Control 1 (R90h)
  ILI932X_PANEL_IF_CTRL2   , 0X0000,
  ILI932X_PANEL_IF_CTRL3   , 0X0003,
  ILI932X_PANEL_IF_CTRL4   , 0X1100,
  ILI932X_PANEL_IF_CTRL5   , 0X0000,
  ILI932X_PANEL_IF_CTRL6   , 0X0000,
  ILI932X_DISP_CTRL1       , 0x0133, // Main screen turn on
};

void tft_begin_id(uint16_t id) {
  uint8_t i = 0;

  tft_reset();

  delay_ms(200);

  if((id == 0x9325) || (id == 0x9328)) {

    uint16_t a, d;
    driver = ID_932X;
    CS_ACTIVE;
    while(i < sizeof(ILI932x_regValues) / sizeof(uint16_t)) {
      a = ILI932x_regValues[i++];
      d = ILI932x_regValues[i++];
      if(a == TFTLCD_DELAY) delay_ms(d);
      else                  tft_writeRegister16(a, d);
    }
    tft_setRotation(rotation);
    tft_setAddrWindow(0, 0, TFTWIDTH-1, TFTHEIGHT-1);

  } else if (id == 0x9341) {

    uint16_t a, d;
    driver = ID_9341;
    CS_ACTIVE;
    tft_writeRegister8(ILI9341_SOFTRESET, 0);
    delay_ms(50);
    tft_writeRegister8(ILI9341_DISPLAYOFF, 0);

    tft_writeRegister8(ILI9341_POWERCONTROL1, 0x23);
    tft_writeRegister8(ILI9341_POWERCONTROL2, 0x10);
    tft_writeRegister16(ILI9341_VCOMCONTROL1, 0x2B2B);
    tft_writeRegister8(ILI9341_VCOMCONTROL2, 0xC0);
    tft_writeRegister8(ILI9341_MEMCONTROL, ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR);
    tft_writeRegister8(ILI9341_PIXELFORMAT, 0x55);
    tft_writeRegister16(ILI9341_FRAMECONTROL, 0x001B);
    
    tft_writeRegister8(ILI9341_ENTRYMODE, 0x07);
    /* tft_writeRegister32(ILI9341_DISPLAYFUNC, 0x0A822700);*/

    tft_writeRegister8(ILI9341_SLEEPOUT, 0);
    delay_ms(150);
    tft_writeRegister8(ILI9341_DISPLAYON, 0);
    delay_ms(500);
    tft_setAddrWindow(0, 0, TFTWIDTH-1, TFTHEIGHT-1);
    return;

  } else if (id == 0x8357) {
    // HX8357D
    driver = ID_HX8357D;
    CS_ACTIVE;
     while(i < sizeof(HX8357D_regValues)) {
      uint8_t r = HX8357D_regValues[i++];
      uint8_t len = HX8357D_regValues[i++];
      if(r == TFTLCD_DELAY) {
	delay_ms(len);
      } else {
	//Serial.print("Register $"); Serial.print(r, HEX);
	//Serial.print(" datalen "); Serial.println(len);

	CS_ACTIVE;
	CD_COMMAND;
	write8(r);
	CD_DATA;
    uint8_t d;
	for (d=0; d<len; d++) {
	  uint8_t x = HX8357D_regValues[i++];
	  write8(x);
	}
	CS_IDLE;

      }
    }
     return;
     
  } else if(id == 0x7575) {

    uint8_t a, d;
    driver = ID_7575;
    CS_ACTIVE;
    while(i < sizeof(HX8347G_regValues)) {
      a = HX8347G_regValues[i++];
      d = HX8347G_regValues[i++];
      if(a == TFTLCD_DELAY) delay_ms(d);
      else                  tft_writeRegister8(a, d);
    }
    tft_setRotation(rotation);
    tft_setLR(); // Lower-right corner of address window

  } else {
    driver = ID_UNKNOWN;
    return;
  }
}

void tft_begin() {
    tft_begin_id(0x9341);
}

void tft_reset(void) {
    uint8_t i;
  CS_IDLE;
//  CD_DATA;
  WR_IDLE;
  RD_IDLE;

  RESET_LOW;
  delay_ms(2);
  RESET_HIGH;

/*  if(_reset) {
    digitalWrite(_reset, LOW);
    delay(2);
    digitalWrite(_reset, HIGH);
  }*/


  // Data transfer sync
  CS_ACTIVE;
  CD_COMMAND;
  write8(0x00);
  for(i=0; i<3; i++) WR_STROBE; // Three extra 0x00s
  CS_IDLE;
}

// Sets the LCD address window (and address counter, on 932X).
// Relevant to rect/screen fills and H/V lines.  Input coordinates are
// assumed pre-sorted (e.g. x2 >= x1).
inline void tft_setAddrWindow(int x1, int y1, int x2, int y2) {
  CS_ACTIVE;
//  if(driver == ID_932X) {
//
//    // Values passed are in current (possibly rotated) coordinate
//    // system.  932X requires hardware-native coords regardless of
//    // MADCTL, so rotate inputs as needed.  The address counter is
//    // set to the top-left corner -- although fill operations can be
//    // done in any direction, the current screen rotation is applied
//    // because some users find it disconcerting when a fill does not
//    // occur top-to-bottom.
//    int x, y, t;
//    switch(rotation) {
//     default:
//      x  = x1;
//      y  = y1;
//      break;
//     case 1:
//      t  = y1;
//      y1 = x1;
//      x1 = TFTWIDTH  - 1 - y2;
//      y2 = x2;
//      x2 = TFTWIDTH  - 1 - t;
//      x  = x2;
//      y  = y1;
//      break;
//     case 2:
//      t  = x1;
//      x1 = TFTWIDTH  - 1 - x2;
//      x2 = TFTWIDTH  - 1 - t;
//      t  = y1;
//      y1 = TFTHEIGHT - 1 - y2;
//      y2 = TFTHEIGHT - 1 - t;
//      x  = x2;
//      y  = y2;
//      break;
//     case 3:
//      t  = x1;
//      x1 = y1;
//      y1 = TFTHEIGHT - 1 - x2;
//      x2 = y2;
//      y2 = TFTHEIGHT - 1 - t;
//      x  = x1;
//      y  = y2;
//      break;
//    }
//    tft_writeRegister16(0x0050, x1); // Set address window
//    tft_writeRegister16(0x0051, x2);
//    tft_writeRegister16(0x0052, y1);
//    tft_writeRegister16(0x0053, y2);
//    tft_writeRegister16(0x0020, x ); // Set address counter to top left
//    tft_writeRegister16(0x0021, y );
//
//  } else if(driver == ID_7575) {
//
//    tft_writeRegisterPair(HX8347G_COLADDRSTART_HI, HX8347G_COLADDRSTART_LO, x1);
//    tft_writeRegisterPair(HX8347G_ROWADDRSTART_HI, HX8347G_ROWADDRSTART_LO, y1);
//    tft_writeRegisterPair(HX8347G_COLADDREND_HI  , HX8347G_COLADDREND_LO  , x2);
//    tft_writeRegisterPair(HX8347G_ROWADDREND_HI  , HX8347G_ROWADDREND_LO  , y2);
//
//  } else if ((driver == ID_9341) || (driver == ID_HX8357D)){
    uint32_t t;

    t = x1;
    t <<= 16;
    t |= x2;
    tft_writeRegister32(ILI9341_COLADDRSET, t);  // HX8357D uses same registers!
    t = y1;
    t <<= 16;
    t |= y2;
    tft_writeRegister32(ILI9341_PAGEADDRSET, t); // HX8357D uses same registers!

//  }
  CS_IDLE;
}

// Unlike the 932X drivers that set the address window to the full screen
// by default (using the address counter for drawPixel operations), the
// 7575 needs the address window set on all graphics operations.  In order
// to save a few register writes on each pixel drawn, the lower-right
// corner of the address window is reset after most fill operations, so
// that drawPixel only needs to change the upper left each time.
void tft_setLR(void) {
  CS_ACTIVE;
  tft_writeRegisterPair(HX8347G_COLADDREND_HI, HX8347G_COLADDREND_LO, _width  - 1);
  tft_writeRegisterPair(HX8347G_ROWADDREND_HI, HX8347G_ROWADDREND_LO, _height - 1);
  CS_IDLE;
}

// Fast block fill operation for fillScreen, fillRect, H/V line, etc.
// Requires setAddrWindow() has previously been called to set the fill
// bounds.  'len' is inclusive, MUST be >= 1.
void tft_flood(uint16_t color, uint32_t len) {
  uint16_t blocks;
  uint8_t  i, hi = color >> 8,
              lo = color;

  CS_ACTIVE;
  CD_COMMAND;
//  if (driver == ID_9341) {
    write8(0x2C);
//  } else if (driver == ID_932X) {
//    write8(0x00); // High byte of GRAM register...
//    write8(0x22); // Write data to GRAM
//  } else if (driver == ID_HX8357D) {
//    write8(HX8357_RAMWR);
//  } else {
//    write8(0x22); // Write data to GRAM
//  }

  // Write first pixel normally, decrement counter by 1
  CD_DATA;
  write8(hi);
  write8(lo);
  len--;

  blocks = (uint16_t)(len / 64); // 64 pixels/block
  if(hi == lo) {
    // High and low bytes are identical.  Leave prior data
    // on the port(s) and just toggle the write strobe.
    while(blocks--) {
      i = 16; // 64 pixels/block / 4 pixels/pass
      do {
        WR_STROBE; WR_STROBE; WR_STROBE; WR_STROBE; // 2 bytes/pixel
        WR_STROBE; WR_STROBE; WR_STROBE; WR_STROBE; // x 4 pixels
      } while(--i);
    }
    // Fill any remaining pixels (1 to 64)
    for(i = (uint8_t)len & 63; i--; ) {
      WR_STROBE;
      WR_STROBE;
    }
  } else {
    while(blocks--) {
      i = 16; // 64 pixels/block / 4 pixels/pass
      do {
        write8(hi); write8(lo); write8(hi); write8(lo);
        write8(hi); write8(lo); write8(hi); write8(lo);
      } while(--i);
    }
    for(i = (uint8_t)len & 63; i--; ) {
      write8(hi);
      write8(lo);
    }
  }
  CS_IDLE;
}

void tft_drawFastHLine(int16_t x, int16_t y, int16_t length,
  uint16_t color)
{
  int16_t x2;

  // Initial off-screen clipping
  if((length <= 0     ) ||
     (y      <  0     ) || ( y                  >= _height) ||
     (x      >= _width) || ((x2 = (x+length-1)) <  0      )) return;

  if(x < 0) {        // Clip left
    length += x;
    x       = 0;
  }
  if(x2 >= _width) { // Clip right
    x2      = _width - 1;
    length  = x2 - x + 1;
  }

  tft_setAddrWindow(x, y, x2, y);
  tft_flood(color, length);
//  if(driver == ID_932X) tft_setAddrWindow(0, 0, _width - 1, _height - 1);
//  else                  tft_setLR();
  tft_setLR();
}

void tft_drawFastVLine(int16_t x, int16_t y, int16_t length,
  uint16_t color)
{
  int16_t y2;

  // Initial off-screen clipping
  if((length <= 0      ) ||
     (x      <  0      ) || ( x                  >= _width) ||
     (y      >= _height) || ((y2 = (y+length-1)) <  0     )) return;
  if(y < 0) {         // Clip top
    length += y;
    y       = 0;
  }
  if(y2 >= _height) { // Clip bottom
    y2      = _height - 1;
    length  = y2 - y + 1;
  }

  tft_setAddrWindow(x, y, x, y2);
  tft_flood(color, length);
//  if(driver == ID_932X) tft_setAddrWindow(0, 0, _width - 1, _height - 1);
//  else                  tft_setLR();
  tft_setLR();
}

void tft_fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h, 
  uint16_t fillcolor) {
  int16_t  x2, y2;

  // Initial off-screen clipping
//  if( (w            <= 0     ) ||  (h             <= 0      ) ||
//      (x1           >= _width) ||  (y1            >= _height) ||
//     ((x2 = x1+w-1) <  0     ) || ((y2  = y1+h-1) <  0      )) return;
//  if(x1 < 0) { // Clip left
//    w += x1;
//    x1 = 0;
//  }
//  if(y1 < 0) { // Clip top
//    h += y1;
//    y1 = 0;
//  }
//  if(x2 >= _width) { // Clip right
//    x2 = _width - 1;
//    w  = x2 - x1 + 1;
//  }
//  if(y2 >= _height) { // Clip bottom
//    y2 = _height - 1;
//    h  = y2 - y1 + 1;
//  }
//
//  tft_setAddrWindow(x1, y1, x2, y2);
  
  // rudimentary clipping (drawChar w/big text requires this)
  if((x1 >= _width) || (y1 >= _height)) return;
  if((x1 + w - 1) >= _width)  w = _width  - x1;
  if((y1 + h - 1) >= _height) h = _height - y1;

  tft_setAddrWindow(x1, y1, x1+w-1, y1+h-1);
  
  
  tft_flood(fillcolor, (uint32_t)w * (uint32_t)h);
//  if(driver == ID_932X) tft_setAddrWindow(0, 0, _width - 1, _height - 1);
//  else                  tft_setLR();
  tft_setLR();
}

void tft_fillScreen(uint16_t color) {
  
  if(driver == ID_932X) {

    // For the 932X, a full-screen address window is already the default
    // state, just need to set the address pointer to the top-left corner.
    // Although we could fill in any direction, the code uses the current
    // screen rotation because some users find it disconcerting when a
    // fill does not occur top-to-bottom.
    uint16_t x, y;
    switch(rotation) {
      default: x = 0            ; y = 0            ; break;
      case 1 : x = TFTWIDTH  - 1; y = 0            ; break;
      case 2 : x = TFTWIDTH  - 1; y = TFTHEIGHT - 1; break;
      case 3 : x = 0            ; y = TFTHEIGHT - 1; break;
    }
    CS_ACTIVE;
    tft_writeRegister16(0x0020, x);
    tft_writeRegister16(0x0021, y);

  } else if ((driver == ID_9341) || (driver == ID_7575) || (driver == ID_HX8357D)) {
    // For these, there is no settable address pointer, instead the
    // address window must be set for each drawing operation.  However,
    // this display takes rotation into account for the parameters, no
    // need to do extra rotation math here.
    tft_setAddrWindow(0, 0, _width - 1, _height - 1);

  }
  tft_flood(color, (long)TFTWIDTH * (long)TFTHEIGHT);
}

void tft_drawPixel(int16_t x, int16_t y, uint16_t color) {

  // Clip
  if((x < 0) || (y < 0) || (x >= _width) || (y >= _height)) return;

  //CS_ACTIVE;
//  if(driver == ID_932X) {
//    int16_t t;
//    switch(rotation) {
//     case 1:
//      t = x;
//      x = TFTWIDTH  - 1 - y;
//      y = t;
//      break;
//     case 2:
//      x = TFTWIDTH  - 1 - x;
//      y = TFTHEIGHT - 1 - y;
//      break;
//     case 3:
//      t = x;
//      x = y;
//      y = TFTHEIGHT - 1 - t;
//      break;
//    }
//    tft_writeRegister16(0x0020, x);
//    tft_writeRegister16(0x0021, y);
//    tft_writeRegister16(0x0022, color);
//
//  } else if(driver == ID_7575) {
//
//    uint8_t hi, lo;
//    switch(rotation) {
//     default: lo = 0   ; break;
//     case 1 : lo = 0x60; break;
//     case 2 : lo = 0xc0; break;
//     case 3 : lo = 0xa0; break;
//    }
//    tft_writeRegister8(   HX8347G_MEMACCESS      , lo);
//    // Only upper-left is set -- bottom-right is full screen default
//    tft_writeRegisterPair(HX8347G_COLADDRSTART_HI, HX8347G_COLADDRSTART_LO, x);
//    tft_writeRegisterPair(HX8347G_ROWADDRSTART_HI, HX8347G_ROWADDRSTART_LO, y);
//    hi = color >> 8; lo = color;
//    CD_COMMAND; write8(0x22); CD_DATA; write8(hi); write8(lo);
//
//  } else if ((driver == ID_9341) || (driver == ID_HX8357D)) {
    
    tft_setAddrWindow(x, y, _width-1, _height-1);
    
    CS_ACTIVE;
    CD_COMMAND; 
    write8(0x2C);
    CD_DATA; 
    write8(color >> 8); write8(color);
//  }

  CS_IDLE;
}

// Issues 'raw' an array of 16-bit color values to the LCD; used
// externally by BMP examples.  Assumes that setWindowAddr() has
// previously been set to define the bounds.  Max 255 pixels at
// a time (BMP examples read in small chunks due to limited RAM).
void tft_pushColors(uint16_t *data, uint8_t len, int first) {
  uint16_t color;
  uint8_t  hi, lo;
  CS_ACTIVE;
  if(first != 0) { // Issue GRAM write command only on first call
    CD_COMMAND;
    if(driver == ID_932X) write8(0x00);
    if ((driver == ID_9341) || (driver == ID_HX8357D)){
       write8(0x2C);
     }  else {
       write8(0x22);
     }
  }
  CD_DATA;
  while(len--) {
    color = *data++;
    hi    = color >> 8; // Don't simplify or merge these
    lo    = color;      // lines, there's macro shenanigans
    write8(hi);         // going on.
    write8(lo);
  }
  CS_IDLE;
}

void tft_setRotation(uint8_t x) {

  // Call parent rotation func first -- sets up rotation flags, etc.
  //Adafruit_GFX::setRotation(x);
  //tft_gfx_setRotation(x);
  rotation = x % 4;          
  // Then perform hardware-specific rotation operations...

  CS_ACTIVE;
  if(driver == ID_932X) {

    uint16_t t;
    switch(rotation) {
     default: t = 0x1030; break;
     case 1 : t = 0x1028; break;
     case 2 : t = 0x1000; break;
     case 3 : t = 0x1018; break;
    }
    tft_writeRegister16(0x0003, t ); // MADCTL
    // For 932X, init default full-screen address window:
    tft_setAddrWindow(0, 0, _width - 1, _height - 1); // CS_IDLE happens here

  }
 if(driver == ID_7575) {

    uint8_t t;
    switch(rotation) {
     default: t = 0   ; break;
     case 1 : t = 0x60; break;
     case 2 : t = 0xc0; break;
     case 3 : t = 0xa0; break;
    }
    tft_writeRegister8(HX8347G_MEMACCESS, t);
    // 7575 has to set the address window on most drawing operations.
    // drawPixel() cheats by setting only the top left...by default,
    // the lower right is always reset to the corner.
    tft_setLR(); // CS_IDLE happens here
  }

 if (driver == ID_9341) { 
   // MEME, HX8357D uses same registers as 9341 but different values
   uint16_t t;

   switch (rotation) {
   case 2:
     t = ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR;
     break;
   case 3:
     t = ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR;
     _width = TFTHEIGHT;
     _height = TFTWIDTH;     
     break;
  case 0:
    t = ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR;
    break;
   case 1:
     t = ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR;
     _width = TFTHEIGHT;
     _height = TFTWIDTH;
     break;
  }
   tft_writeRegister8(ILI9341_MADCTL, t ); // MADCTL
   // For 9341, init default full-screen address window:
   tft_setAddrWindow(0, 0, _width - 1, _height - 1); // CS_IDLE happens here
  }
  
  if (driver == ID_HX8357D) { 
    // MEME, HX8357D uses same registers as 9341 but different values
    uint16_t t;
    
    switch (rotation) {
      case 2:
        t = HX8357B_MADCTL_RGB;
        break;
      case 3:
        t = HX8357B_MADCTL_MX | HX8357B_MADCTL_MV | HX8357B_MADCTL_RGB;
        break;
      case 0:
        t = HX8357B_MADCTL_MX | HX8357B_MADCTL_MY | HX8357B_MADCTL_RGB;
        break;
      case 1:
        t = HX8357B_MADCTL_MY | HX8357B_MADCTL_MV | HX8357B_MADCTL_RGB;
        break;
    }
    tft_writeRegister8(ILI9341_MADCTL, t ); // MADCTL
    // For 8357, init default full-screen address window:
    tft_setAddrWindow(0, 0, _width - 1, _height - 1); // CS_IDLE happens here
  }}

#ifdef read8isFunctionalized
  #define read8(x) x=read8fn()
#endif

// Because this function is used infrequently, it configures the ports for
// the read operation, reads the data, then restores the ports to the write
// configuration.  Write operations happen a LOT, so it's advantageous to
// leave the ports in that state as a default.
/*uint16_t tft_readPixel(int16_t x, int16_t y) {
    uint8_t pass;
  if((x < 0) || (y < 0) || (x >= _width) || (y >= _height)) return 0;

  CS_ACTIVE;
  if(driver == ID_932X) {

    uint8_t hi, lo;
    int16_t t;
    switch(rotation) {
     case 1:
      t = x;
      x = TFTWIDTH  - 1 - y;
      y = t;
      break;
     case 2:
      x = TFTWIDTH  - 1 - x;
      y = TFTHEIGHT - 1 - y;
      break;
     case 3:
      t = x;
      x = y;
      y = TFTHEIGHT - 1 - t;
      break;
    }
    tft_writeRegister16(0x0020, x);
    tft_writeRegister16(0x0021, y);
    // Inexplicable thing: sometimes pixel read has high/low bytes
    // reversed.  A second read fixes this.  Unsure of reason.  Have
    // tried adjusting timing in read8() etc. to no avail.
    for(pass=0; pass<2; pass++) {
      CD_COMMAND; write8(0x00); write8(0x22); // Read data from GRAM
      CD_DATA;
      setReadDir();  // Set up LCD data port(s) for READ operations
      read8(hi);     // First 2 bytes back are a dummy read
      read8(hi);
      read8(hi);     // Bytes 3, 4 are actual pixel value
      read8(lo);
      setWriteDir(); // Restore LCD data port(s) to WRITE configuration
    }
    CS_IDLE;
    return ((uint16_t)hi << 8) | lo;

  } else if(driver == ID_7575) {

    uint8_t r, g, b;
    tft_writeRegisterPair(HX8347G_COLADDRSTART_HI, HX8347G_COLADDRSTART_LO, x);
    tft_writeRegisterPair(HX8347G_ROWADDRSTART_HI, HX8347G_ROWADDRSTART_LO, y);
    CD_COMMAND; write8(0x22); // Read data from GRAM
    setReadDir();  // Set up LCD data port(s) for READ operations
    CD_DATA;
    read8(r);      // First byte back is a dummy read
    read8(r);
    read8(g);
    read8(b);
    setWriteDir(); // Restore LCD data port(s) to WRITE configuration
    CS_IDLE;
    return (((uint16_t)r & 0xF8) << 8) |
           (((uint16_t)g & 0xFC) << 3) |
           (           b              >> 3);
  } else return 0;
}*/

// Ditto with the read/write port directions, as above.
uint16_t tft_readID(void) {

  uint8_t hi, lo;

  uint8_t i;
  /*for (i=0; i<128; i++) {
      printf("$%x", i);
      printf(" = 0x%x\n\r", tft_readReg(i));
    //Serial.print("$"); Serial.print(i, HEX);
    //Serial.print(" = 0x"); Serial.println(readReg(i), HEX);
  }*/
  

  if (tft_readReg(0x04) == 0x8000) { // eh close enough
    // setc!
    tft_writeRegister24(HX8357D_SETC, 0xFF8357);
    delay_ms(300);
    //Serial.println(readReg(0xD0), HEX);
    if (tft_readReg(0xD0) == 0x990000) {
      return 0x8357;
    }
  }

  uint16_t id = tft_readReg(0xD3);
  if (id == 0x9341) {
    return id;
  }

  CS_ACTIVE;
  CD_COMMAND;
  write8(0x00);
  WR_STROBE;     // Repeat prior byte (0x00)
  setReadDir();  // Set up LCD data port(s) for READ operations
  CD_DATA;
  read8(hi);
  read8(lo);
  setWriteDir();  // Restore LCD data port(s) to WRITE configuration
  CS_IDLE;

  id = hi; id <<= 8; id |= lo;
  return id;
}

uint32_t tft_readReg(uint8_t r) {
  uint32_t id;
  uint8_t x;

  // try reading register #4
  CS_ACTIVE;
  CD_COMMAND;
  write8(r);
  setReadDir();  // Set up LCD data port(s) for READ operations
  CD_DATA;
  delay_us(50);
  read8(x);
  id = x;          // Do not merge or otherwise simplify
  id <<= 8;              // these lines.  It's an unfortunate
  read8(x);
  id  |= x;        // shenanigans that are going on.
  id <<= 8;              // these lines.  It's an unfortunate
  read8(x);
  id  |= x;        // shenanigans that are going on.
  id <<= 8;              // these lines.  It's an unfortunate
  read8(x);
  id  |= x;        // shenanigans that are going on.
  CS_IDLE;
  setWriteDir();  // Restore LCD data port(s) to WRITE configuration

  return id;
}

// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t tft_color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// For I/O macros that were left undefined, declare function
// versions that reference the inline macros just once:
#ifndef tft_writeRegister8
void tft_writeRegister8(uint8_t a, uint8_t d) {
  writeRegister8inline(a, d);
}
#endif

#ifndef tft_writeRegister16
void tft_writeRegister16(uint16_t a, uint16_t d) {
  writeRegister16inline(a, d);
}
#endif

#ifndef tft_writeRegisterPair
void tft_writeRegisterPair(uint8_t aH, uint8_t aL, uint16_t d) {
  writeRegisterPairInline(aH, aL, d);
}
#endif


void tft_writeRegister24(uint8_t r, uint32_t d) {
  CS_ACTIVE;
  CD_COMMAND;
  write8(r);
  CD_DATA;
  delay_us(10);
  write8(d >> 16);
  delay_us(10);
  write8(d >> 8);
  delay_us(10);
  write8(d);
  CS_IDLE;

}


void tft_writeRegister32(uint8_t r, uint32_t d) {
  CS_ACTIVE;
  CD_COMMAND;
  write8(r);
  CD_DATA;
  //delay_us(10);
  //delay_us(1);
//    asm volatile(       \
//    "nop" "\n\t" \
//    ::);
  write8(d >> 24);
  //delay_us(10);
  //delay_us(1);
//      asm volatile(       \
//    "nop" "\n\t" \
//    ::);
  write8(d >> 16);
  //delay_us(10);
  //delay_us(1);
//      asm volatile(       \
//    "nop" "\n\t" \
//    ::);
  write8(d >> 8);
  //delay_us(10);
  //delay_us(1);
//      asm volatile(       \
//    "nop" "\n\t" \
//    ::);
  write8(d);
  CS_IDLE;
}

void delay_ms(unsigned long i){
/* Create a software delay about i ms long
 * Parameters:
 *      i:  equal to number of milliseconds for delay
 * Returns: Nothing
 * Note: Uses Core Timer. Core Timer is cleared at the initialiazion of
 *      this function. So, applications sensitive to the Core Timer are going
 *      to be affected
 */
    unsigned int j;
    j = dTime_ms * i;
    WriteCoreTimer(0);
    while (ReadCoreTimer() < j);
}

void delay_us(unsigned long i){
/* Create a software delay about i us long
 * Parameters:
 *      i:  equal to number of microseconds for delay
 * Returns: Nothing
 * Note: Uses Core Timer. Core Timer is cleared at the initialiazion of
 *      this function. So, applications sensitive to the Core Timer are going
 *      to be affected
 */
    unsigned int j;
    j = dTime_us * i;
    WriteCoreTimer(0);
    while (ReadCoreTimer() < j);
}