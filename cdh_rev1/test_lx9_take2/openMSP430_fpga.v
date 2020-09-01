//----------------------------------------------------------------------------
// Copyright (C) 2011 Authors
//
// This source file may be used and distributed without restriction provided
// that this copyright statement is not removed from the file and that any
// derivative work contains the original copyright notice and the associated
// disclaimer.
//
// This source file is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation; either version 2.1 of the License, or
// (at your option) any later version.
//
// This source is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this source; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
//----------------------------------------------------------------------------
// 
// *File Name: openMSP430_fpga.v
// 
// *Module Description:
//          Top-level design for the Hackasat Finals Flatsat. This runs on the
// Ztex 2.13c module on the Cromulence designed C&DH carrier board with the
// Raspberry Pi Zero Payload Module. There are two softcore processors
// present in the FPGA, a Leon-3 processor from Gaissler and an OpenMSP430
// processor from Olivier Girard (thank you Olivier for this awesome open source project!). 
// The OpenMSP430 processor acts as the main power-on reset processor and radio baseband.
// The Leon-3 processor is responsible for running the Flight Software of the
// Flatsat.
//
// The Flatsat, challenges, hardware, software, and many of the technical items of the
// competition were made possible by the hard work, countless hours, passion and dedication
// of the members of the Hackasat Cromulence LLC team.
//
// *Author(s):
//			    - Cromulence LLC team members
//              - Jason Williams, aka sirgoon, jdw@cromulence.com
//              - Ricardo Ribalda,    ricardo.ribalda@gmail.com
//              - Olivier Girard,     olgirard@gmail.com
//
//----------------------------------------------------------------------------

module openMSP430_fpga (

	//----------------------------------------------
	// M25PE40
	// SPI Configuration FLash
	//----------------------------------------------
	MASTER_CONFIG_SCK,
	MASTER_CONFIG_CSN,
	MASTER_CONFIG_MISO,
	MASTER_CONFIG_MOSI,



	//----------------------------------------------
	// Radio
	//   SX1272
	//----------------------------------------------
	RADIO_SCK,
	RADIO_NSS,
	RADIO_MISO,
	RADIO_MOSI,

	RADIO_DIO0,
	RADIO_DIO1,
	RADIO_DIO2,
	RADIO_DIO3,
	RADIO_DIO4,
	RADIO_DIO5,

	RADIO_RESET,

	//----------------------------------------------
	// Battery Indicators (output)
	//----------------------------------------------
	BATTERY_GREEN_LED,
	BATTERY_YELLOW_LED,

	//----------------------------------------------
	// Status Indicators (output)
	//----------------------------------------------
	STATUS_LED_MSP430,
	STATUS_LED_RADIOLINK,
	STATUS_LED_LEON_HEARTBEAT,
	STATUS_LED_EXTRA,
	
	LEON3_LED4,
	LEON3_LED5,
	LEON3_LED6,
	LEON3_LED7,
	
	// Leon-3 GPIOs remaining
	LEON3_GPIO2,
	LEON3_GPIO3,
	LEON3_GPIO4,
	LEON3_GPIO5,
	LEON3_GPIO6,
	LEON3_GPIO7,
	
	//----------------------------------------------
	// Payload connections
	//---------------------------------------------
	LEON3_PAYLOAD_POWER_ENABLE,
	LEON3_PAYLOAD_RXD,
	LEON3_PAYLOAD_TXD,
	
	//----------------------------------------------
	// Console Serial -- either Leon-3 or MSP430 depending on Jumper setting
	//----------------------------------------------
	EXTERNAL_CONSOLE_RXD,
	EXTERNAL_CONSOLE_TXD,
	
	//----------------------------------------------
	// Umbilical Serial (goes through PL2303SA) directly to Leon-3 (jumper enables)
	//----------------------------------------------
	LEON3_UMBILICAL_RXD,
	LEON3_UMBILICAL_TXD,
	
	//----------------------------------------------
	//Experiment Radio pins on Eyassat 40 pin connector
	//----------------------------------------------
	LEON3_EXP_RADIO_TXD,
	LEON3_EXP_RADIO_RXD,
	
	//----------------------------------------------
	// EyasSat Serial
	//----------------------------------------------
	LEON3_EYASSAT_RXD,
	LEON3_EYASSAT_TXD,
	
	//----------------------------------------------
	// Debug Serial -- either Leon-3 or MSP430 depending on Jumper setting
	//----------------------------------------------
	EXTERNAL_DEBUG_RXD,
	EXTERNAL_DEBUG_TXD,

    //-----------------------------------------------
	// Leon-3 Debug JTAG
	//-----------------------------------------------
	//LEON3_DEBUG_JTAG_TCK,
	//LEON3_DEBUG_JTAG_TMS,
	//LEON3_DEBUG_JTAG_TDI,
	//LEON3_DEBUG_JTAG_TDO,
	
	//-----------------------------------------------
	// Raspberry Pi-Zero SPI0
	//-----------------------------------------------
	PAYLOAD_SPI0_SI,       // Slave in
	PAYLOAD_SPI0_SO,       // Slave out
	PAYLOAD_SPI0_SCK,      // Slave SCK
	PAYLOAD_SPI0_SSEL,     // Slave Select

	//-----------------------------------------------
	// Raspberry Pi-Zero I2C
	//-----------------------------------------------
	PAYLOAD_I2C_SCL,       // I2C Clock
	PAYLOAD_I2C_SDA,       // I2C Serial Data
	
	//----------------------------------------------
	// Input
	//----------------------------------------------
	UMBILICAL_SELECT_JUMPER,       // Select whether cmd/tlm goes to MSP430 or goes to Umbilical header
	FLASH_SELECT_JUMPER,           // Select which SPI flash chip to use for Leon-3
	EYASSAT_BUS_BYPASS_JUMPER,     // Select whether to bypass the EyasSat BUs through the MSP430
	DEBUG_UART_SELECT_JUMPER,      // Select between MSP430 and Leon-3 Debug UART
	CONSOLE_UART_SELECT_JUMPER,    // Select between MSP430 and Leon-3 Console UART
	
	//----------------------------------------------
    // Memory
    // DDR3
    //----------------------------------------------
    ddr3_dq,
    ddr3_dqs_p,
    ddr3_dqs_n,
    ddr3_addr,
    ddr3_ba,
    ddr3_ras_n,
    ddr3_cas_n,
    ddr3_we_n,
    ddr3_reset_n,
    ddr3_ck_p,
    ddr3_ck_n,
    ddr3_cke,
    ddr3_dm,
    ddr3_odt,

	//----------------------------------------------
	// 48 MHz oscillator input 
	//----------------------------------------------
	USER_CLOCK
);

//----------------------------------------------
// M25PE40
// SPI Configuration FLash
//----------------------------------------------
output MASTER_CONFIG_SCK;
output MASTER_CONFIG_CSN;
input MASTER_CONFIG_MISO;
output MASTER_CONFIG_MOSI;

//----------------------------------------------
// Input clock from external 20 MHz oscillator 
//----------------------------------------------
input    USER_CLOCK;

//----------------------------------------------
// Radio
//   SX1272
//----------------------------------------------
output RADIO_SCK;
output RADIO_NSS;
input RADIO_MISO;
output RADIO_MOSI;

inout RADIO_DIO0;
inout RADIO_DIO1;
inout RADIO_DIO2;
inout RADIO_DIO3;
inout RADIO_DIO4;
inout RADIO_DIO5;

output RADIO_RESET;

//----------------------------------------------
// Battery Indicators
//----------------------------------------------
output BATTERY_GREEN_LED;
output BATTERY_YELLOW_LED;

//----------------------------------------------
// LED Indicators
//----------------------------------------------
output STATUS_LED_MSP430;
output STATUS_LED_RADIOLINK;
output STATUS_LED_LEON_HEARTBEAT;
output STATUS_LED_EXTRA;

output LEON3_LED4;
output LEON3_LED5;
output LEON3_LED6;
output LEON3_LED7;

// Leon-3 GPIOs
inout LEON3_GPIO2;
inout LEON3_GPIO3;
inout LEON3_GPIO4;
inout LEON3_GPIO5;
inout LEON3_GPIO6;
inout LEON3_GPIO7;

//----------------------------------------------
// Payload connections
//---------------------------------------------
output LEON3_PAYLOAD_POWER_ENABLE;
input LEON3_PAYLOAD_RXD;
output LEON3_PAYLOAD_TXD;
	
//----------------------------------------------
// Console Serial
//----------------------------------------------
input EXTERNAL_CONSOLE_RXD;
output EXTERNAL_CONSOLE_TXD;

//----------------------------------------------
// Umbilical Serial (goes through PL2303SA) directly to Leon-3 (jumper enables) or to MSP430 radio
//----------------------------------------------
input LEON3_UMBILICAL_RXD;
output LEON3_UMBILICAL_TXD;

//----------------------------------------------
// Experiment Radio pins on Eyassat 40 pin connector
//----------------------------------------------
input LEON3_EXP_RADIO_RXD;
output LEON3_EXP_RADIO_TXD;

// ---------------------------------------------
// Eyassat Serial goes directly to Leon-3
//----------------------------------------------
input 	LEON3_EYASSAT_RXD;
output  LEON3_EYASSAT_TXD;	

//----------------------------------------------
// Debug Serial (goes through PL2303SA)
//----------------------------------------------
input EXTERNAL_DEBUG_RXD;
output EXTERNAL_DEBUG_TXD;

//-----------------------------------------------
// Leon-3 Debug JTAG
//-----------------------------------------------
//input LEON3_DEBUG_JTAG_TCK;
//input LEON3_DEBUG_JTAG_TMS;
//input LEON3_DEBUG_JTAG_TDI;
//output LEON3_DEBUG_JTAG_TDO;

//-----------------------------------------------
// Raspberry Pi-Zero SPI0
//-----------------------------------------------
input PAYLOAD_SPI0_SI;       // Slave in
output PAYLOAD_SPI0_SO;      // Slave out
input PAYLOAD_SPI0_SCK;      // Slave SCK
input PAYLOAD_SPI0_SSEL;     // Slave Select

//-----------------------------------------------
// Raspberry Pi-Zero I2C
//-----------------------------------------------
inout PAYLOAD_I2C_SCL;       // I2C Clock
inout PAYLOAD_I2C_SDA;       // I2C Data

//----------------------------------------------
// Input
//----------------------------------------------
input UMBILICAL_SELECT_JUMPER;       // Select whether cmd/tlm goes to MSP430 or goes to Umbilical header
input FLASH_SELECT_JUMPER;           // Select which SPI flash chip to use for Leon-3
input EYASSAT_BUS_BYPASS_JUMPER;     // Select whether to bypass the EyasSat BUs through the MSP430
input DEBUG_UART_SELECT_JUMPER;      // Select between MSP430 and Leon-3 Debug UART
input CONSOLE_UART_SELECT_JUMPER;    // Select between MSP430 and Leon-3 Console UART

//----------------------------------------------
// DDR3 Memory
//----------------------------------------------
inout [15:0] ddr3_dq;
inout [1:0] ddr3_dqs_p;
inout [1:0] ddr3_dqs_n;
output [13:0] ddr3_addr;
output [2:0] ddr3_ba;
output ddr3_ras_n;
output ddr3_cas_n;
output ddr3_we_n;
output ddr3_reset_n;
output ddr3_ck_p;
output ddr3_ck_n;
output ddr3_cke;
output [1:0] ddr3_dm;
output ddr3_odt;



//=============================================================================
// 1)  INTERNAL WIRES/REGISTERS/PARAMETERS DECLARATION
//=============================================================================

// Clock generation
wire               clk_48mhz;
wire               dcm_locked;
wire               dcm_clkfx;
//wire               dcm_clk0;
wire               dcm_clkfb;
wire               dco_clk;

wire               dcm_clkfbout;
wire               dcm_clkfbin;

// Reset generation
//wire               master_reset_n;

// Debug interface

// Battery Status LEDs
wire				battery_yellow_led;
wire				battery_green_led;

// Leon-3 reset line
wire                leon3_reset_n;

// Output status LEDs
wire                omspradio_status_good;
wire                omspradio_status_link;
wire                leon3_status_heartbeat;
wire                omsp_status_extra;

// Data memory
// RADIO (32 KB)
wire [13:0] omspradio_dmem_addr;
wire               omspradio_dmem_cen;
wire        [15:0] omspradio_dmem_din;
wire         [1:0] omspradio_dmem_wen;
wire        [15:0] omspradio_dmem_dout;

// Program memory
// RADIO (32KB)
wire [13:0] omspradio_pmem_addr;
wire               omspradio_pmem_cen;
wire        [15:0] omspradio_pmem_din;
wire         [1:0] omspradio_pmem_wen;
wire        [15:0] omspradio_pmem_dout;



// Radio Interface
wire		omspradio_radio_mosi;
wire		omspradio_radio_miso;
wire		omspradio_radio_sck;
wire		omspradio_radio_csn;

wire		omspradio_radio_reset;
wire		omspradio_radio_dio0;
wire		omspradio_radio_dio1;
wire		omspradio_radio_dio2;
wire		omspradio_radio_dio3;
wire		omspradio_radio_dio4;
wire		omspradio_radio_dio5;

// Configuration Flash Interface
wire		omspradio_conflash_mosi;
wire		omspradio_conflash_miso;
wire		omspradio_conflash_clk;
wire		omspradio_conflash_csn;



// Radio Interface UART
wire        omspradio_radio_uart_txd;
wire        omspradio_radio_uart_rxd;

// Battery indicator
wire        omspradio_battery_status_low;

// Jumper inputs
wire		jumper_umbilical_select;
wire		jumper_flash_select;
wire		jumper_eyassat_bypass_select;
wire        jumper_debug_uart_select;
wire        jumper_console_uart_select;

// Jumper Inputs (Debounced)
wire		jumper_umbilical_select_debounced;
wire		jumper_flash_select_debounced;
wire		jumper_eyassat_bypass_select_debounced;
wire        jumper_debug_uart_select_debounced;
wire        jumper_console_uart_select_debounced;





// Debug Interface/Programming Interface Multiplexing
wire		omsp0_dbg_rxd;
wire		omsp0_dbg_txd;
wire		omsp0_dbg_en;

// Leon-3 LED outputs
wire [7:0]  leon3_leds;

// Leon-3 GPIOs
wire [13:0] leon3_gpio_dout;
wire [13:0] leon3_gpio_din;
wire [13:0] leon3_gpio_oen;



// Leon-3 Debug UART
wire        leon3_debug_uart_txd;
wire        leon3_debug_uart_rxd;

// Leon-3 Debug JTAG
//wire        leon3_debug_jtag_tck;
//wire        leon3_debug_jtag_tms;
//wire        leon3_debug_jtag_tdi;
//wire        leon3_debug_jtag_tdo;

// Payload SPI0 Controller
wire payload_spi0_si;      // Slave in
wire payload_spi0_so;      // Slave out
wire payload_spi0_sck;     // Slave SCK
wire payload_spi0_ssel;    // Slave Select

// Payload I2C Controller
wire payload_i2c0_scl_o;	// I2C Clock Output
wire payload_i2c0_scl_i;    // I2C Clock Input
wire payload_i2c0_scl_oen;  // I2C Clock Output Enable
wire payload_i2c0_sda_o;	// I2C Serial Data Output
wire payload_i2c0_sda_i;    // I2C Serial Data Input
wire payload_i2c0_sda_oen;  // I2C Serial Data Output Enable

// Leon-3 console UART
wire        leon3_console_txd;
wire        leon3_console_rxd;

// Leon-3 umbilical UART
wire        leon3_umbilical_rxd;
wire        leon3_umbilical_txd;

// Leon-3 Eyassat Exp Radio
wire        leon3_exp_radio_rxd;
wire        leon3_exp_radio_txd;

// Leon-3 Eyassat UART
wire        leon3_eyassat_rxd;
wire        leon3_eyassat_txd;

// The radio interface for the Leon-3 (goes to Umbilical or MSP430 radio interface)
wire        leon3_radio_rxd;
wire        leon3_radio_txd;

// Payload interface for Leon-3 (UART)
wire        leon3_payload_rxd;
wire        leon3_payload_txd;


// SPI Flash Interface
wire        leon3_spi_cs;          // out   std_ulogic;
wire        leon3_spi_miso;        // inout
wire        leon3_spi_mosi;        // inout
wire        leon3_spi_sck;             // out   std_ulogic

// Config flash wire
wire        master_config_csn;
wire        master_config_sck;
wire        master_config_miso;
wire        master_config_mosi;

// Programming Serial Interface
wire		omspradio_console_txd;
wire        omspradio_console_rxd;


// External Console UART
wire        external_console_uart_txd;
wire        external_console_uart_rxd;



// External Debug UART
wire        external_debug_uart_txd;
wire        external_debug_uart_rxd;

// Internal routing of Payload UART
wire        payload_uart_internal_txd;
wire        payload_uart_internal_rxd;

//=============================================================================
// 2)  RESET GENERATION & FPGA STARTUP
//=============================================================================

// Reset input buffer
//IBUF   ibuf_reset_n   (.O(reset_pin), .I(USER_RESET));
//assign reset_pin_n = ~reset_pin;

// Release the reset only, if the DCM is locked
//assign  reset_n = reset_pin_n & dcm_locked;


//=============================================================================
// 3)  CLOCK GENERATION
//=============================================================================

// Input buffers
//------------------------
IBUFG ibuf_clk_main   (.O(clk_48mhz),    .I(USER_CLOCK));


// Digital Clock Manager
//------------------------
/*
DCM_SP #(.CLKFX_MULTIPLY(2),
	 .CLKFX_DIVIDE(4),
	 .CLKIN_PERIOD(20.8333)) dcm_inst (

// OUTPUTs
    .CLKFX        (dcm_clkfx),
    .CLK0         (dcm_clk0),
    .LOCKED       (dcm_locked),

// INPUTs
    .CLKFB        (dcm_clkfb),
    .CLKIN        (clk_48mhz),
    .PSEN         (1'b0),
    .RST          (1'b0)
);

BUFG CLK0_BUFG_INST (
    .I(dcm_clk0),
    .O(dcm_clkfb)
);
*/

/*
MMCME2_BASE #( .CLKOUT0_DIVIDE_F(36),
               .CLKFBOUT_MULT_F(18),        // Use 15 for 20MHz clock, and 18 for 24MHz clock
               .CLKIN1_PERIOD(20.8333)) mmcm (
               .CLKOUT0(dcm_clk0),
               .CLKFBOUT(dcm_clkfbout),
               .LOCKED(dcm_locked),
               .CLKIN1(clk_48mhz),
               .PWRDWN(1'b0),
               .RST(1'b0),
               .CLKFBIN(dcm_clkfbin));
 
 BUFG   bufclkfb    (.I(dcm_clkfbout),  .O(dcm_clkfbin));         
 
 // Reset input buffer
 assign master_reset_n = dcm_locked;     
*/

//synthesis translate_off
// 16 MHz clock
defparam dcm_inst.CLKFX_MULTIPLY  = 1;
defparam dcm_inst.CLKFX_DIVIDE    = 2;
defparam dcm_inst.CLKIN_PERIOD    = 20.83;
//synthesis translate_on


reg dcm_clk0 = 1'b0;
always @(posedge clk_48mhz)
begin
  dcm_clk0 <=  ~dcm_clk0;
end

reg master_reset_n = 1'b1;



// Clock buffers
//------------------------
//BUFG  buf_sys_clock  (.O(dco_clk), .I(dcm_clkfx));
BUFG  buf_sys_clock  (.O(dco_clk), .I(dcm_clk0));


//=============================================================================
//
// DEBOUNCE HARDWARE I/O
//
//=============================================================================
DeBounce debounce_umbilical_select (
	.clk			(dco_clk), 
	.n_reset		(master_reset_n), 
	.button_in	(jumper_umbilical_select),
	.DB_out		(jumper_umbilical_select_debounced)
);

DeBounce debounce_flash_select (
	.clk			(dco_clk), 
	.n_reset		(master_reset_n), 
	.button_in	(jumper_flash_select),
	.DB_out		(jumper_flash_select_debounced)
);

DeBounce debounce_eyassat_bypass_select (
	.clk			(dco_clk), 
	.n_reset		(master_reset_n), 
	.button_in	(jumper_eyassat_bypass_select),
	.DB_out		(jumper_eyassat_bypass_select_debounced)
);

DeBounce debounce_debug_uart_select (
	.clk			(dco_clk), 
	.n_reset		(master_reset_n), 
	.button_in	(jumper_debug_uart_select),
	.DB_out		(jumper_debug_uart_select_debounced)
);

DeBounce debounce_console_uart_select (
	.clk			(dco_clk), 
	.n_reset		(master_reset_n), 
	.button_in	(jumper_console_uart_select),
	.DB_out		(jumper_console_uart_select_debounced)
);




//=============================================================================
// 5)  OPENMSP430 SYSTEM 1
// RADIO
//=============================================================================

omsp_system_1 omsp_system_radio_inst (

// Clock & Reset
    .dco_clk           (dco_clk),                     // Fast oscillator (fast clock)
    .reset_n           (master_reset_n),                     // Reset Pin (low active, asynchronous and non-glitchy)

// Serial Debug Interface (I2C)
    .dbg_uart_rxd      (omsp0_dbg_rxd),
	.dbg_uart_txd	   (omsp0_dbg_txd),
	.dbg_en		       (omsp0_dbg_en),

// Data Memory
    .dmem_addr         (omspradio_dmem_addr),             // Data Memory address
    .dmem_cen          (omspradio_dmem_cen),              // Data Memory chip enable (low active)
    .dmem_din          (omspradio_dmem_din),              // Data Memory data input
    .dmem_wen          (omspradio_dmem_wen),              // Data Memory write enable (low active)
    .dmem_dout         (omspradio_dmem_dout),             // Data Memory data output

// Program Memory
    .pmem_addr         (omspradio_pmem_addr),             // Program Memory address
    .pmem_cen          (omspradio_pmem_cen),              // Program Memory chip enable (low active)
    .pmem_din          (omspradio_pmem_din),              // Program Memory data input (optional)
    .pmem_wen          (omspradio_pmem_wen),              // Program Memory write enable (low active) (optional)
    .pmem_dout         (omspradio_pmem_dout),             // Program Memory data output

// SPI RADIO
	 .spi_radio_mosi		(omspradio_radio_mosi),				// SPI Radio Master OUT Slave IN
	 .spi_radio_miso		(omspradio_radio_miso),				// SPI Radio Master IN Slave OUT
	 .spi_radio_clk		(omspradio_radio_clk),				// SPI Radio CLOCK
	 .spi_radio_csn		(omspradio_radio_csn),				// SPI Radio CSN
	 
	 .spi_radio_reset		(omspradio_radio_reset),
	 .spi_radio_dio0		(omspradio_radio_dio0),
	 .spi_radio_dio1		(omspradio_radio_dio1),
	 .spi_radio_dio2		(omspradio_radio_dio2),
	 .spi_radio_dio3		(omspradio_radio_dio3),
	 .spi_radio_dio4		(omspradio_radio_dio4),
	 .spi_radio_dio5		(omspradio_radio_dio5),
	 
// SPI FLASH
    .spi_conflash_mosi	(omspradio_conflash_mosi),			// SPI Master OUT Slave IN for the configuration flash
	 .spi_conflash_miso	(omspradio_conflash_miso),			// SPI Master IN Slave OUT for the configuration flash
	 .spi_conflash_clk	(omspradio_conflash_sck),			// SPI CLK for the configuration flash
	 .spi_conflash_csn	(omspradio_conflash_csn),			// SPI Configuration Flash CSN
	 
// UART Program
	.uart_program_rxd		(omspradio_console_rxd),			// UART RXD
	.uart_program_txd		(omspradio_console_txd),			// UART TXD
	
// UART App Processor
	.uart_app_rxd			(omspradio_radio_uart_rxd),				// UART RXD (connected to Leon-3 processor TXD)
	.uart_app_txd			(omspradio_radio_uart_txd),				// UART TXD (connected to Leon-3 processor RXD)

// Input
	.jumper_umbilical_select			(jumper_umbilical_select_debounced),
	.jumper_flash_select				(jumper_flash_select_debounced),
	.jumper_eyassat_bypass_select		(jumper_eyassat_bypass_select_debounced),
	
// Outputs
    .leon3_reset_n          (leon3_reset_n),
    .battery_status_low     (omspradio_battery_status_low),
    .status_output          (omspradio_status_good),
    .status_radio_link      (omspradio_status_link),
    .status_extra           (omspradio_status_extra)
     

);

//=============================================================================
// Leon3 System
//=============================================================================
leon3mp leon3_system_1 (
    .sysclk             (clk_48mhz),        // Input 48 MHz clock
    
    // LEDs
    .led                (leon3_leds),
  
    // Reset signal
    .cpu_resetn         (leon3_reset_n),
    
    // GPIO
    .gpio_dout          (leon3_gpio_dout),
    .gpio_din           (leon3_gpio_din),
    .gpio_oen           (leon3_gpio_oen),
    
    // Switches
    //sw                 : in    std_logic_vector(7 downto 0);    
    
    // Debug UART
    .dbg_uart_txd       (leon3_debug_uart_txd),
    .dbg_uart_rxd       (leon3_debug_uart_rxd),
    
    // Debug JTAG
    //.dbg_jtag_tck       (leon3_debug_jtag_tck),
    //.dbg_jtag_tms       (leon3_debug_jtag_tms),
    //.dbg_jtag_tdi       (leon3_debug_jtag_tdi),
    //.dbg_jtag_tdo       (leon3_debug_jtag_tdo),
    
    // CMD/TLM Interface -- Could be umbilical or radio depending on switch settings
    .radio_txd          (leon3_radio_txd),  //(omspradio_program_rxd),
    .radio_rxd          (leon3_radio_rxd),  //(omspradio_program_txd),
    
    // RTEMs console
    .console_txd        (leon3_console_txd),
    .console_rxd        (leon3_console_rxd),
    
    // EyasSat C&DH Interface
    .eyassat_txd        (leon3_eyassat_txd),
    .eyassat_rxd        (leon3_eyassat_rxd),
    
    // Payload Interface
    .payload_txd        (leon3_payload_txd),
    .payload_rxd        (leon3_payload_rxd),
    
    // DDR3
    .ddr3_dq           (ddr3_dq),
    .ddr3_dqs_p        (ddr3_dqs_p),
    .ddr3_dqs_n        (ddr3_dqs_n),
    .ddr3_addr         (ddr3_addr),
    .ddr3_ba           (ddr3_ba),
    .ddr3_ras_n        (ddr3_ras_n),
    .ddr3_cas_n        (ddr3_cas_n),
    .ddr3_we_n         (ddr3_we_n),
    .ddr3_reset_n      (ddr3_reset_n),
    .ddr3_ck_p         (ddr3_ck_p),
    .ddr3_ck_n         (ddr3_ck_n),
    .ddr3_cke          (ddr3_cke),
    .ddr3_dm           (ddr3_dm),
    .ddr3_odt          (ddr3_odt),
    
    // ROM SPI
    .spi_cs            (leon3_spi_cs),
    .spi_miso          (leon3_spi_miso),
    .spi_mosi          (leon3_spi_mosi),    
    .spi_sck           (leon3_spi_sck),
    
    // Payload SPI
    .spi0_mosi         (payload_spi0_si),
    .spi0_miso         (payload_spi0_so),
    .spi0_sck          (payload_spi0_sck),
    .spi0_sel          (payload_spi0_ssel),

	// Payload I2C
	.i2c0_scl_o         (payload_i2c0_scl_o),
	.i2c0_scl_i         (payload_i2c0_scl_i),
	.i2c0_scl_oen       (payload_i2c0_scl_oen),
	.i2c0_sda_o         (payload_i2c0_sda_o),
	.i2c0_sda_i         (payload_i2c0_sda_i),
	.i2c0_sda_oen       (payload_i2c0_sda_oen)
);

//=============================================================================
// 6)  PROGRAM AND DATA MEMORIES
//=============================================================================


// Data Memory (CPU RADIO PROCESSOR) (32KB)
ram_16x16k_sp_dmem ram_dmem_omsp_radio (
    .clka           ( dco_clk),
    .ena            (~omspradio_dmem_cen),
    .wea            (~omspradio_dmem_wen),
    .addra          ( omspradio_dmem_addr),
    .dina           ( omspradio_dmem_din),
    .douta          ( omspradio_dmem_dout)
);





// Program memory (CPU RADIO PROCESSOR) (32KB)
ram_16x16k_sp_pmem ram_pmem_omsp_radio (
    .clka           ( dco_clk),
    .ena            (~omspradio_pmem_cen),
    .wea            (~omspradio_pmem_wen),
    .addra          ( omspradio_pmem_addr),
    .dina           ( omspradio_pmem_din),
    .douta          ( omspradio_pmem_dout)
);


//=============================================================================
// 7)  I/O CELLS
//=============================================================================

// MSP430 CONSOLE Serial Interface - - NO LONGER USED -- NOW SWITCHED WITH MSP430 Console UART
//OBUF    MSP430_CONSOLE_TXD_PIN			(.I(omspradio_console_txd),			.O(MSP430_CONSOLE_TXD));
//IBUF    MSP430_CONSOLE_RXD_PIN			(.O(omspradio_console_rxd),			.I(MSP430_CONSOLE_RXD));

//--------------------------
// RADIO
//--------------------------
// Configuration Flash SPI
OBUF    CONFIG_MOSI_PIN			(.I(master_config_mosi),		.O(MASTER_CONFIG_MOSI));
IBUF	CONFIG_MISO_PIN			(.O(master_config_miso),		.I(MASTER_CONFIG_MISO));
OBUF	CONFIG_SCK_PIN			(.I(master_config_sck),		.O(MASTER_CONFIG_SCK));
OBUF	CONFIG_CSN_PIN			(.I(master_config_csn),		.O(MASTER_CONFIG_CSN));

// Radio Interface SPI
OBUF	RADIO_MOSI_PIN				(.I(omspradio_radio_mosi),			.O(RADIO_MOSI));
IBUF	RADIO_MISO_PIN				(.O(omspradio_radio_miso),			.I(RADIO_MISO));
OBUF	RADIO_NSS_PIN				(.I(omspradio_radio_csn),			.O(RADIO_NSS));
OBUF	RADIO_SCK_PIN				(.I(omspradio_radio_clk),			.O(RADIO_SCK));

OBUF	RADIO_RESET_PIN			(.I(omspradio_radio_reset),		.O(RADIO_RESET));
IBUF	RADIO_DIO0_PIN				(.O(omspradio_radio_dio0),			.I(RADIO_DIO0));
IBUF	RADIO_DIO1_PIN				(.O(omspradio_radio_dio1),			.I(RADIO_DIO1));
IBUF	RADIO_DIO2_PIN				(.O(omspradio_radio_dio2),			.I(RADIO_DIO2));
IBUF	RADIO_DIO3_PIN				(.O(omspradio_radio_dio3),			.I(RADIO_DIO3));
IBUF	RADIO_DIO4_PIN				(.O(omspradio_radio_dio4),			.I(RADIO_DIO4));
IBUF	RADIO_DIO5_PIN				(.O(omspradio_radio_dio5),			.I(RADIO_DIO5));

// Status LEDs
OBUF    STATUS_LED_MSP430_PIN           (.I(omspradio_status_good),         .O(STATUS_LED_MSP430));
OBUF    STATUS_LED_RADIOLINK_PIN        (.I(omspradio_status_link),         .O(STATUS_LED_RADIOLINK));
OBUF    STATUS_LED_LEON_HEARTBEAT_PIN   (.I(leon3_gpio_dout[2]),            .O(STATUS_LED_LEON_HEARTBEAT));
OBUF    STATUS_LED_EXTRA_PIN            (.I(omspradio_status_extra),        .O(STATUS_LED_EXTRA));

// Jumper Pins
IBUF	UMBILICAL_SELECT_JUMPER_PIN		(.O(jumper_umbilical_select),		  .I(UMBILICAL_SELECT_JUMPER));
IBUF	FLASH_SELECT_JUMPER_PIN	        (.O(jumper_flash_select),			  .I(FLASH_SELECT_JUMPER));
IBUF	EYASSAT_BUS_BYPASS_JUMPER_PIN	(.O(jumper_eyassat_bypass_select),	  .I(EYASSAT_BUS_BYPASS_JUMPER));
IBUF    DEBUG_UART_SELECT_JUMPER_PIN    (.O(jumper_debug_uart_select),        .I(DEBUG_UART_SELECT_JUMPER));
IBUF    CONSOLE_UART_SELECT_JUMPER_PIN  (.O(jumper_console_uart_select),      .I(CONSOLE_UART_SELECT_JUMPER));


// Battery Detection I/Os
OBUF	BATTERY_GREEN_LED_PIN	(.I(battery_green_led),				.O(BATTERY_GREEN_LED));
OBUF	BATTERY_YELLOW_LED_PIN	(.I(battery_yellow_led),			.O(BATTERY_YELLOW_LED));

// Leon-3 GPIOs
IOBUF   LEON3_GPIO3_PIN         (.I(leon3_gpio_din[3]), .O(leon3_gpio_dout[3]), .T(leon3_gpio_oen[3]), .IO(LEON3_GPIO3));
IOBUF   LEON3_GPIO4_PIN         (.I(leon3_gpio_din[4]), .O(leon3_gpio_dout[4]), .T(leon3_gpio_oen[4]), .IO(LEON3_GPIO4));
IOBUF   LEON3_GPIO5_PIN         (.I(leon3_gpio_din[5]), .O(leon3_gpio_dout[5]), .T(leon3_gpio_oen[5]), .IO(LEON3_GPIO5));
IOBUF   LEON3_GPIO6_PIN         (.I(leon3_gpio_din[6]), .O(leon3_gpio_dout[6]), .T(leon3_gpio_oen[6]), .IO(LEON3_GPIO6));
IOBUF   LEON3_GPIO7_PIN         (.I(leon3_gpio_din[7]), .O(leon3_gpio_dout[7]), .T(leon3_gpio_oen[7]), .IO(LEON3_GPIO7));


// External Debug UART
OBUF	EXTERNAL_DEBUG_TXD_PIN		(.I(external_debug_uart_txd),			.O(EXTERNAL_DEBUG_TXD));
IBUF	EXTERNAL_DEBUG_RXD_PIN		(.O(external_debug_uart_rxd),			.I(EXTERNAL_DEBUG_RXD));


// External Console UART
OBUF	EXTERNAL_CONSOLE_TXD_PIN		(.I(external_console_uart_txd),			.O(EXTERNAL_CONSOLE_TXD));
IBUF	EXTERNAL_CONSOLE_RXD_PIN		(.O(external_console_uart_rxd),			.I(EXTERNAL_CONSOLE_RXD));

// Debug Connections - NO LONGER USED -- NOW SWITCHED WITH MSP430 Debug UART
//OBUF	MSP430_DEBUG_TXD_PIN		(.I(omsp0_dbg_txd),			.O(MSP430_DEBUG_TXD));
//IBUF	MSP430_DEBUG_RXD_PIN		(.O(omsp0_dbg_rxd),			.I(MSP430_DEBUG_RXD));

// Leon-3 Debug UART - - NO LONGER USED -- NOW SWITCHED WITH MSP430 Console UART
//OBUF    LEON3_DEBUG_TXD_PIN         (.I(leon3_debug_uart_txd),  .O(LEON3_DEBUG_TXD));
//IBUF    LEON3_DEBUG_RXD_PIN         (.O(leon3_debug_uart_rxd),  .I(LEON3_DEBUG_RXD));

// Leon-3 Debug JTAG (No longer used)
//IBUF    LEON3_DEBUG_JTAG_TCK_PIN    (.O(leon3_debug_jtag_tck),  .I(LEON3_DEBUG_JTAG_TCK));
//IBUF    LEON3_DEBUG_JTAG_TMS_PIN    (.O(leon3_debug_jtag_tms),  .I(LEON3_DEBUG_JTAG_TMS));
//IBUF    LEON3_DEBUG_JTAG_TDI_PIN    (.O(leon3_debug_jtag_tdi),  .I(LEON3_DEBUG_JTAG_TDI));
//OBUF    LEON3_DEBUG_JTAG_TDO_PIN    (.I(leon3_debug_jtag_tdo),  .O(LEON3_DEBUG_JTAG_TDO));

// Leon-3 Console UART -- NO LONGER USED -- NOW SWITCHED WITH MSP430 Console UART
//OBUF    LEON3_CONSOLE_TXD_PIN       (.I(leon3_console_txd),     .O(LEON3_CONSOLE_TXD));
//IBUF    LEON3_CONSOLE_RXD_PIN       (.O(leon3_console_rxd),     .I(LEON3_CONSOLE_RXD));

// Leon-3 Eyassat UART
OBUF    LEON3_EYASSAT_TXD_PIN       (.I(leon3_eyassat_txd),     .O(LEON3_EYASSAT_TXD));
IBUF    LEON3_EYASSAT_RXD_PIN       (.O(leon3_eyassat_rxd),     .I(LEON3_EYASSAT_RXD));

// Leon-3 Eyassat Exp Radio Pins
OBUF    LEON3_EXP_RADIO_TXD_PIN       (.I(leon3_exp_radio_txd),     .O(LEON3_EXP_RADIO_TXD));
IBUF    LEON3_EXP_RADIO_RXD_PIN       (.O(leon3_exp_radio_rxd),     .I(LEON3_EXP_RADIO_RXD));

// Leon-3 Umbilical (jumper enables) otherwise -- off
OBUF    LEON3_UMBILICAL_TXD_PIN     (.I(leon3_umbilical_txd),   .O(LEON3_UMBILICAL_TXD));
IBUF    LEON3_UMBILICAL_RXD_PIN     (.O(leon3_umbilical_rxd),   .I(LEON3_UMBILICAL_RXD));

// Leon-3 Payload UART
OBUF    LEON3_PAYLOAD_TXD_PIN       (.I(payload_uart_internal_txd),     .O(LEON3_PAYLOAD_TXD));
IBUF    LEON3_PAYLOAD_RXD_PIN       (.O(payload_uart_internal_rxd),     .I(LEON3_PAYLOAD_RXD));

OBUF    LEON3_PAYLOAD_POWER_ENABLE_PIN  (.I(leon3_gpio_dout[0]),    .O(LEON3_PAYLOAD_POWER_ENABLE));       

OBUF    LEON3_LED4_PIN              (.I(leon3_leds[4]),         .O(LEON3_LED4));
OBUF    LEON3_LED5_PIN              (.I(leon3_leds[5]),         .O(LEON3_LED5));
OBUF    LEON3_LED6_PIN              (.I(leon3_leds[6]),         .O(LEON3_LED6));
OBUF    LEON3_LED7_PIN              (.I(leon3_leds[7]),         .O(LEON3_LED7));

// Leon-3 -> Payload SPI Controller
IBUF    PAYLOAD_SPI0_SI_PIN         (.O(payload_spi_si),        .I(PAYLOAD_SPI0_SI));
OBUF    PAYLOAD_SPI0_SO_PIN         (.I(payload_spi_so),        .O(PAYLOAD_SPI0_SO));
IBUF    PAYLOAD_SPI0_SCK_PIN        (.O(payload_spi_sck),       .I(PAYLOAD_SPI_SCK));
IBUF    PAYLOAD_SPI0_SSEL_PIN       (.O(payload_spi_ssel),      .I(PAYLOAD_SPI_SSEL));

// Leon-3 -> Payload I2C Controller
IOBUF   PAYLOAD_I2C_SCL_PIN     (.O(payload_i2c0_scl_i), .I(payload_i2c0_scl_o), .T(payload_i2c0_scl_oen), .IO(PAYLOAD_I2C_SCL));
IOBUF   PAYLOAD_I2C_SDA_PIN     (.O(payload_i2c0_sda_i), .I(payload_i2c0_sda_o), .T(payload_i2c0_sda_oen), .IO(PAYLOAD_I2C_SDA));

// MUX Payload Console UART to Leon-3 (if enabled)
assign payload_uart_internal_txd = (leon3_gpio_dout[1] == 1) ? leon3_payload_txd : 1'b1;
assign leon3_payload_rxd = (leon3_gpio_dout[1] == 1) ? payload_uart_internal_rxd : 1'b0;

// Console UART MUX
assign external_console_uart_txd = (jumper_console_uart_select_debounced == 1) ? leon3_console_txd : omspradio_console_txd;

assign omspradio_console_rxd = (jumper_console_uart_select_debounced == 1) ? 1'b0 : external_console_uart_rxd;
assign leon3_console_rxd = (jumper_console_uart_select_debounced == 1) ? external_console_uart_rxd : 1'b0;

// Debug UART MUX
assign external_debug_uart_txd = (jumper_debug_uart_select_debounced == 1) ? leon3_debug_uart_txd : omsp0_dbg_txd;

assign omsp0_dbg_rxd = (jumper_debug_uart_select_debounced == 1) ? 1'b0 : external_debug_uart_rxd;
assign leon3_debug_uart_rxd = (jumper_debug_uart_select_debounced == 1) ? external_debug_uart_rxd : 1'b0;

assign omsp0_dbg_en = (jumper_debug_uart_select_debounced == 1) ? 1'b0 : 1'b1;

// Assign battery status
assign battery_yellow_led = (omspradio_battery_status_low == 1) ? 1'b1 : 1'b0;
assign battery_green_led  = (omspradio_battery_status_low == 0) ? 1'b1 : 1'b0;

// Assign radio connections (jumper select between radio and umbilical)
assign leon3_umbilical_txd = (jumper_umbilical_select_debounced == 1) ? leon3_radio_txd : 1'b1;
assign leon3_radio_rxd = (jumper_umbilical_select_debounced == 1) ? leon3_umbilical_rxd : omspradio_radio_uart_txd;
assign omspradio_radio_uart_rxd = (jumper_umbilical_select_debounced == 0) ? leon3_radio_txd : 1'b0;


// OLD: Used for Xbee EXP radio
/*
assign leon3_umbilical_txd = (jumper_umbilical_select_debounced == 1) ? leon3_radio_txd : 1'b1;
assign leon3_radio_rxd = (jumper_umbilical_select_debounced == 1) ? leon3_umbilical_rxd : leon3_exp_radio_rxd;
assign leon3_exp_radio_txd = (jumper_umbilical_select_debounced == 0) ? leon3_radio_txd : 1'b0;
*/
//assign omspradio_radio_uart_rxd = omspradio_radio_uart_txd;

//assign omspradio_radio_uart_rxd = leon3_radio_txd;
//assign leon3_radio_rxd = omspradio_radio_uart_txd;

// Assign who is the master of the config flash (MSP430 is always first)
assign master_config_sck = (omspradio_conflash_csn == 0) ? omspradio_conflash_sck : leon3_spi_sck;
assign master_config_csn = (omspradio_conflash_csn == 0) ? omspradio_conflash_csn : leon3_spi_cs;
assign master_config_mosi = (omspradio_conflash_csn == 0) ? omspradio_conflash_mosi : leon3_spi_mosi;
assign omspradio_conflash_miso = (omspradio_conflash_csn == 0) ? master_config_miso : 1'b0;
assign leon3_spi_miso = (omspradio_conflash_csn == 1) ? master_config_miso : 1'b0;


//=============================================================================
//8)  CHIPSCOPE
//=============================================================================
//`define WITH_CHIPSCOPE
`ifdef WITH_CHIPSCOPE

// Sampling clock
reg [7:0] div_cnt;
always @ (posedge dco_clk or posedge dco_rst)
  if (dco_rst)           div_cnt <=  8'h00;
  else if (div_cnt > 10) div_cnt <=  8'h00;
  else                   div_cnt <=  div_cnt+8'h01;

reg clk_sample;
always @ (posedge dco_clk or posedge dco_rst)
  if (dco_rst) clk_sample <=  1'b0;
  else         clk_sample <=  (div_cnt==8'h00);

   
// ChipScope instance
wire        [35:0] chipscope_control;
chipscope_ila chipscope_ila (
    .CONTROL  (chipscope_control),
    .CLK      (clk_sample),
    .TRIG0    (chipscope_trigger)
);

chipscope_icon chipscope_icon (
    .CONTROL0 (chipscope_control)
);


assign chipscope_trigger[0]     = 1'b0;
assign chipscope_trigger[1]     = 1'b0;
assign chipscope_trigger[2]     = 1'b0;
assign chipscope_trigger[23:3]  = 21'h00_0000;
`endif

endmodule // openMSP430_fpga

