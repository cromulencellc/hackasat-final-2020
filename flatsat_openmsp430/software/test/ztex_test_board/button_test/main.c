#include "omsp_system.h"
#include "hardware.h"
#include <stdlib.h>
#include <stdio.h>


#define LED1_BIT	0x1
#define LED2_BIT	0x2
#define LED_PORT	(P2OUT)

#define BUTTON_NEXT_BIT	0x1
#define BUTTON_BACK_BIT 0x2
#define BUTTON_UP_BIT	0x4
#define BUTTON_DOWN_BIT	0x8
#define BUTTON_PORT	(P1IN)

//--------------------------------------------------//
//                   Delay function                 //
//--------------------------------------------------//
void delay(unsigned int d) {
   while(d--) {
      nop();
      nop();
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
 
//----------------------------------------------------//
// PORT1 Interrupt -- Button Depress
//----------------------------------------------------//
interrupt (PORT1_VECTOR) INT_button(void) 
{
	if ( BUTTON_PORT & BUTTON_NEXT_BIT )
		LED_PORT |= LED1_BIT;
	else
		LED_PORT &= ~LED1_BIT;

	if ( BUTTON_PORT & BUTTON_DOWN_BIT )
		LED_PORT |= LED2_BIT;
	else
		LED_PORT &= ~LED2_BIT;
}
 
 
//--------------------------------------------------//
// Main function with init an an endless loop that  //
// is synced with the interrupts trough the         //
// lowpower mode.                                   //
//--------------------------------------------------//
int main(void) 
{
    WDTCTL = WDTPW | WDTHOLD;           // Disable watchdog timer
 
    UART_BAUD = BAUD;                   // Init UART
    UART_CTL  = UART_EN;

    eint();

    P2OUT = 0x01;
    P2DIR = 0x03;

    P1DIR = 0x00;
    P1IE = 0xF; 

	printf( "Running...\n" );

    while ( 1 )
    { }

	return 0;
}
