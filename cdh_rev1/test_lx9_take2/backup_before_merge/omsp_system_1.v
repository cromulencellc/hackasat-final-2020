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
// *File Name: omsp_system_0.v
// 
// *Module Description:
//                      openMSP430 System 1.
//                      This core is dedicated to computing and can
//                      only drive two leds on the board.
//                      It can also read the switches value.
//                      
//
// *Author(s):
//              - Olivier Girard,    olgirard@gmail.com
//
//----------------------------------------------------------------------------
`include "openMSP430_defines.v"

module omsp_system_1 (

// Clock & Reset
    dco_clk,                               // Fast oscillator (fast clock)
    reset_n,                               // Reset Pin (low active, asynchronous and non-glitchy)
    
    // Serial Debug Interface (UART)
	dbg_uart_rxd,
	dbg_uart_txd,
	dbg_en,

// Serial Debug Interface (I2C)
    dbg_i2c_addr,                          // Debug interface: I2C Address
    dbg_i2c_broadcast,                     // Debug interface: I2C Broadcast Address (for multicore systems)
    dbg_i2c_scl,                           // Debug interface: I2C SCL
    dbg_i2c_sda_in,                        // Debug interface: I2C SDA IN
    dbg_i2c_sda_out,                       // Debug interface: I2C SDA OUT

// Data Memory
    dmem_addr,                             // Data Memory address
    dmem_cen,                              // Data Memory chip enable (low active)
    dmem_din,                              // Data Memory data input
    dmem_wen,                              // Data Memory write enable (low active)
    dmem_dout,                             // Data Memory data output

// Program Memory
    pmem_addr,                             // Program Memory address
    pmem_cen,                              // Program Memory chip enable (low active)
    pmem_din,                              // Program Memory data input (optional)
    pmem_wen,                              // Program Memory write enable (low active) (optional)
    pmem_dout,                             // Program Memory data output

// SPI RADIO
	 spi_radio_mosi,								  // SPI Radio Master OUT Slave IN
	 spi_radio_miso,							 	  // SPI Radio Master IN Slave OUT
	 spi_radio_clk,								  // SPI Radio CLOCK
	 spi_radio_csn,
	 
// SPI Radio Control
	spi_radio_reset,								  // Resets the radio chip
	spi_radio_dio0,
	spi_radio_dio1,
	spi_radio_dio2,
	spi_radio_dio3,
	spi_radio_dio4,
	spi_radio_dio5,
	
	 
// SPI FLASH
    spi_conflash_mosi,							  // SPI Master OUT Slave IN for the configuration flash
	 spi_conflash_miso,							  // SPI Master IN Slave OUT for the configuration flash
	 spi_conflash_clk,							  // SPI CLK for the configuration flash
	 spi_conflash_csn,
	 
// UART Program
	uart_program_rxd,								  // UART RXD
	uart_program_txd,								  // UART TXD
	
// UART App Processor
	uart_app_rxd,									  // UART RXD
	uart_app_txd,									  // UART TXD

// Switches
	jumper_umbilical_select,
	jumper_flash_select,
	jumper_eyassat_bypass_select,
	
	leon3_reset_n,
    battery_status_low,
    status_output,
    status_radio_link,
    status_extra
);

// Clock & Reset
input                dco_clk;              // Fast oscillator (fast clock)
input                reset_n;              // Reset Pin (low active, asynchronous and non-glitchy)

// Serial Debug Interface (UART)
input						dbg_uart_rxd;
output					dbg_uart_txd;
input						dbg_en;

// Serial Debug Interface (I2C)
input          [6:0] dbg_i2c_addr;         // Debug interface: I2C Address
input          [6:0] dbg_i2c_broadcast;    // Debug interface: I2C Broadcast Address (for multicore systems)
input                dbg_i2c_scl;          // Debug interface: I2C SCL
input                dbg_i2c_sda_in;       // Debug interface: I2C SDA IN
output               dbg_i2c_sda_out;      // Debug interface: I2C SDA OUT

// Data Memory
input         [15:0] dmem_dout;            // Data Memory data output
output [`DMEM_MSB:0] dmem_addr;            // Data Memory address
output               dmem_cen;             // Data Memory chip enable (low active)
output        [15:0] dmem_din;             // Data Memory data input
output         [1:0] dmem_wen;             // Data Memory write enable (low active)

// Program Memory
input         [15:0] pmem_dout;            // Program Memory data output
output [`PMEM_MSB:0] pmem_addr;            // Program Memory address
output               pmem_cen;             // Program Memory chip enable (low active)
output        [15:0] pmem_din;             // Program Memory data input (optional)
output         [1:0] pmem_wen;             // Program Memory write enable (low active) (optional)

// SPI Radio
output					spi_radio_mosi;		 // SPI Radio Master OUT Slave IN
input	 					spi_radio_miso;		 // SPI Radio Master IN Slave OUT
output					spi_radio_clk;			 // SPI Radio CLOCK
output					spi_radio_csn;

output					spi_radio_reset;
input						spi_radio_dio0;
input						spi_radio_dio1;
input						spi_radio_dio2;
input						spi_radio_dio3;
input						spi_radio_dio4;
input						spi_radio_dio5;

// SPI Configuration Flash
output					spi_conflash_mosi;		 // SPI Configuration Flash Master OUT Slave IN
input	 					spi_conflash_miso;		 // SPI Configuration Flash Master IN Slave OUT
output					spi_conflash_clk;			 // SPI Configuration Flash CLOCK
output					spi_conflash_csn;

// UART Program
output 					uart_program_txd;
input						uart_program_rxd;

// UART Application Processor (baseband)
output					uart_app_txd;
input						uart_app_rxd;   

// Mode operation input selections
input					jumper_umbilical_select;
input                   jumper_flash_select;
input	                jumper_eyassat_bypass_select;

// Outputs
output					leon3_reset_n;
output					battery_status_low;
output					status_output;
output					status_radio_link;
output					status_extra;


//=============================================================================
// 1)  INTERNAL WIRES/REGISTERS/PARAMETERS DECLARATION
//=============================================================================

// Clock & Reset
wire               mclk;
wire               aclk_en;
wire               smclk_en;
wire               puc_rst;

// Debug interface
wire               dbg_freeze;

// Data memory
wire [`DMEM_MSB:0] dmem_addr;
wire               dmem_cen;
wire        [15:0] dmem_din;
wire         [1:0] dmem_wen;
wire        [15:0] dmem_dout;

// Program memory
wire [`PMEM_MSB:0] pmem_addr;
wire               pmem_cen;
wire        [15:0] pmem_din;
wire         [1:0] pmem_wen;
wire        [15:0] pmem_dout;

// Peripheral bus
wire        [13:0] per_addr;
wire        [15:0] per_din;
wire         [1:0] per_we;
wire               per_en;
wire        [15:0] per_dout;

// Interrupts
wire        [13:0] irq_acc;
wire   	    [13:0] irq_bus;
wire   	           nmi;

// GPIO
wire         [7:0] p1_din;
wire         [7:0] p1_dout;
wire         [7:0] p1_dout_en;
wire         [7:0] p1_sel;
wire         [7:0] p2_din;
wire         [7:0] p2_dout;
wire         [7:0] p2_dout_en;
wire         [7:0] p2_sel;
wire        [15:0] per_dout_gpio;

// Timer A
wire        [15:0] per_dout_tA;

// Hardware SPI Radio
wire			[15:0] per_dout_spi_radio;

// Hardware SPI Configuration Flash
wire 		   [15:0] per_dout_spi_conflash;

// UART Program
wire			[15:0] per_dout_uart_program;

// UART Application Processor
wire			[15:0] per_dout_uart_app;

//=============================================================================
// 2)  OPENMSP430 CORE
//=============================================================================

openMSP430 #(.INST_NR (0),
             .TOTAL_NR(1)) openMSP430_0 (

// OUTPUTs
    .aclk              (),                   // ASIC ONLY: ACLK
    .aclk_en           (aclk_en),            // FPGA ONLY: ACLK enable
    .dbg_freeze        (dbg_freeze),         // Freeze peripherals
    .dbg_i2c_sda_out   (dbg_i2c_sda_out),    // Debug interface: I2C SDA OUT
    .dbg_uart_txd      (dbg_uart_txd),       // Debug interface: UART TXD
    .dco_enable        (),                   // ASIC ONLY: Fast oscillator enable
    .dco_wkup          (),                   // ASIC ONLY: Fast oscillator wake-up (asynchronous)
    .dmem_addr         (dmem_addr),          // Data Memory address
    .dmem_cen          (dmem_cen),           // Data Memory chip enable (low active)
    .dmem_din          (dmem_din),           // Data Memory data input
    .dmem_wen          (dmem_wen),           // Data Memory write enable (low active)
    .irq_acc           (irq_acc),            // Interrupt request accepted (one-hot signal)
    .lfxt_enable       (),                   // ASIC ONLY: Low frequency oscillator enable
    .lfxt_wkup         (),                   // ASIC ONLY: Low frequency oscillator wake-up (asynchronous)
    .mclk              (mclk),               // Main system clock
    .per_addr          (per_addr),           // Peripheral address
    .per_din           (per_din),            // Peripheral data input
    .per_we            (per_we),             // Peripheral write enable (high active)
    .per_en            (per_en),             // Peripheral enable (high active)
    .pmem_addr         (pmem_addr),          // Program Memory address
    .pmem_cen          (pmem_cen),           // Program Memory chip enable (low active)
    .pmem_din          (pmem_din),           // Program Memory data input (optional)
    .pmem_wen          (pmem_wen),           // Program Memory write enable (low active) (optional)
    .puc_rst           (puc_rst),            // Main system reset
    .smclk             (),                   // ASIC ONLY: SMCLK
    .smclk_en          (smclk_en),           // FPGA ONLY: SMCLK enable

// INPUTs
    .cpu_en            (1'b1),               // Enable CPU code execution (asynchronous and non-glitchy)
    .dbg_en            (dbg_en),             // Debug interface enable (asynchronous and non-glitchy)
    .dbg_i2c_addr      (dbg_i2c_addr),       // Debug interface: I2C Address
    .dbg_i2c_broadcast (dbg_i2c_broadcast),  // Debug interface: I2C Broadcast Address (for multicore systems)
    .dbg_i2c_scl       (dbg_i2c_scl),        // Debug interface: I2C SCL
    .dbg_i2c_sda_in    (dbg_i2c_sda_in),     // Debug interface: I2C SDA IN
    .dbg_uart_rxd      (dbg_uart_rxd),       // Debug interface: UART RXD (asynchronous)
    .dco_clk           (dco_clk),            // Fast oscillator (fast clock)
    .dmem_dout         (dmem_dout),          // Data Memory data output
    .irq               (irq_bus),            // Maskable interrupts
    .lfxt_clk          (1'b0),               // Low frequency oscillator (typ 32kHz)
    .nmi               (nmi),                // Non-maskable interrupt (asynchronous)
    .per_dout          (per_dout),           // Peripheral data output
    .pmem_dout         (pmem_dout),          // Program Memory data output
    .reset_n           (reset_n),            // Reset Pin (low active, asynchronous and non-glitchy)
    .scan_enable       (1'b0),               // ASIC ONLY: Scan enable (active during scan shifting)
    .scan_mode         (1'b0),               // ASIC ONLY: Scan mode
    .wkup              (1'b0)                // ASIC ONLY: System Wake-up (asynchronous and non-glitchy)
);


//=============================================================================
// 3)  OPENMSP430 PERIPHERALS
//=============================================================================

//
// Digital I/O
//-------------------------------

omsp_gpio #(.P1_EN(1),
            .P2_EN(1),
            .P3_EN(0),
            .P4_EN(0),
            .P5_EN(0),
            .P6_EN(0)) gpio_0 (

// OUTPUTs
    .irq_port1    (irq_port1),             // Port 1 interrupt
    .irq_port2    (irq_port2),             // Port 2 interrupt
    .p1_dout      (p1_dout),               // Port 1 data output
    .p1_dout_en   (p1_dout_en),            // Port 1 data output enable
    .p1_sel       (p1_sel),                // Port 1 function select
    .p2_dout      (p2_dout),               // Port 2 data output
    .p2_dout_en   (p2_dout_en),            // Port 2 data output enable
    .p2_sel       (p2_sel),                // Port 2 function select
    .p3_dout      (),                      // Port 3 data output
    .p3_dout_en   (),                      // Port 3 data output enable
    .p3_sel       (),                      // Port 3 function select
    .p4_dout      (),                      // Port 4 data output
    .p4_dout_en   (),                      // Port 4 data output enable
    .p4_sel       (),                      // Port 4 function select
    .p5_dout      (),                      // Port 5 data output
    .p5_dout_en   (),                      // Port 5 data output enable
    .p5_sel       (),                      // Port 5 function select
    .p6_dout      (),                      // Port 6 data output
    .p6_dout_en   (),                      // Port 6 data output enable
    .p6_sel       (),                      // Port 6 function select
    .per_dout     (per_dout_gpio),         // Peripheral data output
			     
// INPUTs
    .mclk         (mclk),                  // Main system clock
    .p1_din       (p1_din),                // Port 1 data input
    .p2_din       (p2_din),                // Port 2 data input
    .p3_din       (8'h00),                 // Port 3 data input
    .p4_din       (8'h00),                 // Port 4 data input
    .p5_din       (8'h00),                 // Port 5 data input
    .p6_din       (8'h00),                 // Port 6 data input
    .per_addr     (per_addr),              // Peripheral address
    .per_din      (per_din),               // Peripheral data input
    .per_en       (per_en),                // Peripheral enable (high active)
    .per_we       (per_we),                // Peripheral write enable (high active)
    .puc_rst      (puc_rst)                // Main system reset
);

// Assign LEDs
// assign  led         = p2_dout[1:0] & p2_dout_en[1:0];
assign  spi_radio_csn		= p2_dout[0] & p2_dout_en[0];
assign  spi_conflash_csn	= p2_dout[1] & p2_dout_en[1];
assign  spi_radio_reset		= p2_dout[2] & p2_dout_en[2];
assign  leon3_reset_n       = p2_dout[3] & p2_dout_en[3];
assign  battery_status_low  = p2_dout[4] & p2_dout_en[4];
assign  status_output       = p2_dout[5] & p2_dout_en[5];
assign  status_radio_link   = p2_dout[6] & p2_dout_en[6];
assign  status_extra        = p2_dout[7] & p2_dout_en[7];

// Assign Switches
//assign  p1_din[7:3] = 4'h0;
// spi_radio_dio5 not connected.
assign  p1_din[7] = spi_radio_dio4;
assign  p1_din[6] = spi_radio_dio3;
assign  p1_din[5] = spi_radio_dio2;
assign  p1_din[4] = spi_radio_dio1;
assign  p1_din[3] = spi_radio_dio0;
assign  p1_din[2] = jumper_umbilical_select;
assign  p1_din[1] = jumper_flash_select;
assign  p1_din[0] = jumper_eyassat_bypass_select;

//
// Timer A
//----------------------------------------------

omsp_timerA timerA_0 (

// OUTPUTs
    .irq_ta0      (irq_ta0),               // Timer A interrupt: TACCR0
    .irq_ta1      (irq_ta1),               // Timer A interrupt: TAIV, TACCR1, TACCR2
    .per_dout     (per_dout_tA),           // Peripheral data output
    .ta_out0      (),                      // Timer A output 0
    .ta_out0_en   (),                      // Timer A output 0 enable
    .ta_out1      (),                      // Timer A output 1
    .ta_out1_en   (),                      // Timer A output 1 enable
    .ta_out2      (),                      // Timer A output 2
    .ta_out2_en   (),                      // Timer A output 2 enable

// INPUTs
    .aclk_en      (aclk_en),               // ACLK enable (from CPU)
    .dbg_freeze   (dbg_freeze),            // Freeze Timer A counter
    .inclk        (1'b0),                  // INCLK external timer clock (SLOW)
    .irq_ta0_acc  (irq_acc[9]),            // Interrupt request TACCR0 accepted
    .mclk         (mclk),                  // Main system clock
    .per_addr     (per_addr),              // Peripheral address
    .per_din      (per_din),               // Peripheral data input
    .per_en       (per_en),                // Peripheral enable (high active)
    .per_we       (per_we),                // Peripheral write enable (high active)
    .puc_rst      (puc_rst),               // Main system reset
    .smclk_en     (smclk_en),              // SMCLK enable (from CPU)
    .ta_cci0a     (1'b0),                  // Timer A capture 0 input A
    .ta_cci0b     (1'b0),                  // Timer A capture 0 input B
    .ta_cci1a     (1'b0),                  // Timer A capture 1 input A
    .ta_cci1b     (1'b0),                  // Timer A capture 1 input B
    .ta_cci2a     (1'b0),                  // Timer A capture 2 input A
    .ta_cci2b     (1'b0),                  // Timer A capture 2 input B
    .taclk        (1'b0)                   // TACLK external timer clock (SLOW)
);

//
// Hardware UART
//----------------------------------------------

omsp_uart uart_program (

// OUTPUTs
    .irq_uart_rx  (irq_uart_program_rx),           // UART receive interrupt
    .irq_uart_tx  (irq_uart_program_tx),           // UART transmit interrupt
    .per_dout     (per_dout_uart_program),         // Peripheral data output
    .uart_txd     (uart_program_txd),              // UART Data Transmit (TXD)

// INPUTs
    .mclk         (mclk),                  // Main system clock
    .per_addr     (per_addr),              // Peripheral address
    .per_din      (per_din),               // Peripheral data input
    .per_en       (per_en),                // Peripheral enable (high active)
    .per_we       (per_we),                // Peripheral write enable (high active)
    .puc_rst      (puc_rst),               // Main system reset
    .smclk_en     (smclk_en),              // SMCLK enable (from CPU)
    .uart_rxd     (uart_program_rxd)       // UART Data Receive (RXD)
);

omsp_uart2 uart_app (

// OUTPUTs
	 .irq_uart_rx  (irq_uart_app_rx),           // UART receive interrupt
    .irq_uart_tx  (irq_uart_app_tx),           // UART transmit interrupt
    .per_dout     (per_dout_uart_app),         // Peripheral data output
    .uart_txd     (uart_app_txd),              // UART Data Transmit (TXD)

// INPUTs
    .mclk         (mclk),                  // Main system clock
    .per_addr     (per_addr),              // Peripheral address
    .per_din      (per_din),               // Peripheral data input
    .per_en       (per_en),                // Peripheral enable (high active)
    .per_we       (per_we),                // Peripheral write enable (high active)
    .puc_rst      (puc_rst),               // Main system reset
    .smclk_en     (smclk_en),              // SMCLK enable (from CPU)
    .uart_rxd     (uart_app_rxd)        	 // UART Data Receive (RXD)
);

// SX1272 SPI Controller
omsp_uspi_0 uspi_radio (
// OUTPUTS
	.spi_irq_tx_done   (irq_spi_radio_tx_done),
	.spi_mosi			 (spi_radio_mosi),
	.spi_clk				 (spi_radio_clk),
	.per_dout			 (per_dout_spi_radio),


// INPUTS
	 .mclk         (mclk),                  // Main system clock
    .per_addr     (per_addr),              // Peripheral address
    .per_din      (per_din),               // Peripheral data input
    .per_en       (per_en),                // Peripheral enable (high active)
    .per_we       (per_we),                // Peripheral write enable (high active)
    .puc_rst      (puc_rst),               // Main system reset
    .smclk_en     (smclk_en),              // SMCLK enable (from CPU)
	 .spi_miso		(spi_radio_miso)
);

// Configuration Flash SPI Controller
omsp_uspi_1 uspi_conflash (
// OUTPUTS
	.spi_irq_tx_done   (irq_spi_conflash_tx_done),
	.spi_mosi			 (spi_conflash_mosi),
	.spi_clk				 (spi_conflash_clk),
	.per_dout			 (per_dout_spi_conflash),


// INPUTS
	 .mclk         (mclk),                  // Main system clock
    .per_addr     (per_addr),              // Peripheral address
    .per_din      (per_din),               // Peripheral data input
    .per_en       (per_en),                // Peripheral enable (high active)
    .per_we       (per_we),                // Peripheral write enable (high active)
    .puc_rst      (puc_rst),               // Main system reset
    .smclk_en     (smclk_en),              // SMCLK enable (from CPU)
	 .spi_miso		(spi_conflash_miso)
);

//
// Combine peripheral data buses
//-------------------------------

assign per_dout = per_dout_gpio | per_dout_tA | per_dout_uart_program | per_dout_uart_app | per_dout_spi_radio | per_dout_spi_conflash;

//
// Assign interrupts
//-------------------------------

assign nmi      =   1'b0;
assign irq_bus  =  {1'b0,         				 // Vector 13  (0xFFFA)
                    irq_spi_conflash_tx_done, // Vector 12  (0xFFF8)
                    irq_spi_radio_tx_done,    // Vector 11  (0xFFF6)
                    1'b0,         				 // Vector 10  (0xFFF4) - Watchdog -
                    irq_ta0,      	 			 // Vector  9  (0xFFF2)
                    irq_ta1,          			 // Vector  8  (0xFFF0)
                    irq_port2,      	 		 // Vector  7  (0xFFEE)
                    irq_port1,      	 		 // Vector  6  (0xFFEC)
                    irq_uart_app_rx,      	 // Vector  5  (0xFFEA)
                    irq_uart_app_tx,      	 // Vector  4  (0xFFE8)
                    irq_uart_program_rx,      // Vector  3  (0xFFE6)
                    irq_uart_program_tx,      // Vector  2  (0xFFE4)
                    1'b0,    				 		 // Vector  1  (0xFFE2)
                    1'b0};   				 		 // Vector  0  (0xFFE0)


endmodule // omsp_system_1



