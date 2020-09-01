//----------------------------------------------------//
// VIP Badge Application
//  This code is designed to run on the app processor
//  of the VIP badge. 
//  It receives messages from the server badge (via the
//  radio processor), displays a scoreboard, and has a 
//  'secret' menu for other options and games. 
//----------------------------------------------------//

#include <stdio.h>
#include "vip.h"
#include "hardware.h"
#include "delay.h"
#include "lcd/lcd_hal.h"
#include "lcd/lcd_gfx.h"

// Determines when to poll buttons 
uint8_t button_poll = 1;

// Holds last 8 button pushes (for secret menu)
volatile uint8_t sequence[8];
static uint8_t contra[] = {4,3,4,3,2,2,1,1};
// Easy mode will allow 4-button press to access menu (instead of contra code)
uint8_t easy_mode = 0;

// Time counters
volatile uint8_t timer_count = 0;
volatile uint32_t seconds = 0;
volatile uint32_t activity_timer = 0;

// Records why we wake from LPM0 (values defined in vip.h)
volatile uint8_t wake_flag;

uint32_t announcement_timer = 0;

//----------------------------------------------------//
// TimerA Interrupt
// In order for this interrupt to fire every 100 ms, use this init code
// (for a 16MHz clock):
//      CCR0 = 20000; 
//      CCTL0 |= CCIE;
//      TACTL = ID_3 | TASSEL_2 | MC_1;  
//----------------------------------------------------//
interrupt (TIMERA0_VECTOR) TimerA_Interrupt( void )
{
    timer_count++;
    
    if (button_poll){
        if ((timer_count % 2) ==  0){
            wake_flag |= WAKE_BUTTON_POLL;
            LPM0_EXIT;
        }
    }
    if (timer_count == 100){
        seconds+=1;
        activity_timer+=1;
        timer_count = 0;
        wake_flag |= WAKE_TIMER;
        LPM0_EXIT;
    }

}

//----------------------------------------------------//
// PORT1 Interrupt -- Button Depress
//----------------------------------------------------//
// Remembers button presses until handled
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
    wake_flag |= WAKE_BUTTON;
    LPM0_EXIT;
}

//--------------------------------------------------//
//                 putChar function                 //
//            (Send a byte to the UART)             //
//--------------------------------------------------//
int putchar (int txdata) 
{

    // Wait until the TX buffer is not full
    while (UART_STAT & UART_TX_FULL);
    // Write the output character
    UART_TXD = txdata;
    return 0;
}

//--------------------------------------------------//
//        UART RX interrupt service routine         //
//         (receive a byte from the UART)           //
//--------------------------------------------------//
volatile char rxdata;
// Holds last radio message
volatile char radio_msg[RADIO_MSG_LEN];
// Position in latest radio message
volatile uint16_t radio_msg_pos = 0;
volatile uint8_t radio_msg_pending = 0;
wakeup interrupt (RADIOUART_RX_VECTOR) INT_radio_uart_rx(void) {

    // Read the received data
    rxdata = APP_UART_RXD;
    if ((rxdata == '\n') && !radio_msg_pending)
    {
        radio_msg[radio_msg_pos] = '\0';
        radio_msg_pending = 1;
        P2OUT |= 0x03;
        
    }
    else if ((radio_msg_pos < RADIO_MSG_LEN) && !radio_msg_pending)
    {
        radio_msg[radio_msg_pos++] = rxdata;
        
    }
    
    // Clear the receive pending flag
    APP_UART_STAT = UART_RX_PND;
     // Exit the low power mode
    wake_flag |= WAKE_UART;
    LPM0_EXIT;
}



//----------------------------------------------------//
// Hardware Init
//----------------------------------------------------//
void init(void)
{ 
    WDTCTL = WDTPW | WDTHOLD;          // Disable watchdog timer
 
    UART_BAUD = BAUD;                  // Init UART
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

    // Init Screen
    lcd_init_screen( INITR_BLACKTAB );
    lcd_init_gfx( 160, 128 );
    lcd_fillScreen( COLOR_LEGIT_BLACK );

    // Init Radio UART
    APP_UART_BAUD = APP_BAUD;
    APP_UART_CTL = UART_EN | UART_IEN_RX;

    
    // Enable interrupts
    eint(); 
}


int main(void)
{
    uint8_t refresh = 1;
    static char *mainmenu[] = {"SCOREBOARD", "PONG", "SETTINGS"};
    uint8_t main_selection = 0;
    uint8_t main_menu_size = 3;
    char *settingsmenu[] = {"FOREGND COLOR", "BACKGND COLOR", "HILIGHT COLOR", "EASY MODE OFF", "RESET COLORS" }; 
    uint8_t settings_selection = 0;
    uint8_t settings_menu_size = 5;
    enum badge_modes {SCOREBOARD, PONG1, SETTINGS_MENU, MAIN_MENU, PICKFG, PICKBG, PICKNAME, PRE_ANNOUNCEMENT, ANNOUNCEMENT};
    enum badge_modes badge_mode = SCOREBOARD;
    init();

    lcd_gfx_setBackgroundColor(COLOR_LEGIT_PURPLE);
    lcd_gfx_setForegroundColor(COLOR_LEGIT_GOLD);
    lcd_gfx_setHighlightColor(COLOR_LEGIT_WHITE);
    lcd_fillScreen(lcd_gfx_getBackgroundColor());
    lcd_gfx_setTextSize(2);
    lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor());
    while(1)
    {
        if (radio_msg_pending)
        {
            if (process_incoming_message((char *)radio_msg, radio_msg_pos) == VIP_ANNOUNCEMENT_MSG)
            {
                badge_mode = PRE_ANNOUNCEMENT;
                refresh = 1;
                announcement_timer = seconds + VIP_PRE_ANNOUNCEMENT_TIME;
            }
            radio_msg_pending = 0;
            radio_msg_pos = 0;
            P2OUT &= 0xFC;
        }

        if ((sequence[0] == contra[0])&&
            (sequence[1] == contra[1])&&
            (sequence[2] == contra[2])&&
            (sequence[3] == contra[3])&&
            (sequence[4] == contra[4])&&
            (sequence[5] == contra[5])&&
            (sequence[6] == contra[6])&&
            (sequence[7] == contra[7]))
        {
            badge_mode = MAIN_MENU;
            refresh = 1;
            sequence[0] = 0;
            button_up = button_down = button_back = button_next = 0;
        }

        switch(badge_mode)
        {
            case SCOREBOARD:
                scoreboard(refresh); 
                refresh = 0; 
                if (easy_mode && ((BUTTON_PORT & 0x0F) == 0x0F))
                {
                    badge_mode = MAIN_MENU;
                    refresh = 1;
                    button_up = button_down = button_back = button_next = 0;
                }
                break;
            case MAIN_MENU:
                if (refresh)
                {
                    refresh = 0;
                    lcd_fillScreen(lcd_gfx_getBackgroundColor());
                    lcd_gfx_setTextSize(2);
                    lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor());
                    lcd_gfx_printMenu(mainmenu, main_menu_size, main_selection);
                }
                if (button_up == 1)  
                {
                    button_up = 0;
                    if (main_selection > 0) main_selection -= 1;
                    lcd_gfx_printMenu(mainmenu, main_menu_size, main_selection);
                }
                if (button_down == 1) 
                {
                    button_down = 0;
                    if (main_selection < main_menu_size - 1) main_selection += 1;
                    lcd_gfx_printMenu(mainmenu, main_menu_size, main_selection);
                }
                if (button_next == 1) 
                {
                    button_next = 0;
                    refresh = 1;
                    badge_mode = main_selection;
                }
                button_up = button_down = button_back = button_next = 0;
                break;
            case SETTINGS_MENU:
                if (refresh)
                {
                    refresh = 0;
                    lcd_fillScreen(lcd_gfx_getBackgroundColor());
                    lcd_gfx_printMenu(settingsmenu, settings_menu_size, settings_selection);
                }
                if (button_up == 1) 
                {
                    button_up = 0;
                    if (settings_selection > 0) settings_selection -= 1;
                    lcd_gfx_printMenu(settingsmenu, settings_menu_size, settings_selection);
                }
                if (button_down == 1) 
                {
                    button_down = 0;
                    if (settings_selection < settings_menu_size - 1) settings_selection += 1;
                    lcd_gfx_printMenu(settingsmenu, settings_menu_size, settings_selection);
                }
                if (button_next == 1) 
                { 
                    button_next = 0;
                    refresh = 1;   
                    switch(settings_selection)
                    {
                        case 0: 
                        // FOREGROUND COLOR
                            badge_mode = PICKFG;
                            break;
                        case 1:
                        // BACKGROUND COLOR
                            badge_mode = PICKBG;
                            break;
                        case 2:
                        // SET NAMETAG COLOR
                            badge_mode = PICKNAME;
                            break;
                        case 3:
                        // EASY MODE
                            if (easy_mode == 0)
                            {
                                easy_mode = 1;
                                settingsmenu[3] = "EASY MODE ON";
                            }
                            else
                            {
                                easy_mode = 0;
                                settingsmenu[3] = "EASY MODE OFF";
                            }
                            break;
                        case 4:
                        // RESET COLORS
                            lcd_gfx_setBackgroundColor(COLOR_LEGIT_PURPLE);
                            lcd_gfx_setForegroundColor(COLOR_LEGIT_GOLD);
                            lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor());
                            break;
                    }

                }
                if (button_back == 1) 
                { 
                    button_back = 0;
                    refresh = 1; 
                    badge_mode = MAIN_MENU;
                }
                button_up = button_down = button_back = button_next = 0;
                break;
            case PONG1:
                if (pong(refresh))
                {
                    refresh = 0;
                }
                else
                {
                    refresh = 1;
                    badge_mode = MAIN_MENU;
                }
                if ((BUTTON_PORT & 0x0F) == 0x0F)
                {
                    refresh = 1; 
                    badge_mode = MAIN_MENU;
                }
                button_up = button_down = button_back = button_next = 0;
                break;
            case PICKFG:
                color_picker(refresh);
                refresh = 0;
                if ((BUTTON_PORT & 0x0F) == 0x0F)
                {
                    if (get_current_color() != lcd_gfx_getBackgroundColor())
                    {
                        lcd_gfx_setForegroundColor(get_current_color());
                        lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor());
                    }    
                    badge_mode = SETTINGS_MENU;
                    refresh = 1;
                }
                button_up = button_down = button_back = button_next = 0;
                break;
            case PICKBG:
                color_picker(refresh);
                refresh = 0;
                if ((BUTTON_PORT & 0x0F) == 0x0F)
                {
                    if (get_current_color() != lcd_gfx_getForegroundColor())
                    {
                        lcd_gfx_setBackgroundColor(get_current_color());
                        lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor());
                    }  
                    badge_mode = SETTINGS_MENU;
                    refresh = 1;
                }
                button_up = button_down = button_back = button_next = 0;
                break;
            case PICKNAME:
                color_picker(refresh);
                refresh = 0;
                if ((BUTTON_PORT & 0x0F) == 0x0F)
                {
                    if (get_current_color() != lcd_gfx_getBackgroundColor())
                    {
                        lcd_gfx_setHighlightColor(get_current_color());
                    }    
                    badge_mode = SETTINGS_MENU;
                    refresh = 1;
                }
                button_up = button_down = button_back = button_next = 0;
                break;
            case PRE_ANNOUNCEMENT:
                pre_announcement(refresh, announcement_timer - seconds);
                refresh = 0;
                if (button_next == 1) 
                {
                    button_next = 0;
                    announcement_timer = seconds;
                }
                if (announcement_timer <= seconds)
                {
                    announcement_timer = seconds + VIP_ANNOUNCEMENT_TIME;
                    badge_mode = ANNOUNCEMENT;
                    refresh = 1;
                }
                button_up = button_down = button_back = button_next = 0;
                break;
            case ANNOUNCEMENT:
                announcement(refresh, announcement_timer - seconds);
                refresh = 0;
                if (button_next == 1) 
                {
                    button_next = 0;
                    announcement_timer = seconds;
                }
                if (announcement_timer <= seconds)
                {
                    badge_mode = SCOREBOARD;
                    lcd_invertOff();
                    refresh = 1;
                }
                button_up = button_down = button_back = button_next = 0;
                break;

        }
        if (refresh != 1)
        {
            // Wait for interrupts
            LPM0;
        }
    }

}
