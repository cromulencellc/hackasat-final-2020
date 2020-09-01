#include "hardware.h"

#define SRAM_PORT	(P2OUT)
#define SRAM_CSN_BIT	(0x08)

void sram_spi_init( void )
{
	// Set everything low except the clock divider to 4Mhz
	SRAMSPI_CTRL = (0x0000);
}

void sram_init( void )
{
	// Bring high the LCD Chip Select
	SRAM_PORT |= SRAM_CSN_BIT;

	// Init spi controller
	sram_spi_init();
}

void spiwrite_byte( unsigned char data )
{
	P2OUT |= 1;
	SRAMSPI_DATA = data;
	SRAMSPI_CTRL |= SRAMSPI_CTRL_EN;

	while ( SRAMSPI_CTRL & SRAMSPI_CTRL_EN )
		;

	P2OUT &= ~1;
}

unsigned char spiread_byte( void )
{
	unsigned char data;

	SRAMSPI_DATA = 0x0;
	SRAMSPI_CTRL |= SRAMSPI_CTRL_EN;

	while ( SRAMSPI_CTRL & SRAMSPI_CTRL_EN )
		;

	data = SRAMSPI_DATA;

	return (data);
}

unsigned int sram_read_mode( void )
{
	unsigned char readbyte;

	SRAM_PORT &= ~SRAM_CSN_BIT;
	spiwrite_byte( 0x5 );
	readbyte = spiread_byte();

	SRAM_PORT |= SRAM_CSN_BIT;

	return (readbyte);
}

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

void delay_ms( unsigned int ms )
{
	delay( ms, 0x7ff );
}


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
int main(void) {

    unsigned char mode;
 
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
 
    if (CPU_NR==0x0100) { 
      delay(0x000f, 0xffff);
    } 

    sram_init();

    mode = sram_read_mode();

    if ( mode == 0x40 )
    {
    while (1) 
    {                         // Main loop, never ends...
      // Status LEDs at P2[0] and P2[1] bit positions... 
      P2OUT = 0x00;
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
    }
    }

}
