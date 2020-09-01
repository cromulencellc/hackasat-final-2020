//----------------------------------------------------------------------------
// Copyright (c) 2013, Legitimate Business Syndicate
// Copyright (c) 2020, Cromulence LLC
//
// Do as you please with this software.  Don't blame me if anything is broken
// and doesn't work.  If you use this with the OpemMSP430 project.  Read the
// COPYRIGHT from Oliver Grand and follow it.  Thanks. Essentially,
// this is provided AS-IS.
//
//----------------------------------------------------------------------------
//
// *File Name: uspi_0.v
// 
// *Module Description:
//                       Simple SPI controller (master only)
// Notice there isn't a chip select.  Ya.  you need to do that with GPIO my
// friend.
//
// *Author(s):
//              - sirgoon, Jason Williams (Cromulence LLC)
//
// Note:
//  This was originally used on the 2013 DEF CON Capture the Flag Badger service
// for the OpenMSP430.
//----------------------------------------------------------------------------
module  omsp_uspi_0 (

// OUTPUTs
	 spi_irq_tx_done,						// Interrupt when all data has been sent
    spi_mosi,                    	// Master out SLAVE IN
    spi_clk,                    		// SPI CLK
    per_dout,                       // Peripheral data output

// INPUTs
    mclk,                           // Main system clock
    per_addr,                       // Peripheral address
    per_din,                        // Peripheral data input
    per_en,                         // Peripheral enable (high active)
    per_we,                         // Peripheral write enable (high active)
    puc_rst,                        // Main system reset
    smclk_en,                       // SMCLK enable (from CPU)
    spi_miso                        // Master in SLAVE OUT
);

// OUTPUTs
//=========
output             spi_irq_tx_done; // Interrupt when all data has been sent
output             spi_mosi;			// Master OUT slave IN
output				 spi_clk;			// SPI CLK output always						
output      [15:0] per_dout;        // Peripheral data output

// INPUTs
//=========
input              mclk;            // Main system clock
input       [13:0] per_addr;        // Peripheral address
input       [15:0] per_din;         // Peripheral data input
input              per_en;          // Peripheral enable (high active)
input        [1:0] per_we;          // Peripheral write enable (high active)
input              puc_rst;         // Main system reset
input              smclk_en;        // SMCLK enable (from CPU)
input              spi_miso;        // Master IN slave OUT


//=============================================================================
// 1)  PARAMETER DECLARATION
//=============================================================================

// Register base address (must be aligned to decoder bit width)
parameter       [14:0] BASE_ADDR   = 15'h0090;

// Decoder bit width (defines how many bits are considered for address decoding)
parameter              DEC_WD      =  2;

// Register addresses offset
parameter [DEC_WD-1:0] CTRL        =  'h0,
                       DATA     	  =  'h2;

   
// Register one-hot decoder utilities
parameter              DEC_SZ      =  (1 << DEC_WD);
parameter [DEC_SZ-1:0] BASE_REG    =  {{DEC_SZ-1{1'b0}}, 1'b1};

// Register one-hot decoder
parameter [DEC_SZ-1:0] CTRL_D      = (BASE_REG << CTRL), 
                       DATA_D   	  = (BASE_REG << DATA); 


//============================================================================
// 2)  REGISTER DECODER
//============================================================================

// Local register selection
wire              reg_sel      =  per_en & (per_addr[13:DEC_WD-1]==BASE_ADDR[14:DEC_WD]);

// Register local address
wire [DEC_WD-1:0] reg_addr     =  {per_addr[DEC_WD-2:0],1'b0};

// Register address decode
wire [DEC_SZ-1:0] reg_dec      = (CTRL_D    &  {DEC_SZ{(reg_addr==CTRL)}}) |
                                 (DATA_D 	  &  {DEC_SZ{(reg_addr==DATA)}});

// Read/Write probes
wire              reg_write =  |per_we & reg_sel;
wire              reg_read  = ~|per_we & reg_sel;

// Read/Write vectors
wire [DEC_SZ-1:0] reg_wr    = reg_dec & {DEC_SZ{reg_write}};
wire [DEC_SZ-1:0] reg_rd    = reg_dec & {DEC_SZ{reg_read}};


//============================================================================
// 3) REGISTERS
//============================================================================

// CTRL Register
//-----------------
reg  [15:0] ctrl;

wire		  ctrl_smclk_sel	  = ctrl[9];		// Select SMCLK instead of MCLK
wire		  ctrl_send_cnt	  = ctrl[8];		// High sends two bytes (hi and lo) where as 0 sends 1 byte (just low)
wire       [2:0]ctrl_div 	  = ctrl[7:5];		// Clock dividers  0 = 1, 1 = 2, 2 = 4, 3 = 8, 4 = 16, 5 = 32, 6 = 64, 7 = 128
wire       ctrl_ien       	  = ctrl[4];		// Enable interrupt, interrupt will fire when all bytes have been sent
wire       ctrl_iflg 	  	  = ctrl[3];		// Interrupt flag.  This can be cleared by writing a zero here.
wire       ctrl_ckph         = ctrl[2];		// Clock phase... this is the standard version (not inverted like MSP430 typically does).
wire       ctrl_ckpol    	  = ctrl[1];		// Clock polarity.... this is the standard version (not inverted like MSP430 typically does).
wire       ctrl_en           = ctrl[0];		// Starts transmission (when HI) set to 0 when transmission complete
 

// DATA Register
//-----------------
reg  [15:0] data_tx;

always @ (posedge mclk or posedge puc_rst)
  if (puc_rst)         data_tx <=  16'h0000;
  else if (reg_wr[DATA])   data_tx <= per_din;


// DATA_RX Register
//-----------------
reg  [15:0] data_rx;


//============================================================================
// 4) DATA OUTPUT GENERATION
//============================================================================

// Data output mux
wire [15:0] ctrl_rd 	   = {(ctrl    	& {16{reg_rd[CTRL]}})};
wire [15:0] data_rd     = {(data_rx    & {16{reg_rd[DATA]}})};

wire [15:0] per_dout  =  ctrl_rd    |
                         data_rd;


//=============================================================================
// 5)  CLOCK SELECTION
//=============================================================================

wire uclk_en = ctrl_smclk_sel ? smclk_en : 1'b1;

      
//=============================================================================
// 6)  SPI TRANSFER
//=============================================================================
   
	
// Transfer counter
//------------------------
reg clock_out;					// The output clock
reg data_out;					// The output data
reg [7:0] clock_cnt;			// Counts the number of clocks between transfers
reg [3:0] transfer_bit;		// Which bit we are currently transfering
reg transfer_start;			// Sets high once transfer has started
reg transfer_end;				// Sets high to end transfer
wire [3:0] transfer_bit_start = ctrl_send_cnt ? 4'hF : 4'h7;

// Run control WRITE
always @ (posedge mclk or posedge puc_rst)
  if (puc_rst)       ctrl <=  16'h0000;
  else if (reg_wr[CTRL])
	begin
	  ctrl[15:1] <=  per_din[15:1];
		
	  if ( (ctrl_en == 1'b0) )  
	    ctrl[0] <= per_din[0];
	end
  else if ( (ctrl_en == 1) & (transfer_end == 1'b1) & (clock_cnt == 8'b00) ) ctrl[0] <= 1'b0;
		

// Run clock counter
always @ (posedge mclk or posedge puc_rst)
	if (puc_rst)		clock_cnt <= {8'b0};
	else if (~ctrl_en)  clock_cnt <= {1 << ctrl_div}+1;
	else if ( (ctrl_en == 1) & (clock_cnt != 8'h00) )  clock_cnt <= clock_cnt - 1;
	else if ( (ctrl_en == 1) & (clock_cnt == 8'h00) )  clock_cnt <= {1 << ctrl_div};

// Run bit counter
always @ (posedge mclk or posedge puc_rst)  
	if (puc_rst)	transfer_bit <= 4'h0;
	else if (~ctrl_en)  transfer_bit <= transfer_bit_start;
	else if ( (ctrl_en == 1) & (transfer_bit != 4'h0) & (clock_cnt == 8'b0) )
	    begin
		   if ( clock_out == 1'b1 & ctrl_ckph == 1'b1 ) transfer_bit <= transfer_bit - 1;
			else if ( clock_out == 1'b0 & ctrl_ckph == 1'b0 ) transfer_bit <= transfer_bit - 1;
		 end

// Send interrupt at completion... if enabled.
assign spi_irq_tx_done = (ctrl_ien & (ctrl_en == 1) & (clock_cnt == 8'b0) & (transfer_end == 1'b1));

// Run clock
always @ (posedge mclk or posedge puc_rst) 
	if (puc_rst)		clock_out <= 1'b0;			// Reset state is CPOL=0 so hold clock low while in reset
	else if (~ctrl_en) clock_out <= ctrl_ckpol;  // Clock defaults to same state as ctrl_ckpol (0 clock idles low, 1 clock idles high) 
	else if ( (ctrl_en == 1) & (clock_cnt == 8'h00) ) clock_out <= ~clock_out;
		
// Assign the clock
assign spi_clk = clock_out;
		
// Run transfer start detection
always @ (posedge mclk or posedge puc_rst)
	if (puc_rst)  transfer_start <= 1'b0;
	else if (~ctrl_en)  transfer_start <= 1'b0;
	else if ( (ctrl_en == 1 ) & (transfer_start == 1'b0) ) transfer_start <= 1'b1;   // Remember start of transfer

// Runs transfer end detection
always @ (posedge mclk or posedge puc_rst)
	if (puc_rst)  transfer_end <= 1'b0;
	else if (~ctrl_en)  transfer_end <= 1'b0;
	else if ( (transfer_bit == 4'h0) & (clock_cnt == 8'b0) )
     begin
	    if ( clock_out == 1'b1 & ctrl_ckph == 1'b1 ) transfer_end <= 1'b1;
		 else if ( clock_out == 1'b0 & ctrl_ckph == 1'b0 ) transfer_end <= 1'b1;
	  end
	else if ( (ctrl_en == 1'b1) & (transfer_end == 1'b1) & (clock_cnt == 8'b00) )	transfer_end <= 1'b0;


// Run data
always @ (posedge mclk or posedge puc_rst)
   if (puc_rst) data_rx <= 16'h0000;
	else if ( (ctrl_en == 1) & (transfer_start == 1'b0) )
     begin
   	  data_out <= (data_tx >> transfer_bit) & 1'b1;
		  data_rx <= 16'h0000;		  // Clear the data receive register
	  end
	else if ( (ctrl_en == 1) & (clock_cnt == 8'b0) )
		begin
			if ( clock_out == 1'b0 ) //  Low -> High
				begin
					if ( (ctrl_ckph == 1'b0) )  data_rx <= data_rx | (spi_miso << transfer_bit);
					else if ( (ctrl_ckph == 1'b1) )  data_out <= (data_tx >> transfer_bit) & 1'b1;
				end
			else  // High -> Low
				begin
					
					if ( (ctrl_ckph == 1'b0) )  data_out <= (data_tx >> transfer_bit) & 1'b1;
					else if ( (ctrl_ckph == 1'b1) )  data_rx <= data_rx | (spi_miso << transfer_bit); 
					
				end
		end
  
// Assign output data
assign spi_mosi = data_out;


endmodule // uspi_0
 

