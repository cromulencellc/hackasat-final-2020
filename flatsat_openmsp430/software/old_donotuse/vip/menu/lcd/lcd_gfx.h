#ifndef _LCD_GFX_H
#define _LCD_GFX_H

#include <stdint.h>
#define swap(a, b) { int16_t t = a; a = b; b = t; }

void lcd_init_gfx(int16_t w, int16_t h); // Constructor

void lcd_gfx_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void lcd_gfx_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void lcd_gfx_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void lcd_gfx_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

// These exist only with Adafruit_GFX (no subclass overrides)
void lcd_gfx_drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void lcd_gfx_drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);

void lcd_gfx_fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void lcd_gfx_fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color);
void lcd_gfx_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void lcd_gfx_fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void lcd_gfx_drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
void lcd_gfx_fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
void lcd_gfx_drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
void lcd_gfx_drawBitmap16(int16_t x, int16_t y,  uint8_t *bitmap, uint16_t *palette, uint8_t width, uint8_t height, uint8_t scale) ;
uint16_t lcd_gfx_color565(uint8_t r, uint8_t g, uint8_t b) ;
int8_t lcd_gfx_drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
void lcd_gfx_print(char *s);
void lcd_gfx_printMenu(char * menu[], uint8_t length, uint8_t selection);
void lcd_gfx_setCursor(int16_t x, int16_t y);
void lcd_gfx_setTextColor(uint16_t c, uint16_t b);
void lcd_gfx_setTextSize(uint8_t s);
void lcd_gfx_setTextWrap(uint8_t w);
void lcd_gfx_setRotation(uint8_t r);
uint8_t lcd_gfx_getRotation(void);
int16_t lcd_gfx_getHeight(void);
int16_t lcd_gfx_getWidth(void); 
void lcd_gfx_setWidth(uint16_t w); 
void lcd_gfx_setHeight(uint16_t h); 
uint16_t lcd_gfx_getBackgroundColor();
void lcd_gfx_setBackgroundColor(uint16_t color);
uint16_t lcd_gfx_getForegroundColor();
void lcd_gfx_setForegroundColor(uint16_t color);

#endif // __LCD_GFX_H__
