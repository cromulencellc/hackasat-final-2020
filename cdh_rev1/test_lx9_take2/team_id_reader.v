//----------------------------------------------------------------------------
// Copyright (C) 2013, Legitimate Business Syndicate
//
// Do as you please with this software.  Don't blame me if anything is broken
// and doesn't work.  If you use this with the OpemMSP430 project.  Read the
// COPYRIGHT from Oliver Grand and follow it.  Thanks.
//
//----------------------------------------------------------------------------
//
// *File Name: team_id_reader.v
// 
// *Module Description:
// 	Team ID reader allows the app processor to read a memory mapped address
// and retrieve the Team ID from the radio (it is read only)
//
// *Author(s):
//              - sirgoon (sirgoon@legitbs.net)
//
//----------------------------------------------------------------------------
module  team_id_reader (

// OUTPUTs
	 per_dout,                       // Peripheral data output

// INPUTs
    mclk,                           // Main system clock
    per_addr,                       // Peripheral address
    per_din,                        // Peripheral data input
    per_en,                         // Peripheral enable (high active)
    per_we,                         // Peripheral write enable (high active)
    puc_rst,                        // Main system reset
    smclk_en,                      // SMCLK enable (from CPU)
	 
// INOUT
	team_id_in							  // Contains the keyfile data
);

// OUTPUTs
//=========					
output      [15:0] per_dout;        // Peripheral data output
input	   	[15:0] team_id_in;		// Retrieves the team_id

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
parameter       [14:0] BASE_ADDR   = 15'h01B0;

// Decoder bit width (defines how many bits are considered for address decoding)
parameter              DEC_WD      =  2;

// Register addresses offset
parameter [DEC_WD-1:0] TEAM_ID_0        =  'h0;

   
// Register one-hot decoder utilities
parameter              DEC_SZ      =  (1 << DEC_WD);
parameter [DEC_SZ-1:0] BASE_REG    =  {{DEC_SZ-1{1'b0}}, 1'b1};

// Register one-hot decoder
parameter [DEC_SZ-1:0] TEAM_ID_0_D      = (BASE_REG << TEAM_ID_0);


//============================================================================
// 2)  REGISTER DECODER
//============================================================================

// Local register selection
wire              reg_sel      =  per_en & (per_addr[13:DEC_WD-1]==BASE_ADDR[14:DEC_WD]);

// Register local address
wire [DEC_WD-1:0] reg_addr     =  {per_addr[DEC_WD-2:0],1'b0};

// Register address decode
wire [DEC_SZ-1:0] reg_dec      = (TEAM_ID_0_D    &  {DEC_SZ{(reg_addr==TEAM_ID_0)}});

// Read/Write probes
wire              reg_write =  |per_we & reg_sel;
wire              reg_read  = ~|per_we & reg_sel;

// Read/Write vectors
wire [DEC_SZ-1:0] reg_wr    = reg_dec & {DEC_SZ{reg_write}};
wire [DEC_SZ-1:0] reg_rd    = reg_dec & {DEC_SZ{reg_read}};


//============================================================================
// 4) DATA OUTPUT GENERATION
//============================================================================

// Data output mux
wire [15:0] team_id_rd 	   = {(team_id_in[15:0]   	& {16{reg_rd[TEAM_ID_0]}})};

wire [15:0] per_dout  =  team_id_rd;

endmodule // team_id_reader