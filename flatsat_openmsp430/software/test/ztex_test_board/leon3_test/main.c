#include "hardware.h"
#include <stdio.h>
#include <stdlib.h>
 
/**
Delay function.
*/
void delay(unsigned int c, unsigned int d) 
{
  volatile int i, j;
  for (i = 0; i<c; i++) {
    for (j = 0; j<d; j++) {
      nop();
      nop();
    }
  }
}

//--------------------------------------------------//
//                 putChar function                 //
//            (Send a byte to the UART)             //
//--------------------------------------------------//
int putchar (int txdata) {

  // Wait until the TX buffer is not full
  while (UART_STAT & UART_TX_FULL);

  // Write the output character
  UART_TXD = txdata;

  return 0;
}
 
volatile int timer_count;

wakeup interrupt (TIMERA0_VECTOR) TimerA_Interrupt( void )
{
	timer_count++;

	if ( timer_count >= 50 )
	{
		//P2OUT ^= 0x20;

		timer_count = 0;
	}
}
 
 
/**
Main function with some blinking leds
*/
int main(void) 
{
    volatile int i,j; 
    timer_count = 0;
 
    WDTCTL = WDTPW | WDTHOLD;          // Disable watchdog timer

    UART_BAUD = BAUD;                   // Init UART
    UART_CTL  = UART_EN | UART_IEN_RX;
 
 
    P2DIR  = 0xFF;
    P2OUT  = 0x08;	// Turn on Leon-3

    printf( "Begin Leon-3 Test...\n" );
 
    //WDTCTL = WDTPW | WDTTMSEL | WDTCNTCL;// | WDTIS1  | WDTIS0 ;          // Configure watchdog interrupt
#if 0
    TACCR0 = 60000;
    TACTL = ID_3 | TASSEL_2 | MC_1;
    TACCTL0 |= CCIE;
#endif

    //eint();
    //IE1 |= 0x01;
    //eint();                            //enable interrupts
 
    while (1) 
    {                         // Main loop, never ends...
      //delay(0x0000, 0xffff);
        for ( i = 0; i < 0x3; i++ )
	{
	    for ( j = 0; j < 0xffff; j++ )
	    {
                nop();
		nop();
	    }
	}
      P2OUT ^= 0x20;
      // Status LEDs at P2[0] and P2[1] bit positions... 
/*      P2OUT = 0x00;
      delay(0x000f, 0xffff);
 
      P2OUT = 0x01;
      delay(0x000f, 0xffff);
 
      P2OUT = 0x02;
      delay(0x000f, 0xffff);
 
      P2OUT = 0x03;
      delay(0x000f, 0xffff);
 
      P2OUT = 0x02;
      delay(0x000f, 0xffff);
 
      P2OUT = 0x01;
      delay(0x000f, 0xffff);
*/
 //     delay(0x000f, 0xffff);
//	printf( "Time: %d\n", timer_count ); 
//
    }
}
