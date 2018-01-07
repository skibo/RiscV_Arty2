
# by thomas@skibo.net
# Heavily based upon a script of the same name by stephenm@xilinx.com

# This script creates an mmi file from a completed design.  It also creates
# a prototype invocation of updatemem (updatemem.txt) which you can source
# to update the .bit file after making software changes to the bootrom.

proc write_mmi {filename size width} {
    set inst_path [get_cells -hierarchical *riscv_too_mem_0]
    set roms [get_cells -hierarchical -regexp -filter \
            { PRIMITIVE_TYPE =~  BMEM.bram.* && NAME =~ .*riscv_too_mem_0.*}]
    
    set bram_type "RAMB32"
    set lane_width [expr {$width / [llength $roms]}]
    set lane_depth [expr {32768 / $lane_width}]
    set fileout [open $filename "w"]

    puts $fileout "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    puts $fileout "<MemInfo Version=\"1\" Minor=\"0\">"

    puts $fileout "  <Processor Endianness=\"Little\" InstPath=\"riscv\">"
    puts $fileout "    <AddressSpace Name=\"$inst_path\" Begin=\"0\" End=\"[expr {$size - 1}]\">"
    puts $fileout "      <BusBlock>"

    for {set i [expr {[llength $roms] - 1}]} {$i >= 0} {incr i -1} {
        set rom [lindex $roms [lsearch $roms "*_$i"]]

        set bram_loc [lindex [split [get_property LOC [get_cell $rom]] "_"] 1]
        set lsb [expr {$i * $lane_width}]
        set msb [expr {$lane_width + $lsb - 1}]
        puts $fileout "        <BitLane MemType=\"$bram_type\" Placement=\"$bram_loc\">"
        puts $fileout "          <DataWidth MSB=\"$msb\" LSB=\"$lsb\"/>"
        puts $fileout "          <AddressRange Begin=\"0\" End=\"[expr {$lane_depth - 1}]\"/>"
        puts $fileout "          <Parity ON=\"false\" NumBits=\"0\"/>"
        puts $fileout "        </BitLane>"
    }
    
    puts $fileout "      </BusBlock>"
    puts $fileout "    </AddressSpace>"
    puts $fileout "  </Processor>"
    puts $fileout "<Config>"
    puts $fileout "  <Option Name=\"Part\" Val=\"[get_property PART [current_project]]\"/>"
    puts $fileout "</Config>"
    puts $fileout "</MemInfo>"
    close $fileout
    puts "MMI file ($filename) created successfully."
}

proc protocmd {mmi_filename} {
    set bootrom_data "[get_property DIRECTORY [current_project]]/../[get_property NAME [current_project]].srcs/sw_1/bootrom/bootrom.mem"
    set bit_file "[get_property DIRECTORY [current_run]]/[get_property TOP [current_design]].bit"
    set filename "[get_property DIRECTORY [current_run]]/updatemem.txt"
    set fileout [open $filename "w"]

    puts $fileout "updatemem -force -meminfo $mmi_filename -data $bootrom_data -bit $bit_file -proc riscv -out $bit_file"

    close $fileout
}

# If called from Makefile, we have to open project and implementation.
if {[llength $argv] > 0} {
    open_project [lindex $argv 0]
    open_run impl_1
}

set mmi_filename "[get_property DIRECTORY [current_run]]/bootrom.mmi"
write_mmi $mmi_filename 8192 32
protocmd $mmi_filename
