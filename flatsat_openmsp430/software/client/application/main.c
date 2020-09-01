#include "omsp_system.h"
#include "hardware.h"
#include "sram_hal.h"
#include "lcd_hal.h"
#include "lcd_gfx.h"
#include "radio_uart.h"
#include "cli.h"
#include "gui.h"
#include <stdlib.h>
#include <stdio.h>

volatile uint8_t g_myTeamID;

/**
* Run the application code (display messages from the radio and send responses)
*/
int main(void) 
{
	WDTCTL = WDTPW | WDTHOLD;	// Disable watchdog timer

	P1OUT  = 0x00;                     // Port data output
	P2OUT  = 0x00;
 
	P1DIR  = 0x00;                     // Port direction register
	P2DIR  = 0xff;
 
	P1IES  = 0x00;                     // Port interrupt enable (0=dis 1=enabled)
	P2IES  = 0x00;
	P1IE   = 0x0F;                     // Port interrupt Edge Select (0=pos 1=neg)
	P2IE   = 0x00;

	sram_init();
	cli_init_uart();
	radio_init_uart();
	init_gui();
	
	g_myTeamID = TEAM_ID_ADDR; 
	
	puts( "Application Core v1.0" );
	puts( "openMSP430 core by Oliver Girard" );
	puts( "p.s. I modded the core to make data executable -sirgoon" );

	// Enable interrupts
	eint();

	// Run process loop
	while ( 1 )
	{
		LPM0;
		cli_run();
		radio_uart_run();
	}
}
