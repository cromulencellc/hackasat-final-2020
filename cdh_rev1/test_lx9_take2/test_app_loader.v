`timescale 1ns / 1ps

////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer:
//
// Create Date:   23:24:26 06/08/2014
// Design Name:   app_loader
// Module Name:   C:/Users/jdub/Documents/FPGA/test_lx9_take2/test_app_loader.v
// Project Name:  test_lx9_take2
// Target Device:  
// Tool versions:  
// Description: 
//
// Verilog Test Fixture created by ISE for module: app_loader
//
// Dependencies:
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
////////////////////////////////////////////////////////////////////////////////

module test_app_loader;

	// Inputs
	reg mclk;
	reg [13:0] per_addr;
	reg [15:0] per_din;
	reg per_en;
	reg [1:0] per_we;
	reg puc_rst;
	reg smclk_en;

	// Outputs
	wire [15:0] per_dout;
	wire app_reset_n;
	wire [12:0] app_pmem_addr;
	wire [15:0] app_pmem_dout;
	wire app_pmem_cen;
	wire [1:0] app_pmem_wen;

	// Instantiate the Unit Under Test (UUT)
	app_loader uut (
		.per_dout(per_dout), 
		.app_reset_n(app_reset_n), 
		.app_pmem_addr(app_pmem_addr), 
		.app_pmem_dout(app_pmem_dout), 
		.app_pmem_cen(app_pmem_cen), 
		.app_pmem_wen(app_pmem_wen), 
		.mclk(mclk), 
		.per_addr(per_addr), 
		.per_din(per_din), 
		.per_en(per_en), 
		.per_we(per_we), 
		.puc_rst(puc_rst), 
		.smclk_en(smclk_en)
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

		#90;
		puc_rst = 0;
		
		// Wait 100 ns for global reset to finish
		#10;
        
		// Add stimulus here
		per_en = 1;
		per_we = 2'b01;
		per_addr = 14'h0054;
		per_din = 16'h0001;
		
		#20;
		per_en = 0;
		per_we = 2'b00;
		
		#20;
		per_addr = 14'h0055;
		per_din = 16'h4dac;
		per_en = 1;
		per_we = 2'b11;
		
		#20;
		per_addr = 14'h0055;
		per_din = 16'h73cc;
		per_en = 1;
		per_we = 2'b11;
		
		#780;
		per_we = 2'b01;
		per_addr = 14'h0054;
		per_din = 16'h0000;

	end
	
	always
		#10 mclk = ~mclk;
      
endmodule

