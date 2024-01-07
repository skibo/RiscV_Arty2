#
# Makefile for RiscV_Arty2 project
#

# Name of Vivado project.
PROJNAME=RiscV_Arty2
SRCDIR=$(PROJNAME).srcs
SCRIPTDIR=$(SRCDIR)/scripts_1

RTLTOP=design_1_wrapper

SWSRCS= \
	$(SRCDIR)/sw_1/bootrom/ldscript.riscv	\
	$(SRCDIR)/sw_1/common/types.h		\
	$(SRCDIR)/sw_1/common/io.h		\
	$(SRCDIR)/sw_1/common/console.h		\
	$(SRCDIR)/sw_1/common/console.c		\
	$(SRCDIR)/sw_1/bootrom/exception.c	\
	$(SRCDIR)/sw_1/bootrom/main.c		\
	$(SRCDIR)/sw_1/bootrom/vect.S		\
	$(SRCDIR)/sw_1/bootrom/start.S

MEMFILES= \
	$(SRCDIR)/sw_1/bootrom/bootrom.mem

ifndef XILINX_VIVADO
$(error XILINX_VIVADO must be set to point to Xilinx tools)
endif

VIVADO=$(XILINX_VIVADO)/bin/vivado

.PHONY: default project impl bitstream program mmi

default: project

PROJFILE=$(PROJNAME)/$(PROJNAME).xpr

project: $(PROJFILE)

$(PROJFILE): $(MEMFILES)
ifeq ("","$(wildcard $(PROJFILE))")
	$(VIVADO) -mode batch -source project.tcl
else
	@echo Project already exists.
endif

IMPLEMENTATION=$(PROJNAME)/$(PROJNAME).runs/impl_1/$(RTLTOP)_routed.dcp
BITSTREAM=$(PROJNAME)/$(PROJNAME).runs/impl_1/$(RTLTOP).bit

impl: $(IMPLEMENTATION)
$(IMPLEMENTATION):
	@echo Building implementation from rtl sources.
	$(VIVADO) -mode batch -source $(SCRIPTDIR)/implement.tcl \
		-tclargs $(PROJNAME)

bitstream: $(BITSTREAM)

$(BITSTREAM): $(IMPLEMENTATION)
	@echo Generating new $(BITSTREAM)
	$(VIVADO) -mode batch -source $(SCRIPTDIR)/bitstream.tcl \
		-tclargs $(PROJNAME)

$(MEMFILES): $(SWSRCS)
	(cd $(SRCDIR)/sw_1/bootrom ; $(MAKE))

program: $(BITSTREAM)
	@echo Programming Arty
	djtgcfg prog -d Arty -i 0 -f $(BITSTREAM)

# You can create an MMI file which can be used ith updatemem to load
# a new binary into the bootrom without resynthesizing.
MMI_FILE=$(PROJNAME)/$(PROJNAME).runs/impl_1/bootrom.mmi
mmi: $(MMI_FILE)

$(MMI_FILE): $(IMPLEMENTATION)
	$(VIVADO) -mode batch -source $(SCRIPTDIR)/write_mmi.tcl \
		-tclargs $(PROJFILE)
