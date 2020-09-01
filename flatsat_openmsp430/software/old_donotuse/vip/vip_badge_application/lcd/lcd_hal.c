
/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!
 
Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include "lcd_hal.h"
#include "delay.h"

// LCD globals
unsigned char tabcolor;
unsigned char colstart, rowstart;
unsigned char _width = ST7735_TFTWIDTH;
unsigned char _height = ST7735_TFTHEIGHT;


void lcd_spi_init( void )
{
	// Bring high the LCD Chip Select
	LCD_PORT |= LCD_CSN_BIT;

	// Set everything low except the clock divider to 4Mhz
	LCDSPI_CTRL = (0x0000);

	// Turn on backlight
//	LCD_PORT |= LCD_BACKLIGHT_BIT;
}

void lcd_spiwrite_byte( unsigned char data )
{
	LCDSPI_DATA = data;
	LCDSPI_CTRL |= LCDSPI_CTRL_EN;

	while ( LCDSPI_CTRL & LCDSPI_CTRL_EN )
		;
}

void lcd_writecommand( unsigned char cmd )
{
	LCD_PORT &= ~LCD_RS_BIT;
	LCD_PORT &= ~LCD_CSN_BIT;
	
	lcd_spiwrite_byte( cmd );

	LCD_PORT |= LCD_CSN_BIT;
}

void lcd_writedata( unsigned char data )
{
	LCD_PORT |= LCD_RS_BIT;
	LCD_PORT &= ~LCD_CSN_BIT;
	
	lcd_spiwrite_byte( data );

	LCD_PORT |= LCD_CSN_BIT;
}

// Rather than a bazillion writecommand() and writedata() calls, screen
// initialization commands and arguments are organized in these tables
// stored in PROGMEM.  The table may look bulky, but that's mostly the
// formatting -- storage-wise this is hundreds of bytes more compact
// than the equivalent code.  Companion function follows.
#define DELAY 0x80
static const unsigned char
  Bcmd[] = {                  // Initialization commands for 7735B screens
    18,                       // 18 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, no args, w/delay
      50,                     //     50 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, no args, w/delay
      255,                    //     255 = 500 ms delay
    ST7735_COLMOD , 1+DELAY,  //  3: Set color mode, 1 arg + delay:
      0x05,                   //     16-bit color
      10,                     //     10 ms delay
    ST7735_FRMCTR1, 3+DELAY,  //  4: Frame rate control, 3 args + delay:
      0x00,                   //     fastest refresh
      0x06,                   //     6 lines front porch
      0x03,                   //     3 lines back porch
      10,                     //     10 ms delay
    ST7735_MADCTL , 1      ,  //  5: Memory access ctrl (directions), 1 arg:
      0x08,                   //     Row addr/col addr, bottom to top refresh
    ST7735_DISSET5, 2      ,  //  6: Display settings #5, 2 args, no delay:
      0x15,                   //     1 clk cycle nonoverlap, 2 cycle gate
                              //     rise, 3 cycle osc equalize
      0x02,                   //     Fix on VTL
    ST7735_INVCTR , 1      ,  //  7: Display inversion control, 1 arg:
      0x0,                    //     Line inversion
    ST7735_PWCTR1 , 2+DELAY,  //  8: Power control, 2 args + delay:
      0x02,                   //     GVDD = 4.7V
      0x70,                   //     1.0uA
      10,                     //     10 ms delay
    ST7735_PWCTR2 , 1      ,  //  9: Power control, 1 arg, no delay:
      0x05,                   //     VGH = 14.7V, VGL = -7.35V
    ST7735_PWCTR3 , 2      ,  // 10: Power control, 2 args, no delay:
      0x01,                   //     Opamp current small
      0x02,                   //     Boost frequency
    ST7735_VMCTR1 , 2+DELAY,  // 11: Power control, 2 args + delay:
      0x3C,                   //     VCOMH = 4V
      0x38,                   //     VCOML = -1.1V
      10,                     //     10 ms delay
    ST7735_PWCTR6 , 2      ,  // 12: Power control, 2 args, no delay:
      0x11, 0x15,
    ST7735_GMCTRP1,16      ,  // 13: Magical unicorn dust, 16 args, no delay:
      0x09, 0x16, 0x09, 0x20, //     (seriously though, not sure what
      0x21, 0x1B, 0x13, 0x19, //      these config values represent)
      0x17, 0x15, 0x1E, 0x2B,
      0x04, 0x05, 0x02, 0x0E,
    ST7735_GMCTRN1,16+DELAY,  // 14: Sparkles and rainbows, 16 args + delay:
      0x0B, 0x14, 0x08, 0x1E, //     (ditto)
      0x22, 0x1D, 0x18, 0x1E,
      0x1B, 0x1A, 0x24, 0x2B,
      0x06, 0x06, 0x02, 0x0F,
      10,                     //     10 ms delay
    ST7735_CASET  , 4      ,  // 15: Column addr set, 4 args, no delay:
      0x00, 0x02,             //     XSTART = 2
      0x00, 0x81,             //     XEND = 129
    ST7735_RASET  , 4      ,  // 16: Row addr set, 4 args, no delay:
      0x00, 0x02,             //     XSTART = 1
      0x00, 0x81,             //     XEND = 160
    ST7735_NORON  ,   DELAY,  // 17: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,   DELAY,  // 18: Main screen turn on, no args, w/delay
      255 },                  //     255 = 500 ms delay

  Rcmd1[] = {                 // Init for 7735R, part 1 (red or green tab)
    15,                       // 15 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
      150,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
      255,                    //     500 ms delay
    ST7735_FRMCTR1, 3      ,  //  3: Frame rate ctrl - normal mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3      ,  //  4: Frame rate control - idle mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6      ,  //  5: Frame rate ctrl - partial mode, 6 args:
      0x01, 0x2C, 0x2D,       //     Dot inversion mode
      0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR , 1      ,  //  6: Display inversion ctrl, 1 arg, no delay:
      0x07,                   //     No inversion
    ST7735_PWCTR1 , 3      ,  //  7: Power control, 3 args, no delay:
      0xA2,
      0x02,                   //     -4.6V
      0x84,                   //     AUTO mode
    ST7735_PWCTR2 , 1      ,  //  8: Power control, 1 arg, no delay:
      0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3 , 2      ,  //  9: Power control, 2 args, no delay:
      0x0A,                   //     Opamp current small
      0x00,                   //     Boost frequency
    ST7735_PWCTR4 , 2      ,  // 10: Power control, 2 args, no delay:
      0x8A,                   //     BCLK/2, Opamp current small & Medium low
      0x2A,  
    ST7735_PWCTR5 , 2      ,  // 11: Power control, 2 args, no delay:
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,  // 12: Power control, 1 arg, no delay:
      0x0E,
    ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_MADCTL , 1      ,  // 14: Memory access control (directions), 1 arg:
      0xC8,                   //     row addr/col addr, bottom to top refresh
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
      0x05 },                 //     16-bit color

  Rcmd2green[] = {            // Init for 7735R, part 2 (green tab only)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x02,             //     XSTART = 0
      0x00, 0x7F+0x02,        //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x01,             //     XSTART = 0
      0x00, 0x9F+0x01 },      //     XEND = 159
  Rcmd2red[] = {              // Init for 7735R, part 2 (red tab only)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x9F },           //     XEND = 159

  Rcmd3[] = {                 // Init for 7735R, part 3 (red or green tab)
    4,                        //  4 commands in list:
    ST7735_GMCTRP1, 16      , //  1: Magical unicorn dust, 16 args, no delay:
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , //  2: Sparkles and rainbows, 16 args, no delay:
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
      100 };                  //     100 ms delay


 
// Companion code to the above tables.  Reads and issues
// a series of LCD commands stored in PROGMEM byte array.
void commandList(const unsigned char *addr) 
{

  unsigned char  numCommands, numArgs;
  unsigned int ms;

  numCommands = *addr++;   // Number of commands to follow
  while(numCommands--) {                 // For each command...
    lcd_writecommand( *addr++ ); //   Read, issue command
    numArgs  = *addr++;    //   Number of args to follow
    ms       = numArgs & DELAY;          //   If hibit set, delay follows args
    numArgs &= ~DELAY;                   //   Mask out delay bit
    while(numArgs--) {                   //   For each argument...
      lcd_writedata( *addr++ );  //     Read, issue argument
    }

    if(ms) {
      ms = *addr++; // Read post-command delay time (ms)
      if(ms == 255) ms = 500;     // If 255, delay for 500 ms
      delay_ms(ms);
    }
  }
}


void commonInit(const unsigned char *cmdList) 
{
	colstart  = rowstart = 0; // May be overridden in init func

	lcd_spi_init();

	// Reset screen (TODO: Optimize this to reset faster)
	LCD_PORT |= LCD_RESET_BIT;
	delay_ms(500);
	LCD_PORT &= ~LCD_RESET_BIT;
	delay_ms(500);
	LCD_PORT |= LCD_RESET_BIT;
	delay_ms(500);

	if(cmdList) 
		commandList(cmdList);
}


// Initialization for ST7735R screens (green or red tabs)
void lcd_init_screen(unsigned char options) 
{
	commonInit(Rcmd1);
	if (options == INITR_GREENTAB) 
	{
		commandList(Rcmd2green);
		colstart = 2;
		rowstart = 1;
	} 
	else 
	{
		// colstart, rowstart left at default '0' values
		commandList(Rcmd2red);
	}

	commandList(Rcmd3);

  // if black, change MADCTL color filter
  if (options == INITR_BLACKTAB) 
  {
    lcd_writecommand(ST7735_MADCTL);
    // TODO: OLD lcd_writedata(0xC0);
    lcd_writedata( 0x60 );
  }

  tabcolor = options;
  // Turn on backlight
  LCD_PORT |= LCD_BACKLIGHT_BIT;
}

void lcd_setAddrWindow( unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1 )
{

  lcd_writecommand(ST7735_CASET); // Column addr set
  lcd_writedata(0x00);
  lcd_writedata(x0+colstart);     // XSTART 
  lcd_writedata(0x00);
  lcd_writedata(x1+colstart);     // XEND

  lcd_writecommand(ST7735_RASET); // Row addr set
  lcd_writedata(0x00);
  lcd_writedata(y0+rowstart);     // YSTART
  lcd_writedata(0x00);
  lcd_writedata(y1+rowstart);     // YEND

  lcd_writecommand(ST7735_RAMWR); // write to RAM
}




// fill a rectangle
void lcd_fillRect( unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int color )
{
  unsigned char hi, lo;

  // rudimentary clipping (drawChar w/big text requires this)
  if((x >= _width) || (y >= _height)) return;
  if((x + w - 1) >= _width)  w = _width  - x;
  if((y + h - 1) >= _height) h = _height - y;

  lcd_setAddrWindow(x, y, x+w-1, y+h-1);

  hi = color >> 8, lo = color;

  LCD_PORT |= LCD_RS_BIT;
  LCD_PORT &= ~LCD_CSN_BIT;
  
  for(y=h; y>0; y--) 
  {
    for(x=w; x>0; x--) 
    {
      lcd_spiwrite_byte(hi);
      lcd_spiwrite_byte(lo);
    }
  }

  LCD_PORT |= LCD_CSN_BIT;
}
 
void lcd_fillScreen( unsigned int color ) 
{
  lcd_fillRect(0, 0,  _width, _height, color);
}

void lcd_drawPixel( unsigned int x, unsigned int y, unsigned int color) 
{

  if ((x >= _width) || (y >= _height)) return;

  lcd_setAddrWindow(x,y,x+1,y+1);

  LCD_PORT |= LCD_RS_BIT;
  LCD_PORT &= ~LCD_CSN_BIT;
  
  lcd_pushColor(color);

  LCD_PORT |= LCD_CSN_BIT;
}

void lcd_invertOn(void){
  lcd_writecommand(ST7735_INVON);
}

void lcd_invertOff(void){
  lcd_writecommand(ST7735_INVOFF);
}

void lcd_pushColor(unsigned int color){
  lcd_spiwrite_byte(color>>8);
  lcd_spiwrite_byte(color);
}
