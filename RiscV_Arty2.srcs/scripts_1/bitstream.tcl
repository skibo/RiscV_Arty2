#
# bitstream.tcl
#
set project_name [lindex $argv 0]
set origin_proj_dir [file normalize ./$project_name]

open_project $origin_proj_dir/$project_name.xpr

reset_run impl_1 -from_step write_bitstream

launch_runs impl_1 -to_step write_bitstream
wait_on_run impl_1
