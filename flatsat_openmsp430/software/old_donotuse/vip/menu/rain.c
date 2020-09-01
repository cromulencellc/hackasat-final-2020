#include "hardware.h"
#include "lcd/lcd_hal.h"
#include "lcd/lcd_gfx.h"

extern volatile uint8_t button_next, button_up, button_down, button_back;
extern volatile int seconds;

uint8_t active[6] = {3,7,15,18,24,30};
uint8_t place[32] = {2,5,8,5,13,3,7,3,5,14,
					6,14,4,1,13,13,11,12,8,2,
					4,6,11,7,4,1,4,11,5,6,
					5,7};
uint8_t last[32] = {110, 16, 246, 18, 186, 54, 230, 67, 21, 30, 
					199, 186, 226, 253, 196, 11, 69, 215, 134, 85, 
					202, 185, 12, 125, 107, 64, 239, 59, 146, 47, 
					251, 163};

void rain(void){
	uint8_t index;
	uint8_t current;
	uint16_t textcolor, bgcolor;

	bgcolor = lcd_gfx_getBackgroundColor();
	textcolor = lcd_gfx_getForegroundColor();
    lcd_fillScreen(bgcolor);
   
	while(1) 
    {
		for(current=0;current<6; current++)
		{
			index = active[current];
			if (last[index] <= 0x90)
			{
				lcd_gfx_drawChar(index*6, place[index]*9, last[index], textcolor, bgcolor, 1);
			}
			else
			{
				lcd_gfx_drawChar(index*6, place[index]*9, 0x20, textcolor, bgcolor, 1);
			}

			place[index] += 1;
			last[index] += 2;
			if (last[index] > 0x90)
			{
				lcd_gfx_drawChar(index*6, place[index]*9, 0x20, textcolor, bgcolor, 1);
			}
			else
			{
				lcd_gfx_drawChar(index*6, place[index]*9, last[index], COLOR_LEGIT_WHITE, bgcolor, 1);
			}

			if (place[index] > 17){
				place[index] = 0;
				active[current] = (active[current] + 5) % 32;

			}
		}	

		if (button_back == 1){
            button_back = 0;
            break;
        }
         // All buttons pressed exits 
        if ((BUTTON_PORT & 0x0F) == 0x0F){
            break;
        }

    }
}