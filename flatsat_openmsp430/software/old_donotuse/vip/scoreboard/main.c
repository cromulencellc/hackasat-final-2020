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

wakeup interrupt (WDT_VECTOR) INT_Watchdog(void) {
 
  irq_counter++;
  if (irq_counter == 300) {
    irq_counter = 0;
    offset = (offset+1) % 20;
  }
}
 
volatile uint8_t timer_count;
volatile int seconds;

interrupt (TIMERA0_VECTOR) TimerA_Interrupt( void )
{
    timer_count++;
    if (timer_count == 100){
        seconds+=1;
        timer_count = 0;
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
    
    timer_count = seconds = 0;

    CCR0 = 20000; 
    CCTL0 |= CCIE;
    TACTL = ID_3 | TASSEL_2 | MC_1; // ACLK, upmode 

    

    eint();                            //enable interrupts
    
    lcd_init_screen( INITR_BLACKTAB );
    lcd_init_gfx( 160, 128 );
}

void testprint(char *s)
{
	printf("%s\r", s);
	lcd_gfx_print(s);
}

int main(void)
{
    init();
    irq_counter = 0;
    offset      = 0;
    button_up = button_down = button_back = button_next = 0;

    lcd_fillScreen( COLOR_LEGIT_ORANGE );
    lcd_gfx_drawChar(30, 50, 0x56, COLOR_LEGIT_PURPLE, COLOR_LEGIT_ORANGE, 4);
    lcd_gfx_drawChar(70, 50, 0x49, COLOR_LEGIT_PURPLE, COLOR_LEGIT_ORANGE, 4);
    lcd_gfx_drawChar(110, 50, 0x50, COLOR_LEGIT_PURPLE, COLOR_LEGIT_ORANGE, 4);

    lcd_gfx_setTextColor(COLOR_LEGIT_PURPLE, COLOR_LEGIT_ORANGE);
	testprint("Testing Buttons..\n");

    while(1) 
    {
        LPM0;

		if (button_up == 1){
            button_up = 0;
		
            lcd_fillScreen( COLOR_LEGIT_ORANGE );
            lcd_gfx_drawChar(30, 50, 0x56, COLOR_LEGIT_PURPLE, COLOR_LEGIT_ORANGE, 4);
            lcd_gfx_drawChar(70, 50, 0x49, COLOR_LEGIT_PURPLE, COLOR_LEGIT_ORANGE, 4);
            lcd_gfx_drawChar(110, 50, 0x50, COLOR_LEGIT_PURPLE, COLOR_LEGIT_ORANGE, 4);

            lcd_gfx_setTextColor(COLOR_LEGIT_PURPLE, COLOR_LEGIT_ORANGE);

        }
        if (button_down == 1){
            lcd_fillScreen( COLOR_LEGIT_PURPLE);
            lcd_gfx_drawChar(30, 50, 0x56, COLOR_LEGIT_ORANGE, COLOR_LEGIT_PURPLE, 4);
            lcd_gfx_drawChar(70, 50, 0x49, COLOR_LEGIT_ORANGE, COLOR_LEGIT_PURPLE, 4);
            lcd_gfx_drawChar(110, 50, 0x50, COLOR_LEGIT_ORANGE, COLOR_LEGIT_PURPLE, 4);

            lcd_gfx_setTextColor(COLOR_LEGIT_PURPLE, COLOR_LEGIT_ORANGE);
            button_down =0;
						
        }
        if (button_back == 1){
            button_back = 0;
						
            lcd_fillScreen( COLOR_LEGIT_PURPLE);
            lcd_gfx_drawChar(30, 50, 0x56, ST7735_WHITE  , COLOR_LEGIT_PURPLE , 4);
            lcd_gfx_drawChar(70, 50, 0x49, ST7735_WHITE , COLOR_LEGIT_PURPLE , 4);
            lcd_gfx_drawChar(110, 50, 0x50, ST7735_WHITE , COLOR_LEGIT_PURPLE , 4);

            lcd_gfx_setTextColor(COLOR_LEGIT_PURPLE, COLOR_LEGIT_ORANGE);
        }
        if (button_next == 1) {
            unsigned char stringbuf[32];
            button_next = 0;
            sprintf(stringbuf, "seconds: %d\n", seconds);
            testprint(stringbuf);
        }

    } 

    return 0;
}
