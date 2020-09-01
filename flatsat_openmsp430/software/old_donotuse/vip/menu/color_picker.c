#include "hardware.h"
#include "lcd/lcd_hal.h"
#include "lcd/lcd_gfx.h"

uint16_t picker_width, picker_height;
extern volatile uint8_t button_next, button_up, button_down, button_back;

uint16_t get_color(uint16_t x, uint16_t y)
{
    uint16_t r, g, b;
    r = x/4;
    g = y/4;
    b = (127 -x)/4;
    return (((r & 0x1f) << 11) | ((g & 0x1f) << 6 ) | (b & 0x1f));
}


uint16_t color_picker(void)
{
    uint8_t soft_timer;
    uint16_t cursor_x, cursor_y;
    button_up = button_down = button_back = button_next = 0;
    lcd_fillScreen( ST7735_BLACK );

    for (cursor_x = 0; cursor_x < 128; cursor_x++)
    {
        for (cursor_y = 0; cursor_y<128; cursor_y++)
        {
            lcd_drawPixel(cursor_x, cursor_y, get_color(cursor_x, cursor_y));
        }
    }		
    cursor_x = cursor_y = 0;

    while(1){
        lcd_fillRect(130,0, 160-130, 128, get_color(cursor_x, cursor_y));
        lcd_fillRect(cursor_x, cursor_y, 4,4, 0xffff);
        for(soft_timer = 0; soft_timer < 7; soft_timer++) LPM0;
        // All buttons pressed exits 
        if ((BUTTON_PORT & 0x0F) == 0x0F){
            return get_color(cursor_x, cursor_y);
        }

        if ((button_up == 1) || (BUTTON_PORT & BUTTON_UP_BIT)) {
            button_up = 0;
            lcd_fillRect(cursor_x, cursor_y, 4,4, get_color(cursor_x, cursor_y));
            if(cursor_y > 0) cursor_y-=4;
            else cursor_y = 124;
        }
        if ((button_down == 1) || (BUTTON_PORT & BUTTON_DOWN_BIT)){
            button_down = 0;
            lcd_fillRect(cursor_x, cursor_y, 4,4, get_color(cursor_x, cursor_y));
            if(cursor_y < 124) cursor_y+=4;
            else cursor_y = 0;
        }
        if ((button_back == 1) || (BUTTON_PORT & BUTTON_BACK_BIT)){
            button_back = 0;
            lcd_fillRect(cursor_x, cursor_y, 4,4, get_color(cursor_x, cursor_y));
            if(cursor_x < 124) cursor_x+=4;
            else cursor_x = 0;
        }
        if ((button_next == 1) || (BUTTON_PORT & BUTTON_NEXT_BIT)){
            button_next = 0;
            lcd_fillRect(cursor_x, cursor_y, 4,4, get_color(cursor_x, cursor_y));
            if(cursor_x > 0) cursor_x-=4;
            else cursor_x = 124;
        }    
    }
}
