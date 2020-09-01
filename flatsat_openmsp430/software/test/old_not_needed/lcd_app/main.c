#include "hardware.h"
#include "lcd_hal.h"
#include "lcd_gfx.h"

/**
This one is executed onece a second. it counts seconds, minues, hours - hey
it shoule be a clock ;-)
it does not count days, but i think you'll get the idea.
*/
volatile int irq_counter, offset;
 
wakeup interrupt (WDT_VECTOR) INT_Watchdog(void) {
 
  irq_counter++;
  if (irq_counter == 300) {
    irq_counter = 0;
    offset = (offset+1) % 20;
  }
}
 
 
/**
Main function with some blinking leds
*/
int main(void) 
{
    uint8_t x_pos, x_pos_old; 
    irq_counter = 0;
    offset      = 0;
 
    WDTCTL = WDTPW | WDTHOLD;          // Disable watchdog timer
 
    P1OUT  = 0x00;                     // Port data output
    P2OUT  = 0x00;
 
    P1DIR  = 0x00;                     // Port direction register
    P2DIR  = 0xff;
 
    P1IES  = 0x00;                     // Port interrupt enable (0=dis 1=enabled)
    P2IES  = 0x00;
    P1IE   = 0x00;                     // Port interrupt Edge Select (0=pos 1=neg)
    P2IE   = 0x00;
 
    //WDTCTL = WDTPW | WDTTMSEL | WDTCNTCL;// | WDTIS1  | WDTIS0 ;          // Configure watchdog interrupt
 
    //IE1 |= 0x01;
    //eint();                            //enable interrupts
 

    lcd_init_screen( INITR_BLACKTAB );

    lcd_fillScreen( ST7735_RED );

    lcd_init_gfx( 160, 128 );
    lcd_gfx_setRotation( 0 );

    lcd_gfx_fillCircle( 50, 50, 10, ST7735_GREEN );
    lcd_gfx_fillCircle( 100, 50, 10, ST7735_GREEN );
    lcd_gfx_fillCircle( 50, 100, 10, ST7735_GREEN );

    x_pos_old = 10;
    while ( 1 )
    {
    for ( x_pos = 10; x_pos < 80; x_pos++ ) 
    {                         // Main loop, never ends...
        delay_ms( 50 );

	// ERASE
	lcd_fillRect( x_pos_old, 30, (6*4), 8, ST7735_RED );

	// DRAW
	lcd_gfx_drawChar( x_pos, 30, 0x41, ST7735_GREEN, ST7735_RED, 1 );
	lcd_gfx_drawChar( x_pos+6, 30, 0x42, ST7735_GREEN, ST7735_RED, 1 );
	lcd_gfx_drawChar( x_pos+12, 30, 0x43, ST7735_GREEN, ST7735_RED, 1 );
	lcd_gfx_drawChar( x_pos+18, 30, 0x44, ST7735_GREEN, ST7735_RED, 1 );
        
        x_pos_old = x_pos; 
    }
    }

}
