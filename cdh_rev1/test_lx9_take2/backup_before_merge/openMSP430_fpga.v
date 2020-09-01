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
//                      openMSP430 FPGA Top-level for Defcon 22 CTF Badge
//
// *Author(s):
//					 - sirgoon,            sirgoon@legitbs.net
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
	// Console Serial (goes through PL2303SA) directly to MSP430
	//----------------------------------------------
	MSP430_CONSOLE_RXD,
	MSP430_CONSOLE_TXD,
	
	//----------------------------------------------
	// Console Serial (goes through PL2303SA) directly to Leon-3
	//----------------------------------------------
	LEON3_CONSOLE_RXD,
	LEON3_CONSOLE_TXD,
	
	//----------------------------------------------
	// Umbilical Serial (goes through PL2303SA) directly to Leon-3 (jumper enables)
	//----------------------------------------------
	LEON3_UMBILICAL_RXD,
	LEON3_UMBILICAL_TXD,
	
	//----------------------------------------------
	// EyasSat Serial
	//----------------------------------------------
	LEON3_EYASSAT_RXD,
	LEON3_EYASSAT_TXD,
	
	//----------------------------------------------
	// Debug Serial (goes through PL2303SA)
	// Has multiple modes of operation.
	//   Run - Device runs standard (RXD/TXD is unconnected)
	//   Prog - Allows programming of the application processor through the Radio processor
	//   Debug - Connects to the serial debug module of the application processor
	// don't let them run the radio while in PROG or DBG mode -- we are eeeeeevil
	//----------------------------------------------
	MSP430_DEBUG_RXD,
	MSP430_DEBUG_TXD,

	//----------------------------------------------
	// Input
	//----------------------------------------------
	UMBILICAL_SELECT_JUMPER,       // Select whether cmd/tlm goes to MSP430 or goes to Umbilical header
	FLASH_SELECT_JUMPER,           // Select which SPI flash chip to use for Leon-3
	EYASSAT_BUS_BYPASS_JUMPER,     // Select whether to bypass the EyasSat BUs through the MSP430
	
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
// Console Serial (goes through PL2303SA) directly to MSP430
//----------------------------------------------
input MSP430_CONSOLE_RXD;
output MSP430_CONSOLE_TXD;
	
//----------------------------------------------
// Console Serial (goes through PL2303SA) directly to Leon-3
//----------------------------------------------
input LEON3_CONSOLE_RXD;
output LEON3_CONSOLE_TXD;

//----------------------------------------------
// Umbilical Serial (goes through PL2303SA) directly to Leon-3 (jumper enables) or to MSP430 radio
//----------------------------------------------
input LEON3_UMBILICAL_RXD;
output LEON3_UMBILICAL_TXD;

// ---------------------------------------------
// Eyassat Serial goes directly to Leon-3
//----------------------------------------------
input 	LEON3_EYASSAT_RXD;
output  LEON3_EYASSAT_TXD;	

//----------------------------------------------
// Debug Serial (goes through PL2303SA)
//----------------------------------------------
input MSP430_DEBUG_RXD;
output MSP430_DEBUG_TXD;

//----------------------------------------------
// Input
//----------------------------------------------
input UMBILICAL_SELECT_JUMPER;       // Select whether cmd/tlm goes to MSP430 or goes to Umbilical header
input FLASH_SELECT_JUMPER;           // Select which SPI flash chip to use for Leon-3
input EYASSAT_BUS_BYPASS_JUMPER;     // Select whether to bypass the EyasSat BUs through the MSP430

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

/*
wire [15:0] ddr3_dq;          // inout std_logic_vector(15 downto 0);
wire [1:0]  ddr3_dqs_p;        // inout std_logic_vector(1 downto 0);
wire [1:0]  ddr3_dqs_n;        // inout std_logic_vector(1 downto 0);
wire [13:0] ddr3_addr;        // out   std_logic_vector(13 downto 0);
wire [2:0]  ddr3_ba;           // out   std_logic_vector(2 downto 0);
wire        ddr3_ras_n;        // out   std_logic;
wire        ddr3_cas_n;        // out   std_logic;
wire        ddr3_we_n;         // out   std_logic;
wire        ddr3_reset_n;       // out   std_logic;
wire        ddr3_ck_p;         // out   std_logic_vector(0 downto 0);
wire        ddr3_ck_n;         // out   std_logic_vector(0 downto 0);
wire        ddr3_cke;          // out   std_logic_vector(0 downto 0);
wire [1:0]  ddr3_dm;            // out   std_logic_vector(1 downto 0);
wire        ddr3_odt;           // out   std_logic_vector(0 downto 0);
*/

//=============================================================================
// 1)  INTERNAL WIRES/REGISTERS/PARAMETERS DECLARATION
//=============================================================================

// Clock generation
wire               clk_48mhz;
wire               dcm_locked;
wire               dcm_clkfx;
wire               dcm_clk0;
wire               dcm_clkfb;
wire               dco_clk;

// Reset generation
wire               master_reset_n;

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
// RADIO (16 KB)
wire [12:0] omspradio_dmem_addr;
wire               omspradio_dmem_cen;
wire        [15:0] omspradio_dmem_din;
wire         [1:0] omspradio_dmem_wen;
wire        [15:0] omspradio_dmem_dout;

// Program memory
// RADIO (16KB)
wire [12:0] omspradio_pmem_addr;
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

// Programming Serial Interface
wire		omspradio_console_txd;
wire		omspradio_console_rxd;

// Radio Interface UART
wire        omspradio_radio_uart_txd;
wire        omspradio_radio_uart_rxd;

// Battery indicator
wire        omspradio_battery_status_low;

// Jumper inputs
wire		jumper_umbilical_select;
wire		jumper_flash_select;
wire		jumper_eyassat_bypass_select;

// Jumper Inputs (Debounced)
wire		jumper_umbilical_select_debounced;
wire		jumper_flash_select_debounced;
wire		jumper_eyassat_bypass_select_debounced;





// Debug Interface/Programming Interface Multiplexing
wire		omsp0_dbg_rxd;
wire		omsp0_dbg_txd;
wire		omsp0_dbg_en = 1'b1;

// Leon-3 LED outputs
wire [7:0]  leon3_leds;

// Leon-3 GPIOs
wire [13:0] leon3_gpio_dout;
wire [13:0] leon3_gpio_din;
wire [13:0] leon3_gpio_oen;

/*
wire [15:0] ddr3_dq;          // inout std_logic_vector(15 downto 0);
wire [1:0]  ddr3_dqs_p;        // inout std_logic_vector(1 downto 0);
wire [1:0]  ddr3_dqs_n;        // inout std_logic_vector(1 downto 0);
wire [13:0] ddr3_addr;        // out   std_logic_vector(13 downto 0);
wire [2:0]  ddr3_ba;           // out   std_logic_vector(2 downto 0);
wire        ddr3_ras_n;        // out   std_logic;
wire        ddr3_cas_n;        // out   std_logic;
wire        ddr3_we_n;         // out   std_logic;
wire        ddr3_reset_n;       // out   std_logic;
wire        ddr3_ck_p;         // out   std_logic_vector(0 downto 0);
wire        ddr3_ck_n;         // out   std_logic_vector(0 downto 0);
wire        ddr3_cke;          // out   std_logic_vector(0 downto 0);
wire [1:0]  ddr3_dm;            // out   std_logic_vector(1 downto 0);
wire        ddr3_odt;           // out   std_logic_vector(0 downto 0);
*/

// Leon-3 console UART
wire        leon3_console_txd;
wire        leon3_console_rxd;

// Leon-3 umbilical UART
wire        leon3_umbilical_rxd;
wire        leon3_umbilical_txd;

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



//=============================================================================
// 2)  RESET GENERATION & FPGA STARTUP
//=============================================================================

// Reset input buffer
//IBUF   ibuf_reset_n   (.O(reset_pin), .I(USER_RESET));
//assign reset_pin_n = ~reset_pin;

// Release the reset only, if the DCM is locked
//assign  reset_n = reset_pin_n & dcm_locked;
assign master_reset_n = dcm_locked;

//=============================================================================
// 3)  CLOCK GENERATION
//=============================================================================

// Input buffers
//------------------------
IBUFG ibuf_clk_main   (.O(clk_48mhz),    .I(USER_CLOCK));


// Digital Clock Manager
//------------------------
DCM_SP #(.CLKFX_MULTIPLY(4),
	 .CLKFX_DIVIDE(8),
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

//synthesis translate_off
// 16 MHz clock
defparam dcm_inst.CLKFX_MULTIPLY  = 4;
defparam dcm_inst.CLKFX_DIVIDE    = 8;
defparam dcm_inst.CLKIN_PERIOD    = 20.83;
//synthesis translate_on

// Clock buffers
//------------------------
BUFG  buf_sys_clock  (.O(dco_clk), .I(dcm_clkfx));


//=============================================================================
//
// DEBOUNCE HARDWARE I/O
//
//=============================================================================
DeBounce debounce_mode_debug (
	.clk			(dco_clk), 
	.n_reset		(master_reset_n), 
	.button_in	(jumper_umbilical_select),
	.DB_out		(jumper_umbilical_select_debounced)
);

DeBounce debounce_mode_prog (
	.clk			(dco_clk), 
	.n_reset		(master_reset_n), 
	.button_in	(jumper_flash_select),
	.DB_out		(jumper_flash_select_debounced)
);

DeBounce debounce_mode_run (
	.clk			(dco_clk), 
	.n_reset		(master_reset_n), 
	.button_in	(jumper_eyassat_bypass_select),
	.DB_out		(jumper_eyassat_bypass_select_debounced)
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
	.uart_app_rxd			(omspradio_radio_uart_txd),				// UART RXD (connected to Leon-3 processor TXD)
	.uart_app_txd			(omspradio_radio_uart_rxd),				// UART TXD (connected to Leon-3 processor RXD)

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
    .uart_tx_in         (leon3_uart_tx_in),
    .uart_rx_out        (leon3_uart_rx_out),
    
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
    .spi_sck           (leon3_spi_sck)
);

//=============================================================================
// 6)  PROGRAM AND DATA MEMORIES
//=============================================================================


// Data Memory (CPU RADIO PROCESSOR) (16KB)
ram_16x8k_sp_2 ram_dmem_omsp_radio (
    .clka           ( dco_clk),
    .ena            (~omspradio_dmem_cen),
    .wea            (~omspradio_dmem_wen),
    .addra          ( omspradio_dmem_addr),
    .dina           ( omspradio_dmem_din),
    .douta          ( omspradio_dmem_dout)
);





// Program memory (CPU RADIO PROCESSOR) (16KB)
ram_16x8k_sp_2 ram_pmem_omsp_radio (
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

// MSP430 CONSOLE Serial Interface
OBUF    MSP430_CONSOLE_TXD_PIN			(.I(omspradio_console_txd),			.O(MSP430_CONSOLE_TXD));
IBUF    MSP430_CONSOLE_RXD_PIN			(.O(omspradio_console_rxd),			.I(MSP430_CONSOLE_RXD));

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
OBUF    STATUS_LED_LEON_HEARTBEAT_PIN   (.I(leon3_gpio_dout[1]),            .O(STATUS_LED_LEON_HEARTBEAT));
OBUF    STATUS_LED_EXTRA_PIN            (.I(omspradio_status_extra),        .O(STATUS_LED_EXTRA));

// Jumper Pins
IBUF	UMBILICAL_SELECT_JUMPER_PIN		(.O(jumper_umbilical_select),		  .I(UMBILICAL_SELECT_JUMPER));
IBUF	FLASH_SELECT_JUMPER_PIN	        (.O(jumper_flash_select),			  .I(FLASH_SELECT_JUMPER));
IBUF	EYASSAT_BUS_BYPASS_JUMPER_PIN	(.O(jumper_eyassat_bypass_select),	  .I(EYASSAT_BUS_BYPASS_JUMPER));


// Battery Detection I/Os
OBUF	BATTERY_GREEN_LED_PIN	(.I(battery_green_led),				.O(BATTERY_GREEN_LED));
OBUF	BATTERY_YELLOW_LED_PIN	(.I(battery_yellow_led),			.O(BATTERY_YELLOW_LED));

// Leon-3 GPIOs
IOBUF   LEON3_GPIO2_PIN         (.I(leon3_gpio_din[2]), .O(leon3_gpio_dout[2]), .T(leon3_gpio_oen[2]), .IO(LEON3_GPIO2));
IOBUF   LEON3_GPIO3_PIN         (.I(leon3_gpio_din[3]), .O(leon3_gpio_dout[3]), .T(leon3_gpio_oen[3]), .IO(LEON3_GPIO3));
IOBUF   LEON3_GPIO4_PIN         (.I(leon3_gpio_din[4]), .O(leon3_gpio_dout[4]), .T(leon3_gpio_oen[4]), .IO(LEON3_GPIO4));
IOBUF   LEON3_GPIO5_PIN         (.I(leon3_gpio_din[5]), .O(leon3_gpio_dout[5]), .T(leon3_gpio_oen[5]), .IO(LEON3_GPIO5));
IOBUF   LEON3_GPIO6_PIN         (.I(leon3_gpio_din[6]), .O(leon3_gpio_dout[6]), .T(leon3_gpio_oen[6]), .IO(LEON3_GPIO6));
IOBUF   LEON3_GPIO7_PIN         (.I(leon3_gpio_din[7]), .O(leon3_gpio_dout[7]), .T(leon3_gpio_oen[7]), .IO(LEON3_GPIO7));



// Debug Connections
OBUF	MSP430_DEBUG_TXD_PIN		(.I(omsp0_dbg_txd),			.O(MSP430_DEBUG_TXD));
IBUF	MSP430_DEBUG_RXD_PIN		(.O(omsp0_dbg_rxd),			.I(MSP430_DEBUG_RXD));

// Leon-3 Console UART
OBUF    LEON3_CONSOLE_TXD_PIN       (.I(leon3_console_txd),     .O(LEON3_CONSOLE_TXD));
IBUF    LEON3_CONSOLE_RXD_PIN       (.O(leon3_console_rxd),     .I(LEON3_CONSOLE_RXD));

// Leon-3 Eyassat UART
OBUF    LEON3_EYASSAT_TXD_PIN       (.I(leon3_eyassat_txd),     .O(LEON3_EYASSAT_TXD));
IBUF    LEON3_EYASSAT_RXD_PIN       (.O(leon3_eyassat_rxd),     .I(LEON3_EYASSAT_RXD));

// Leon-3 Umbilical (jumper enables) otherwise -- off
OBUF    LEON3_UMBILICAL_TXD_PIN     (.I(leon3_umbilical_txd),   .O(LEON3_UMBILICAL_TXD));
IBUF    LEON3_UMBILICAL_RXD_PIN     (.O(leon3_umbilical_rxd),   .I(LEON3_UMBILICAL_RXD));

// Leon-3 Payload UART
OBUF    LEON3_PAYLOAD_TXD_PIN       (.I(leon3_payload_txd),     .O(LEON3_PAYLOAD_TXD));
IBUF    LEON3_PAYLOAD_RXD_PIN       (.O(leon3_payload_rxd),     .I(LEON3_PAYLOAD_RXD));

OBUF    LEON3_PAYLOAD_POWER_ENABLE_PIN  (.I(leon3_gpio_dout[0]),    .O(LEON3_PAYLOAD_POWER_ENABLE));       

// Assign battery status
assign battery_yellow_led = (omspradio_battery_status_low == 1) ? 1'b1 : 1'b0;
assign battery_green_led  = (omspradio_battery_status_low == 0) ? 1'b1 : 1'b0;

// Assign umbilical connections
assign leon3_umbilical_txd = (jumper_umbilical_select_debounced == 1) ? leon3_radio_txd : 1'b1;
assign leon3_umbilical_rxd = (jumper_umbilical_select_debounced == 1) ? leon3_radio_rxd : 1'bz;

// Assign radio connections on MSP430
assign omspradio_radio_uart_rxd = (jumper_umbilical_select_debounced == 0) ? leon3_radio_txd : 1'b0;
assign omspradio_radio_uart_txd = (jumper_umbilical_select_debounced == 0) ? leon3_radio_rxd : 1'bz;

// Assign who is the master of the config flash (MSP430 is always first)
assign master_config_sck = (omspradio_conflash_csn == 0) ? omspradio_conflash_sck : leon3_spi_sck;
assign master_config_csn = (omspradio_conflash_csn == 0) ? omspradio_conflash_csn : leon3_spi_cs;
assign master_config_miso = (omspradio_conflash_csn == 0) ? omspradio_conflash_miso : leon3_spi_miso;
assign master_config_mosi = (omspradio_conflash_mosi == 0) ? omspradio_conflash_mosi : leon3_spi_mosi;

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

