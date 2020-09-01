#include <stdio.h>
#include "lcd/lcd_gfx.h"
#include "lcd/lcd_hal.h"
#include "vip.h"


#define VIP_MAX_ANNOUNCEMENT 80
char latest_announcement[VIP_MAX_ANNOUNCEMENT];
uint16_t latest_announcement_id = 0;
uint8_t new_msg = 0;
uint8_t last_second = 0;

int8_t new_announcement(uint16_t announcement_id, char *msg, uint8_t msg_len)
{
 	uint8_t i;
 	if (announcement_id <= latest_announcement_id)
 		return -1;

    if (msg_len >= VIP_MAX_ANNOUNCEMENT )
    {
       	msg_len = VIP_MAX_ANNOUNCEMENT - 1;
    }

    for(i=0; i<msg_len; i++)
    {
        latest_announcement[i] = msg[i];
    }
    latest_announcement[i] = '\0';
    return VIP_ANNOUNCEMENT_MSG;

}

void pre_announcement(uint8_t refresh, uint32_t time)
{
	char buf[4];
	if (refresh)
	{
		printf("\n");
		lcd_fillScreen(lcd_gfx_getBackgroundColor());
		lcd_gfx_setCursor(0, 30);
		lcd_gfx_setTextSize(2);
		lcd_gfx_printCenter("UPCOMING\n");
		lcd_gfx_printCenter("LEGITIMATE\n");
		lcd_gfx_printCenter("ANNOUNCEMENT\n");
		last_second = time;
		lcd_invertOn();
	}
	if (last_second != time)
	{
		last_second = time;
		if (time % 2 == 0)
		{
			lcd_invertOn();
		}
		else
		{
			lcd_invertOff();
		}
		lcd_gfx_setCursor(0, 85);
		sprintf(&buf, " %u \n", (unsigned int)time);
		lcd_gfx_printCenter(&buf);
		
	}
}

void announcement(uint8_t refresh, uint32_t time)
{
	
	if (refresh)
	{
		lcd_invertOff();
		lcd_fillScreen(lcd_gfx_getBackgroundColor());
		lcd_gfx_setCursor(0, 30);
		lcd_gfx_setTextSize(2);
		lcd_gfx_printCenter(latest_announcement);
	
	}

}