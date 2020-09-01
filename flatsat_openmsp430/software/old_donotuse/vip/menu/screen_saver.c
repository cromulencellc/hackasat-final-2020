#include "hardware.h"
#include "lcd/lcd_hal.h"
#include "lcd/lcd_gfx.h"


extern volatile uint8_t button_next, button_up, button_down, button_back;

extern uint16_t get_color(uint16_t x, uint16_t y);

extern uint16_t get_rgb_color(uint16_t r, uint16_t g, uint16_t b);


void screen_saver(void)
{
    uint16_t cursor_x, cursor_y;
    button_up = button_down = button_back = button_next = 0;
    lcd_fillScreen( ST7735_BLACK );
	cursor_x = cursor_y = 0;
    while(1){
        
        while(cursor_x < 124){
            lcd_fillScreen(get_color(cursor_x, cursor_y));
            cursor_x += 4;
            // All buttons pressed exits 
            if (((BUTTON_PORT & 0x0F) == 0x0F) || (button_back == 1)){
                return;
            }
        }
        while(cursor_y < 124){
            lcd_fillScreen(get_color(cursor_x, cursor_y));
            cursor_y += 4;
            // All buttons pressed exits 
            if (((BUTTON_PORT & 0x0F) == 0x0F) || (button_back == 1)){
                return;
            }
        }
        while(cursor_x > 0){
            lcd_fillScreen(get_color(cursor_x, cursor_y));
            cursor_x -=4;
            // All buttons pressed exits 
            if (((BUTTON_PORT & 0x0F) == 0x0F) || (button_back == 1)){
                return;
            }
        }
        cursor_x = 0;
        while(cursor_y > 0){
            lcd_fillScreen(get_color(cursor_x, cursor_y));
            cursor_y -= 4;
            // All buttons pressed exits 
            if (((BUTTON_PORT & 0x0F) == 0x0F) || (button_back == 1)){
                return;
            }
        }
        cursor_y = 0;
        
    }

}
