//----------------------------------------------------------------------------
// Copyright (C) 2013, Legitimate Business Syndicate
//
// Do as you please with this software.  Don't blame me if anything is broken
// and doesn't work.  If you use this with the OpemMSP430 project.  Read the
// COPYRIGHT from Oliver Grand and follow it.  Thanks.
//
//----------------------------------------------------------------------------
//
// *File Name: uspi_1.v
// 
// *Module Description:
// 	Keyfile writer -- Allows the radio processor to write the keyfile
// for the application processor (read by the keyfile reader)
//
// *Author(s):
//              - sirgoon (sirgoon@legitbs.net)
//
//----------------------------------------------------------------------------
module  keyfile_writer (

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
	key_data_out							// This allows the keyfile_reader to read the keyfile register
);

// OUTPUTs
//=========					
output      [15:0] per_dout;        // Peripheral data output
output	   [63:0] key_data_out;			// Stores the key data

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
parameter       [14:0] BASE_ADDR   = 15'h00A0;

// Decoder bit width (defines how many bits are considered for address decoding)
parameter              DEC_WD      =  3;

// Register addresses offset
parameter [DEC_WD-1:0] KEY_0        =  'h0,
                       KEY_1     	=  'h2,
							  KEY_2			=  'h4,
							  KEY_3			=  'h6;

   
// Register one-hot decoder utilities
parameter              DEC_SZ      =  (1 << DEC_WD);
parameter [DEC_SZ-1:0] BASE_REG    =  {{DEC_SZ-1{1'b0}}, 1'b1};

// Register one-hot decoder
parameter [DEC_SZ-1:0] KEY_0_D      = (BASE_REG << KEY_0), 
                       KEY_1_D   	= (BASE_REG << KEY_1),
                       KEY_2_D   	= (BASE_REG << KEY_2),                       
							  KEY_3_D   	= (BASE_REG << KEY_3);


//============================================================================
// 2)  REGISTER DECODER
//============================================================================

// Local register selection
wire              reg_sel      =  per_en & (per_addr[13:DEC_WD-1]==BASE_ADDR[14:DEC_WD]);

// Register local address
wire [DEC_WD-1:0] reg_addr     =  {per_addr[DEC_WD-2:0],1'b0};

// Register address decode
wire [DEC_SZ-1:0] reg_dec      = (KEY_0_D    &  {DEC_SZ{(reg_addr==KEY_0)}}) |
                                 (KEY_1_D 	&  {DEC_SZ{(reg_addr==KEY_1)}}) |
											(KEY_2_D 	&  {DEC_SZ{(reg_addr==KEY_2)}}) |
											(KEY_3_D 	&  {DEC_SZ{(reg_addr==KEY_3)}});

// Read/Write probes
wire              reg_write =  |per_we & reg_sel;
wire              reg_read  = ~|per_we & reg_sel;

// Read/Write vectors
wire [DEC_SZ-1:0] reg_wr    = reg_dec & {DEC_SZ{reg_write}};
wire [DEC_SZ-1:0] reg_rd    = reg_dec & {DEC_SZ{reg_read}};


//============================================================================
// 3) REGISTERS
//============================================================================

reg [63:0] keyfile_data_reg;

// DATA write
//-----------------
always @ (posedge mclk or posedge puc_rst)
  if (puc_rst)         keyfile_data_reg <=  64'h0;
  else if (reg_wr[KEY_0])   keyfile_data_reg[63:48] <= per_din;
  else if (reg_wr[KEY_1]) 	 keyfile_data_reg[47:32] <= per_din;
  else if (reg_wr[KEY_2])	 keyfile_data_reg[31:16] <= per_din;
  else if (reg_wr[KEY_3])	 keyfile_data_reg[15:0] <= per_din;
  

// ASSIGN The keyfile
assign key_data_out = keyfile_data_reg;


//============================================================================
// 4) DATA OUTPUT GENERATION
//============================================================================

// Data output mux
wire [15:0] key0_rd 	   = {(keyfile_data_reg[63:48]   	& {16{reg_rd[KEY_0]}})};
wire [15:0] key1_rd 	   = {(keyfile_data_reg[47:32]   	& {16{reg_rd[KEY_1]}})};
wire [15:0] key2_rd 	   = {(keyfile_data_reg[31:16]   	& {16{reg_rd[KEY_2]}})};
wire [15:0] key3_rd 	   = {(keyfile_data_reg[15:0]   	& {16{reg_rd[KEY_3]}})};

wire [15:0] per_dout  =  key0_rd | key1_rd | key2_rd | key3_rd;

endmodule // keyfile_writer