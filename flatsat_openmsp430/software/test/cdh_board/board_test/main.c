//----------------------------------------------------//
// Cromulence LLC Hackasat 2020
//
// Board Test Application
// This code is designed to run on the application processor to test
// the hardware accessible from the app processor.
// That includes:
//      Console UART
//      Radio UART (to Leon-3)
//      Radio Chipset (RFM95W via RADIOSPI)
//      Program SPI Flash (for Leon-3)
//      LEDs
//----------------------------------------------------//

#include <stdio.h>
#include "cdh_common.h"
#include "delay.h"
#include "radio_hal.h"
#include "conflash.h"
#include "radio_process.h"

volatile unsigned int timer_count = 0;
volatile unsigned int seconds = 0;
volatile unsigned int wake_flag = 0;

#define WAKE_TIMER		0x1
#define WAKE_PORT1		0x2
#define WAKE_PORT2		0x4
#define WAKE_CONSOLE_UART_RX	0x8
#define WAKE_APP_UART_RX	0x10


#if 0
//----------------------------------------------------//
// TimerA Interrupt
// In order for this interrupt to fire every 100 ms, use this init code
// (for a 16MHz clock):
//      CCR0 = 20000; 
//      CCTL0 |= CCIE;
//      TACTL = ID_3 | TASSEL_2 | MC_1;  
//  ID_0 -> Divide by 1
//  ID_1 -> Divide by 2
//  ID_2 -> Divide by 4
//  ID_3 -> Divide by 8
//
//  TASSEL_0 -- Timer A Clock
//  TASSEL_1 -- A clock
//  TASSEL_2 -- SMCLK (master clock)
//  TASSEL_3 -- INCLK (unused)
//
//  MC_0 -- Stop, timer halted
//  MC_1 -- Up mode, timer counts up to TACCR0
//  MC_2 -- Continuous mode, timer counts up to 0xFFFF
//  MC_3 -- Up/down mode, timer counts up to TACCR0 then down to 0x000
//----------------------------------------------------//
interrupt (TIMERA0_VECTOR) TimerA_Interrupt( void )
{
    if (timer_count == 100){
        seconds+=1;
        timer_count = 0;

	wake_flag |= WAKE_TIMER;
        LPM0_EXIT;
    }

}

//----------------------------------------------------//
// PORT1 Interrupt -- Button Depress
//----------------------------------------------------//
interrupt (PORT1_VECTOR) INT_button(void)
{
    // Using Low Power Mode to synchronize with main loop
    wake_flag |= WAKE_PORT1;
    LPM0_EXIT;
}
#endif

//--------------------------------------------------//
//                 putChar function                 //
//            (Send a byte to the UART)             //
//--------------------------------------------------//
int putchar (int txdata) 
{

    // Wait until the TX buffer is not full
    while ( CONSOLE_UART_STAT & UART_TX_FULL );

    // Write the output character
    CONSOLE_UART_TXD = txdata;

    return 0;
}

//--------------------------------------------------//
//        UART RX interrupt service routine         //
//         (receive a byte from the UART)           //
//--------------------------------------------------//
volatile char console_rxdata;
 
wakeup interrupt (CONSOLE_UART_RX_VECTOR) INT_console_uart_rx(void) 
{

   
    // Read the received data
    console_rxdata = CONSOLE_UART_RXD;

    // Clear the receive pending flag
    CONSOLE_UART_STAT = UART_RX_PND;

    // Set wake flag
    wake_flag |= WAKE_CONSOLE_UART_RX;

    // Exit low power mode
    LPM0_EXIT;
}

volatile char app_rxdata;
 
wakeup interrupt (APP_UART_RX_VECTOR) INT_app_uart_rx(void) 
{

   
    // Read the received data
    app_rxdata = APP_UART_RXD;

    // Clear the receive pending flag
    APP_UART_STAT = UART_RX_PND;

    // Set wake flag
    wake_flag |= WAKE_APP_UART_RX;

    // Exit low power mode
    LPM0_EXIT;
}

void conflash_test( void )
{
	uint32_t erase_address = 0x100000;
	uint8_t data_set[32];
	uint8_t i;

	printf( "Performing CONFLASH Test:\n" );

	printf( "Erasing sector at address %d\n" );
	conflash_write_enable();
	conflash_ss_erase( erase_address );

	while( conflash_wip_check() )
		; // Do nothing


	for ( i = 0; i < 32; i++ )
		data_set[i] = i;

	printf( "Programming 32-bytes at erased address:\n" );
	conflash_page_program( erase_address, data_set, 32 );

	for ( i = 0; i < 32; i++ )
		data_set[i] = 0;

	printf( "Reading back!\n" );

	conflash_read_bytes( erase_address, data_set, 32 );

	for ( i = 0; i < 32; i++ )
		printf( "%02d: %02X\n", i, data_set[i] );

	printf( "\n" );
}


//----------------------------------------------------//
// Hardware Init
//----------------------------------------------------//
void init(void)
{ 
    WDTCTL = WDTPW | WDTHOLD;          // Disable watchdog timer

    // Console UART initialization 
    CONSOLE_UART_BAUD = CONSOLE_UART_BAUD_VALUE;                  // Init UART
    CONSOLE_UART_CTL = UART_EN | UART_IEN_RX; 			//Enable UART 

    APP_UART_BAUD = APP_UART_BAUD_VALUE;
    APP_UART_CTL = UART_EN | UART_IEN_RX;

    // Initialization GPIO
    P1OUT  = 0x00;                     // Port data output
    P2OUT  = 0x00;
 
    P1DIR  = 0x00;                     // Port direction register
    P2DIR  = 0xff;

#if 0 
    P1IES  = 0x00;                     // Port interrupt enable (0=dis 1=enabled)
    P2IES  = 0x00;
    P1IE   = 0x0F;                     // Port interrupt Edge Select (0=pos 1=neg)
    P2IE   = 0x00;
#endif
 
    //WDTCTL = WDTPW | WDTTMSEL | WDTCNTCL;// | WDTIS1  | WDTIS0 ;          // Configure watchdog interrupt
#if 0    
    timer_count = seconds = 0;
    CCR0 = 20000; 
    CCTL0 |= CCIE;
    
    // Clock divider = div by 8 | Source = SMCLOCK | Mode = Countinuous up
    TACTL = ID_3 | TASSEL_2 | MC_1;  

    wake_flag = 0;
#endif
    
    
    // Init Conflash
    conflash_init();

    printf( "CONFLASH ID: %08X\n", conflash_readid() );

    conflash_test();

    printf( "Initializing radio!\n" );

    radio_init();
    
    printf( "Radio Init Complete!\n" );
}

void board_test(void)
{
    char buf[40];
    int pos = 0;

    printf("BOARD TEST\r\n");
    P2OUT |= 0x03;
    while(1)
    {
        // Wait for interrupts
        LPM0;

        // Toggle LEDs
        if (wake_flag & WAKE_TIMER)
        {
            if (P2OUT & 0x03) 
                P2OUT &= 0xFC;
            else
                P2OUT |= 0x03; 
        }

        // Handle UART RX
        if (wake_flag & WAKE_CONSOLE_UART_RX)
        {
            wake_flag ^= WAKE_CONSOLE_UART_RX;
            switch (console_rxdata) 
            {
                //process RETURN key
                case '\r':
                    printf("\r\n");     //finish line
                    buf[pos++] = 0;     //to use printf...
                    printf("CONSOLE: %s\r\n", buf);
                    pos = 0;            //reset buffer
                    break;

                //backspace
                case '\b':
                    if (pos > 0) {      //is there a char to delete?
                        pos--;          //remove it in buffer
                        putchar('\b');  //go back
                        putchar(' ');   //erase on screen
                        putchar('\b');  //go back
                    }
                    break;

                //other characters
                default:
                    //only store characters if buffer has space
                    if (pos < sizeof(buf)) 
                    {
                        putchar(console_rxdata);     //echo
                        buf[pos++] = console_rxdata; //store
                    }
                    break;
                
            }
        }
    }
    P2OUT &= 0xFC;
}


void radio_test(void)
{

} 

int main(void)
{
    printf( "Initializing peripherals!\n" );
    // Init peripherals
    init();
   
    printf( "Enabling interrupts!\n" ); 
    // Enable interrupts
    eint();                            

    while(1)
    {
        // Wait for interrupts
        LPM0;

        if (wake_flag & WAKE_TIMER)
        {
            wake_flag ^= WAKE_TIMER;

	    // TODO: Timer Test
        }

#if 0
        if (wake_flag & WAKE_BUTTON)
        {
            wake_flag ^= WAKE_BUTTON;
            power_on_idle = 0;
            if (button_up == 1)
            {
                button_up = 0;
                if (selection > 0) selection -= 1;
                lcd_gfx_printMenu(menu, menu_size, selection);
            }
            if (button_down == 1)
            {
                button_down = 0;
                if (selection < menu_size - 1) selection += 1;
                lcd_gfx_printMenu(menu, menu_size, selection);
            }
            if (button_back == 1) 
            {
                button_back = 0;
            }
            
            if (button_next == 1)
            {
                button_next = 0;
                lcd_gfx_setTextSize(1);
                switch (selection){
                    // ALL
                    case 0:
                        lcd_test();
                        sram_test();
                        radio_test();
                        io_test();
                        break;
                    // IO (UART, BUTTONS, LEDs)
                    case 1:
                        io_test();
                        break;
                    // LCD
                    case 2:
                        lcd_test();
                        break;
                    // SRAM
                    case 3:
                        sram_test();
                        break;
                    // RADIO
                    case 4:
                        radio_test();
                        break;
                }
                lcd_gfx_setTextSize(2);
                lcd_fillScreen(lcd_gfx_getBackgroundColor());
                lcd_gfx_printMenu(menu, menu_size, selection);
                button_up = button_down = button_back = button_next = 0;
            }    
        } 
#endif
    }
}
