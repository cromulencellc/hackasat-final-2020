#include <string.h>
#include "hardware.h"
#include "lcd/lcd_hal.h"
#include "lcd/lcd_gfx.h"
#include "vip.h"


uint8_t active[6] = {3,7,15,18,24,30};
uint8_t place[32] = {2,5,8,5,13,3,7,3,5,14,
					6,14,4,1,13,13,11,12,8,2,
					4,6,11,7,4,1,4,11,5,6,
					5,7};
uint8_t last[32] = {110, 16, 246, 18, 186, 54, 230, 67, 21, 30, 
					199, 186, 226, 253, 196, 11, 69, 215, 134, 85, 
					202, 185, 12, 125, 107, 64, 239, 59, 146, 47, 
					251, 163};

uint8_t rain_index;
uint8_t rain_current;

void rain(uint8_t refresh)
{


	if (refresh)
	{
    	lcd_fillScreen(lcd_gfx_getBackgroundColor());
    	lcd_gfx_setTextSize(2);
    	lcd_gfx_setCursor(0, 55);
    	lcd_gfx_setTextColor(lcd_gfx_getHighlightColor(), lcd_gfx_getBackgroundColor());
    	lcd_gfx_printCenter(VIP_NAME);
    }
   	
	for(rain_current=0;rain_current<6; rain_current++)
	{
		rain_index = active[rain_current];

		if ((place[rain_index] < 6) || (place[rain_index] > 7))
		{
			if (last[rain_index] <= 0x90)
			{
				lcd_gfx_drawChar(rain_index*6, place[rain_index]*9, last[rain_index], lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor(), 1);
			}
			else
			{
				lcd_gfx_drawChar(rain_index*6, place[rain_index]*9, 0x20, lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor(), 1);
			}
		}

		place[rain_index] += 1;
		last[rain_index] += 2;
		
		if ((place[rain_index] < 6) || (place[rain_index] > 7))
		{
			if (last[rain_index] > 0x90)
			{
				lcd_gfx_drawChar(rain_index*6, place[rain_index]*9, 0x20, lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor(), 1);
			}
			else
			{
				lcd_gfx_drawChar(rain_index*6, place[rain_index]*9, last[rain_index], COLOR_LEGIT_WHITE, lcd_gfx_getBackgroundColor(), 1);
			}
		}

		if (place[rain_index] > 17){
			place[rain_index] = 0;
			active[rain_current] = (active[rain_current] + 5) % 32;

		}
	}	

}