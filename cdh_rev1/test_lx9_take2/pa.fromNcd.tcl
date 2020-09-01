
# PlanAhead Launch Script for Post PAR Floorplanning, created by Project Navigator

create_project -name test_lx9_take2 -dir "C:/Users/jdub/Documents/FPGA/test_lx9_take2/planAhead_run_5" -part xc6slx9tqg144-2
set srcset [get_property srcset [current_run -impl]]
set_property design_mode GateLvl $srcset
set_property edif_top_file "C:/Users/jdub/Documents/FPGA/test_lx9_take2/openMSP430_fpga.ngc" [ get_property srcset [ current_run ] ]
add_files -norecurse { {C:/Users/jdub/Documents/FPGA/test_lx9_take2} {ipcore_dir} }
add_files [list {ipcore_dir/ram_16k_1k_sp.ncf}] -fileset [get_property constrset [current_run]]
add_files [list {ipcore_dir/ram_16x12k_sp.ncf}] -fileset [get_property constrset [current_run]]
add_files [list {ipcore_dir/ram_16x1k_dp.ncf}] -fileset [get_property constrset [current_run]]
add_files [list {ipcore_dir/ram_16x1k_sp.ncf}] -fileset [get_property constrset [current_run]]
add_files [list {ipcore_dir/ram_16x4k_sp.ncf}] -fileset [get_property constrset [current_run]]
add_files [list {ipcore_dir/ram_16x8k_dp.ncf}] -fileset [get_property constrset [current_run]]
add_files [list {ipcore_dir/ram_16x8k_sp.ncf}] -fileset [get_property constrset [current_run]]
set_property target_constrs_file "openMSP430_fpga.ucf" [current_fileset -constrset]
add_files [list {openMSP430_fpga.ucf}] -fileset [get_property constrset [current_run]]
link_design
read_xdl -file "C:/Users/jdub/Documents/FPGA/test_lx9_take2/openMSP430_fpga.ncd"
if {[catch {read_twx -name results_1 -file "C:/Users/jdub/Documents/FPGA/test_lx9_take2/openMSP430_fpga.twx"} eInfo]} {
   puts "WARNING: there was a problem importing \"C:/Users/jdub/Documents/FPGA/test_lx9_take2/openMSP430_fpga.twx\": $eInfo"
}
