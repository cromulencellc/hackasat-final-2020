`timescale 1ns / 1ps

////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer:
//
// Create Date:   00:32:20 02/23/2014
// Design Name:   omsp_uspi_0
// Module Name:   C:/Users/jdub/Documents/FPGA/test_lx9_take2/test_spi.v
// Project Name:  test_lx9_take2
// Target Device:  
// Tool versions:  
// Description: 
//
// Verilog Test Fixture created by ISE for module: omsp_uspi_0
//
// Dependencies:
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
////////////////////////////////////////////////////////////////////////////////

module test_spi;

	// Inputs
	reg mclk;
	reg [13:0] per_addr;
	reg [15:0] per_din;
	reg per_en;
	reg [1:0] per_we;
	reg puc_rst;
	reg smclk_en;
	reg spi_miso;

	// Outputs
	wire spi_irq_tx_done;
	wire spi_mosi;
	wire spi_clk;
	wire [15:0] per_dout;

	// Instantiate the Unit Under Test (UUT)
	omsp_uspi_0 uut (
		.spi_irq_tx_done(spi_irq_tx_done), 
		.spi_mosi(spi_mosi), 
		.spi_clk(spi_clk), 
		.per_dout(per_dout), 
		.mclk(mclk), 
		.per_addr(per_addr), 
		.per_din(per_din), 
		.per_en(per_en), 
		.per_we(per_we), 
		.puc_rst(puc_rst), 
		.smclk_en(smclk_en), 
		.spi_miso(spi_miso)
	);

	initial begin
		// Initialize Inputs
		mclk = 0;
		per_addr = 0;
		per_din = 0;
		per_en = 0;
		per_we = 0;
		puc_rst = 1;
		smclk_en = 0;
		spi_miso = 0;

		#90;
		puc_rst = 0;
		
		// Wait 100 ns for global reset to finish
		#10;
        
		// Add stimulus here
		per_en = 1;
		per_we = 2'b01;
		per_addr = 14'h0049;
		per_din = 16'h00AD;
		
		#20;
		per_addr = 14'h0048;
		per_din = 16'h0041;
		
		#20;
		
		per_we = 2'b00;
		spi_miso = 1;
		
		#780;
		per_we = 2'b01;
		per_addr = 14'h0049;
		per_din = 16'h007F;
		
		#20;
		per_addr = 14'h0048;
		per_din = 16'h0001;
		
		#20;
		per_we = 2'b00;
		

	end
	
	always
		#10 mclk = ~mclk;	

      
endmodule

