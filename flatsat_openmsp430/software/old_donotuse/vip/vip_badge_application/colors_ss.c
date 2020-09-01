#include "hardware.h"
#include "lcd/lcd_hal.h"
#include "lcd/lcd_gfx.h"
#include "vip.h"

uint16_t cursor_x, cursor_y;
uint8_t color_stage;

void colors_ss(void)
{
    switch(color_stage)
    {
        case 1:
            cursor_x += 4;
            lcd_fillScreen(get_color(cursor_x, cursor_y));
            if (cursor_x >= 124)
                color_stage++;
            break;
        case 2:
            cursor_y += 4;
            lcd_fillScreen(get_color(cursor_x, cursor_y));
            if (cursor_y >= 124)
                color_stage++;
            break;
        case 3:
            cursor_x -=4;
            lcd_fillScreen(get_color(cursor_x, cursor_y));
            if (cursor_x <= 0)
                color_stage++;
            break;
        case 4:
            cursor_y -= 4;
            lcd_fillScreen(get_color(cursor_x, cursor_y));
            if (cursor_y <= 0)
                color_stage = 1;
            break;
        default:
            cursor_y = 0;
            cursor_x = 0;
            color_stage = 1;
            lcd_fillScreen(get_color(cursor_x, cursor_y));
    }

}
