#include <stdio.h>
#include "hardware.h"
#include "delay.h"
#include "lcd/lcd_hal.h"
#include "lcd/lcd_gfx.h"

extern void pong(void);
extern uint16_t color_picker(void);
extern void screen_saver(void);
extern void scoreboard(void);
extern void rain(void);
extern void pong2(void);

uint8_t button_poll = 0;
volatile uint8_t sequence[8] = {0,0,0,0,0,0,0,0};
volatile uint8_t timer_count;
volatile unsigned int seconds;
volatile unsigned int activity_timer;

/* TimerA Interrupt
* In order for this interrupt to fire every 100 ms, use this init code
* for a 16MHz clock:
*   CCR0 = 20000; 
*   CCTL0 |= CCIE;
*   TACTL = ID_3 | TASSEL_2 | MC_1;  
*/
interrupt (TIMERA0_VECTOR) TimerA_Interrupt( void )
{
    timer_count++;
    
    if (button_poll){
        if ((timer_count % 1) ==  0){
            LPM0_EXIT;
        }
    }
    if (timer_count == 100){
        seconds+=1;
        activity_timer+=1;
        timer_count = 0;
        LPM0_EXIT;
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
volatile uint8_t button_next, button_up, button_down, button_back;

interrupt (PORT1_VECTOR) INT_button(void)
{
    activity_timer = 0;

    sequence[7] = sequence[6];
    sequence[6] = sequence[5];
    sequence[5] = sequence[4];
    sequence[4] = sequence[3];
    sequence[3] = sequence[2];
    sequence[2] = sequence[1];
    sequence[1] = sequence[0];

    if ( BUTTON_FLAG & BUTTON_NEXT_BIT )
    {
        button_next =1;
        sequence[0] = 3;
        BUTTON_FLAG ^= BUTTON_NEXT_BIT;
    }
    if (BUTTON_FLAG & BUTTON_DOWN_BIT){
        button_down = 1;
        sequence[0] = 2;
        BUTTON_FLAG ^= BUTTON_DOWN_BIT;
    }
    if ( BUTTON_FLAG & BUTTON_UP_BIT ){
        button_up = 1;
        sequence[0] = 1;
        BUTTON_FLAG ^= BUTTON_UP_BIT;
    }
    if (BUTTON_FLAG & BUTTON_BACK_BIT){
        button_back = 1;
        sequence[0] = 4;
        BUTTON_FLAG ^= BUTTON_BACK_BIT;
    }
    // Using Low Power Mode to synchronize with main loop
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
    // Clock divider = div by 8 | Source = SMCLOCK | Mode = Countinuous up
    TACTL = ID_3 | TASSEL_2 | MC_1;  

    eint();                            //enable interrupts
    
    lcd_init_gfx( 160, 128 );
    lcd_fillScreen( ST7735_BLACK );
    lcd_init_screen( INITR_BLACKTAB );
    

    

    button_up = button_down = button_back = button_next = 0;
    activity_timer = 0;
}

void testprint(char *s)
{
	printf("%s\r", s);
	lcd_gfx_print(s);
}

int main(void)
{

    uint8_t selection, menu_size;
    uint16_t main_bgcolor, main_fgcolor;
    char *menu[] = {"PONG", "PONG-2", "COLOR PICK-BG", "COLOR PICK-FG", "SCREEN SAVER", "SCOREBOARD", "RAIN", "RESET"};
    selection = 0;
    menu_size = 8;
    init();
    lcd_gfx_setBackgroundColor(COLOR_LEGIT_PURPLE);
    lcd_gfx_setForegroundColor(COLOR_LEGIT_GOLD);
  
    scoreboard();
    button_up = button_down = button_back = button_next = 0;

    lcd_fillScreen(lcd_gfx_getBackgroundColor());
    lcd_gfx_setTextSize(2);
    lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor());
    lcd_fillScreen(lcd_gfx_getBackgroundColor());
    lcd_gfx_printMenu(menu, menu_size, selection);
    while(1){

        if ((button_up == 1) || (BUTTON_PORT & BUTTON_UP_BIT)) {
            button_up = 0;
            if (selection > 0) selection -= 1;
            lcd_gfx_printMenu(menu, menu_size, selection);
        }
        if ((button_down == 1) || (BUTTON_PORT & BUTTON_DOWN_BIT)){
            button_down = 0;
            if (selection < menu_size - 1) selection += 1;
            lcd_gfx_printMenu(menu, menu_size, selection);
        }
        if ((button_back == 1) || (BUTTON_PORT & BUTTON_BACK_BIT)){
            button_back = 0;
        }
        if ((button_next == 1) || (BUTTON_PORT & BUTTON_NEXT_BIT)){
            button_next = 0;
            switch (selection){
                case 0:
                    button_poll = 1;
                    pong();
                    button_poll = 0;
                    break;
                case 1:
                    button_poll = 1;
                    pong2();
                    button_poll = 0;
                    break;
                case 2:
                    button_poll = 1;
                    main_bgcolor = color_picker();
                    button_poll = 0;
                    if (main_bgcolor != lcd_gfx_getForegroundColor()){
                        lcd_gfx_setBackgroundColor(main_bgcolor);
                        lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), main_bgcolor);
                    }
                    break;
                case 3:
                    button_poll = 1;
                    main_fgcolor = color_picker();
                    button_poll = 0;
                    if (main_fgcolor != lcd_gfx_getBackgroundColor()){
                        lcd_gfx_setForegroundColor(main_fgcolor);
                        lcd_gfx_setTextColor(main_fgcolor, lcd_gfx_getBackgroundColor());
                    }
                    break;
                case 4:
                    screen_saver();
                    break;
                case 5:
                    scoreboard();
                    lcd_gfx_setTextSize(2);
                    break;
                case 6:
                    rain();
                    break;
                case 7:
                    lcd_gfx_setForegroundColor(COLOR_LEGIT_GOLD);
                    lcd_gfx_setBackgroundColor(COLOR_LEGIT_PURPLE);
                    lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor());
                    break;
            }

            lcd_fillScreen(lcd_gfx_getBackgroundColor());
            lcd_gfx_printMenu(menu, menu_size, selection);

        }    

        // Screen Saver after period of inactivity (seconds)
        if (activity_timer > 60){
            screen_saver();
            lcd_fillScreen(lcd_gfx_getBackgroundColor());
            lcd_gfx_printMenu(menu, menu_size, selection);
        }
        button_up = button_down = button_back = button_next = 0;
        LPM0;
        
    }

}
