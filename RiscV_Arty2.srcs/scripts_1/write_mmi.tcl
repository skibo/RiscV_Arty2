# write_mmi.tcl
#
# by thomas@skibo.net
# Heavily based upon a script of the same name by stephenm@xilinx.com,
# see https://www.xilinx.com/support/answers/63041.html

# write_mmi [<project-name>]

# This script creates an mmi file from a completed design.  It also creates
# a shell script to invoke updatemem (updaterom.sh) which you can run
# to update the .bit file after making software changes to the bootrom.

proc write_lane {fileout rom msb lsb lane_depth} {
    set bram_loc [lindex [split [get_property LOC [get_cell $rom]] "_"] 1]

    puts "rom: [lindex [split $rom "/"] end]\tLOC=$bram_loc\t\[$msb : $lsb\]"

    puts $fileout "        <BitLane MemType=\"RAMB32\" Placement=\"$bram_loc\">"
    puts $fileout "          <DataWidth MSB=\"$msb\" LSB=\"$lsb\"/>"
    puts $fileout "          <AddressRange Begin=\"0\" End=\"[expr {$lane_depth - 1}]\"/>"
    puts $fileout "          <Parity ON=\"false\" NumBits=\"0\"/>"
    puts $fileout "        </BitLane>"
}

proc write_mmi {filename} {
    set inst_path [get_cells -hierarchical *riscv_too_mem_0]
    set roms [get_cells -hierarchical -regexp -filter \
	    { PRIMITIVE_TYPE =~  BMEM.bram.* && NAME =~ .*riscv_too_mem_0.*}]

    set nroms [llength $roms]
    set size [expr {4096 * $nroms}]
    set lane_width [expr {32 / $nroms}]
    set lane_depth [expr {32768 / $lane_width}]
    set fileout [open $filename "w"]

    puts $fileout "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    puts $fileout "<MemInfo Version=\"1\" Minor=\"0\">"

    puts $fileout "  <Processor Endianness=\"Little\" InstPath=\"riscv\">"
    puts $fileout "    <AddressSpace Name=\"$inst_path\" Begin=\"0\" End=\"[expr {$size - 1}]\">"
    puts $fileout "      <BusBlock>"

    # Note: it seems that updatemem ignores the msb/lsb designations
    # and just assumes everything is ordered from high-order bits to
    # low-order bits.

    set msb 31
    if {$nroms == 4} {
	# 16K, 4 BRAMS, lane width 8 bits
	for {set i 3} {$i >= 0} {incr i -1} {
	    set rom [lindex $roms [lsearch $roms "*_$i"]]
	    write_lane $fileout $rom $msb [expr {$msb - $lane_width + 1}] $lane_depth
	    set msb [expr {$msb - $lane_width}]
	}
    } elseif {$nroms == 8} {
	# 32K, 8 BRAMS, lane width 4 bits
	for {set i 3} {$i >= 0} {incr i -1} {
	    for {set j 1} {$j >= 0} {incr j -1} {
		set rom [lindex $roms [lsearch $roms "*_$i\_$j"]]
		write_lane $fileout $rom $msb [expr {$msb - $lane_width + 1}] $lane_depth
		set msb [expr {$msb - $lane_width}]
	    }
	}
    } elseif {$nroms == 16} {
	# 64K, 16 BRAMS, lane width 2 bits
	for {set i 3} {$i >= 0} {incr i -1} {
	    for {set j 3} {$j >= 0} {incr j -1} {
		set rom [lindex $roms [lsearch $roms "*_$i\_$j"]]
		write_lane $fileout $rom $msb [expr {$msb - $lane_width + 1}] $lane_depth
		set msb [expr {$msb - $lane_width}]
	    }
	}
    } elseif {$nroms == 32} {
	# 128K, 32 BRAMS, lane width 1 bit
	for {set i 3} {$i >= 0} {incr i -1} {
	    for {set j 7} {$j >= 0} {incr j -1} {
		    set rom [lindex $roms [lsearch $roms "*_$i\_0\_$j"]]
		    write_lane $fileout $rom $msb [expr {$msb - $lane_width + 1}] $lane_depth
		set msb [expr {$msb - $lane_width}]
	    }
	}
    } else {
	# error!
	puts "ERROR: Unsupported length or other error!"
	puts "nroms = $nroms roms: $roms"
	exit 1
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
    set bootrom_data "[file normalize [get_property DIRECTORY [current_project]]/../[get_property NAME [current_project]].srcs/sw_1/bootrom/bootrom.mem]"
    set bit_file "[get_property DIRECTORY [current_run]]/[get_property TOP [current_design]].bit"
    set fileout [open "updaterom.sh" "w" 0755]

    puts $fileout "#!/bin/sh\n#"
    puts $fileout "# Run this script to update bistream with updated firmware.\n"
    puts $fileout "updatemem --force \\\n   --meminfo $mmi_filename \\"
    puts $fileout "   --data $bootrom_data \\\n   --bit $bit_file \\"
    puts $fileout "   -proc riscv \\\n   --out $bit_file"

    close $fileout
}

# If called from Makefile, we have to open project and implementation.
if {[llength $argv] > 0} {
    puts "Opening project [lindex $argv 0]"
    open_project [lindex $argv 0]
    open_run impl_1
    puts "Finished opening project"
}

set mmi_filename "[get_property DIRECTORY [current_run]]/bootrom.mmi"
write_mmi $mmi_filename
protocmd $mmi_filename
