//----------------------------------------------------------------------------
// Copyright (C) 2013, Legitimate Business Syndicate
//
// Do as you please with this software.  Don't blame me if anything is broken
// and doesn't work.  If you use this with the OpemMSP430 project.  Read the
// COPYRIGHT from Oliver Grand and follow it.  Thanks.
//
//----------------------------------------------------------------------------
//
// *File Name: app_loader.v
// 
// *Module Description:
// 	App Loader -- This module allows the radio processor to load the application
// processor with a new firmware.  This is done to allow them to flash a new
// application processor load and it will be written to the configuration flash
// (in an unused section) and then loaded onto the app processor at bootup by the
// radio processor
//
// *Author(s):
//              - sirgoon (sirgoon@legitbs.net)
//
//----------------------------------------------------------------------------
`include "openMSP430_defines.v"

module  app_loader (

// OUTPUTs
	 per_dout,                       // Peripheral data output
	 app_reset_n,
	 app_pmem_addr,
	 app_pmem_dout,
	 app_pmem_cen,
	 app_pmem_wen,

// INPUTs
    mclk,                           // Main system clock
    per_addr,                       // Peripheral address
    per_din,                        // Peripheral data input
    per_en,                         // Peripheral enable (high active)
    per_we,                         // Peripheral write enable (high active)
    puc_rst,                        // Main system reset
    smclk_en                      // SMCLK enable (from CPU)
);

// OUTPUTs
//=========					
output        [15:0] per_dout;         // Peripheral data output
output				   app_reset_n;		// Holds the app processor in reset
output [`PMEM_MSB:0] app_pmem_addr;		// Address line for pmem 
output		  [15:0] app_pmem_dout;		// Data line for pmem
output				   app_pmem_cen;		// Chip enable for pmem
output			[1:0] app_pmem_wen;		// Write enable for pmem

// INPUTs
//=========
input              mclk;            // Main system clock
input       [13:0] per_addr;        // Peripheral address
input       [15:0] per_din;         // Peripheral data input
input              per_en;          // Peripheral enable (high active)
input        [1:0] per_we;          // Peripheral write enable (high active)
input              puc_rst;         // Main system reset
input              smclk_en;        // SMCLK enable (from CPU)


//=============================================================================
// 1)  PARAMETER DECLARATION
//=============================================================================

// Register base address (must be aligned to decoder bit width)
parameter       [14:0] BASE_ADDR   = 15'h00A8;

// Decoder bit width (defines how many bits are considered for address decoding)
parameter              DEC_WD      =  2;

// Register addresses offset
parameter [DEC_WD-1:0] CTRL        =  'h0,
							  DATA		  =  'h2;

   
// Register one-hot decoder utilities
parameter              DEC_SZ      =  (1 << DEC_WD);
parameter [DEC_SZ-1:0] BASE_REG    =  {{DEC_SZ-1{1'b0}}, 1'b1};

// Register one-hot decoder
parameter [DEC_SZ-1:0] CTRL_D       = (BASE_REG << CTRL), 
                       DATA_D   		= (BASE_REG << DATA);


//============================================================================
// 2)  REGISTER DECODER
//============================================================================

// Local register selection
wire              reg_sel      =  per_en & (per_addr[13:DEC_WD-1]==BASE_ADDR[14:DEC_WD]);

// Register local address
wire [DEC_WD-1:0] reg_addr     =  {per_addr[DEC_WD-2:0],1'b0};

// Register address decode
wire [DEC_SZ-1:0] reg_dec      = (CTRL_D     &  {DEC_SZ{(reg_addr==CTRL)}}) |
                                 (DATA_D 		&  {DEC_SZ{(reg_addr==DATA)}});

// Read/Write probes
wire              reg_write =  |per_we & reg_sel;
wire              reg_read  = ~|per_we & reg_sel;

// Read/Write vectors
wire [DEC_SZ-1:0] reg_wr    = reg_dec & {DEC_SZ{reg_write}};
wire [DEC_SZ-1:0] reg_rd    = reg_dec & {DEC_SZ{reg_read}};


//============================================================================
// 3) REGISTERS
//============================================================================

// ADDRESS Register
//-----------------
reg [`PMEM_MSB:0] address_reg;
reg		  app_reset_n_reg;

// CTRL Register
//-----------------
reg  [15:0] ctrl;
reg	loader_start;

wire       ctrl_en           = ctrl[0];		// Starts writing pmem and holds the device in reset (high enable, low disable)
 
 // Run control WRITE
always @ (posedge mclk or posedge puc_rst)
  if (puc_rst)
   begin
     ctrl <=  16'h0000;
	  app_reset_n_reg <= 1'b1;
	  loader_start <= 1'b0;
	end
  else if (reg_wr[CTRL])
	begin
	  ctrl[15:1] <=  per_din[15:1];
		
	  if ( (ctrl_en == 1'b0 & per_din[0] == 1'b1) )
	    begin
	      ctrl[0] <= 1'b1;
			app_reset_n_reg <= 1'b0;
			loader_start <= 1'b1;
		 end
	  else if ( ctrl_en == 1'b1 & per_din[0] == 1'b0 )
	    begin
		   ctrl[0] <= 1'b0;
			app_reset_n_reg <= 1'b1;
		 end
	end
  else if ( loader_start == 1'b1 ) loader_start <= 1'b0;
		

assign app_reset_n = app_reset_n_reg;
assign app_pmem_addr = address_reg;

reg write_memory_start;
reg write_memory_cen;

// DATA Register
//-----------------
reg  [15:0] data_set;
reg  [15:0] data_out;

// Data register write state machine
always @ (posedge mclk or posedge puc_rst)
  if (puc_rst)
    begin
      data_set <=  16'h0000;
		write_memory_start <= 1'b0;
	 end
  else if (reg_wr[DATA])
    begin
	   if ( ctrl_en == 1'b1 )
		  begin
		    write_memory_start <= 1'b1;
			 data_set <= per_din;
		  end
	 end
	else if ( write_memory_start == 1'b1 ) write_memory_start <= 1'b0;

// Data setup state machine
always @ (negedge mclk or posedge puc_rst)
  if (puc_rst) 
    begin
	   write_memory_cen <= 1'b1;
		data_out <=  16'h0000;
	 end
  else if ( write_memory_start == 1'b1 & write_memory_cen == 1'b1 )
    begin
	   data_out <= data_set;
		write_memory_cen <= 1'b0;
	 end
  else if ( write_memory_cen == 1'b0 )
    begin
	   write_memory_cen <= 1'b1;
	 end
	 
// Address register state machine
always @ (negedge mclk or posedge puc_rst)
  if ( puc_rst ) address_reg <= 16'h0000;
  else if ( loader_start == 1'b1 ) address_reg <= 16'h0000;
  else if ( write_memory_cen == 1'b0 ) address_reg <= address_reg + 16'h1;

assign app_pmem_wen = {write_memory_cen, write_memory_cen};
assign app_pmem_cen = write_memory_cen;

assign app_pmem_dout = data_out;


//============================================================================
// 4) DATA OUTPUT GENERATION
//============================================================================

// Data output mux
wire [15:0] ctrl_rd 	   = {(ctrl    	& {16{reg_rd[CTRL]}})};
wire [15:0] data_rd     = {(data_set    & {16{reg_rd[DATA]}})};

wire [15:0] per_dout  =  ctrl_rd    |
                         data_rd;

endmodule // keyfile_writer
