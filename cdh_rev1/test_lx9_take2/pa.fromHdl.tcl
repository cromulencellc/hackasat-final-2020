
# PlanAhead Launch Script for Pre-Synthesis Floorplanning, created by Project Navigator

create_project -name test_lx9_take2 -dir "C:/Users/jdub/Documents/FPGA/test_lx9_take2/planAhead_run_1" -part xc6slx9tqg144-2
set_param project.pinAheadLayout yes
set srcset [get_property srcset [current_run -impl]]
set_property target_constrs_file "openMSP430_fpga.ucf" [current_fileset -constrset]
add_files [list {ipcore_dir/ram_16x4k_sp.ngc}]
add_files [list {ipcore_dir/ram_16x12k_sp.ngc}]
add_files [list {ipcore_dir/ram_16x8k_sp.ngc}]
set hdlfile [add_files [list {omsp_sync_cell.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_sync_reset.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_register_file.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_dbg_uart.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_dbg_hwbrk.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_alu.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_watchdog.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_sfr.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_multiplier.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_mem_backbone.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_frontend.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_execution_unit.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_dbg.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_clock_module.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {uspi_0.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {openMSP430.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_uart2.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_uart.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_timerA.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_gpio.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_system_1.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {omsp_system_0.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {ipcore_dir/ram_16x8k_sp.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {ipcore_dir/ram_16x4k_sp.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
add_files [list {ipcore_dir/ram_16x1k_sp.ngc}]
set hdlfile [add_files [list {ipcore_dir/ram_16x12k_sp.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set hdlfile [add_files [list {openMSP430_fpga.v}]]
set_property file_type Verilog $hdlfile
set_property library work $hdlfile
set_property top openMSP430_fpga $srcset
add_files [list {openMSP430_fpga.ucf}] -fileset [get_property constrset [current_run]]
add_files [list {ipcore_dir/ram_16k_1k_sp.ncf}] -fileset [get_property constrset [current_run]]
add_files [list {ipcore_dir/ram_16x12k_sp.ncf}] -fileset [get_property constrset [current_run]]
add_files [list {ipcore_dir/ram_16x1k_dp.ncf}] -fileset [get_property constrset [current_run]]
add_files [list {ipcore_dir/ram_16x1k_sp.ncf}] -fileset [get_property constrset [current_run]]
add_files [list {ipcore_dir/ram_16x4k_sp.ncf}] -fileset [get_property constrset [current_run]]
add_files [list {ipcore_dir/ram_16x8k_dp.ncf}] -fileset [get_property constrset [current_run]]
add_files [list {ipcore_dir/ram_16x8k_sp.ncf}] -fileset [get_property constrset [current_run]]
open_rtl_design -part xc6slx9tqg144-2
