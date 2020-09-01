#include <stdio.h>
#include "hardware.h"
#include "delay.h"
#include "lcd/lcd_hal.h"
#include "lcd/lcd_gfx.h"
/**
This one is executed onece a second. it counts seconds, minues, hours - hey
it shoule be a clock ;-)
it does not count days, but i think you'll get the idea.
*/
volatile int irq_counter, offset;
volatile uint8_t button_next, button_up, button_down, button_back;
uint16_t picker_width, picker_height;

wakeup interrupt (WDT_VECTOR) INT_Watchdog(void) {
 
  irq_counter++;
  if (irq_counter == 300) {
    irq_counter = 0;
    offset = (offset+1) % 20;
  }
}
 

int putchar (int txdata) {
    while (UART_STAT & UART_TX_FULL);
    UART_TXD = txdata;
    return 0;
}


//----------------------------------------------------//
// PORT1 Interrupt -- Button Depress
//----------------------------------------------------//
interrupt (PORT1_VECTOR) INT_button(void)
{
    if ( BUTTON_FLAG & BUTTON_NEXT_BIT )
    {
        button_next =1;
        BUTTON_FLAG ^= BUTTON_NEXT_BIT;
    }
    if (BUTTON_FLAG & BUTTON_DOWN_BIT){
        button_down = 1;
        BUTTON_FLAG ^= BUTTON_DOWN_BIT;
    }
    if ( BUTTON_FLAG & BUTTON_UP_BIT ){
        button_up = 1;
        BUTTON_FLAG ^= BUTTON_UP_BIT;
    }
    if (BUTTON_FLAG & BUTTON_BACK_BIT){
        button_back = 1;
        BUTTON_FLAG ^= BUTTON_BACK_BIT;
    }

    LPM0_EXIT;
}




void init(void)
{ 
    WDTCTL = WDTPW | WDTHOLD;          // Disable watchdog timer
 
    UART_BAUD = BAUD;                   // Init UART
    UART_CTL = UART_EN;                 //Enable UART Output
    
    P1OUT  = 0x00;                     // Port data output
    P2OUT  = 0x00;
 
    P1DIR  = 0x00;                     // Port direction register
    P2DIR  = 0xff;
 
    P1IES  = 0x00;                     // Port interrupt enable (0=dis 1=enabled)
    P2IES  = 0x00;
    P1IE   = 0x0F;                     // Port interrupt Edge Select (0=pos 1=neg)
    P2IE   = 0x00;
 
    //WDTCTL = WDTPW | WDTTMSEL | WDTCNTCL;// | WDTIS1  | WDTIS0 ;          // Configure watchdog interrupt
 
    //IE1 |= 0x01;

    eint();                            //enable interrupts
    
    lcd_init_screen( INITR_BLACKTAB );
    lcd_init_gfx( 160, 120 );
}

void testprint(char *s)
{
	printf("%s\r", s);
	lcd_gfx_print(s);
}

uint16_t get_color(uint16_t x, uint16_t y)
{
    uint16_t r, g, b;
    r = x/4;
    g = y/4;
    b = (127 -x)/4;
    return (((r & 0x1f) << 11) | ((g & 0x1f) << 6 ) | (b & 0x1f));
}

uint16_t get_rgb_color(uint16_t r, uint16_t g, uint16_t b){
    return (((r & 0x1f) << 11) | ((g & 0x1f) << 6 ) | (b & 0x1f));
}

int main(void)
{
    uint16_t cursor_x, cursor_y;
    init();
    irq_counter = 0;
    offset      = 0;
    button_up = button_down = button_back = button_next = 0;
    lcd_fillScreen( ST7735_BLACK );
	cursor_x = cursor_y = 0;
    while(1){
        
        while(cursor_x < 124){
            lcd_fillScreen(get_color(cursor_x, cursor_y));
            cursor_x += 4;
        }
        while(cursor_y < 124){
            lcd_fillScreen(get_color(cursor_x, cursor_y));
            cursor_y += 4;
        }
        while(cursor_x > 0){
            lcd_fillScreen(get_color(cursor_x, cursor_y));
            cursor_x -=4;
        }
        cursor_x = 0;
        while(cursor_y > 0){
            lcd_fillScreen(get_color(cursor_x, cursor_y));
            cursor_y -= 4;
        }
        cursor_y = 0;
        
    }

}
