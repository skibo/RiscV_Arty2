
ifdef RISCV
   CROSS_COMPILE=$(RISCV)/bin/riscv64-unknown-elf-
else
   CROSS_COMPILE=riscv64-unknown-elf-
endif

LD=$(CROSS_COMPILE)ld
CC=$(CROSS_COMPILE)gcc
AS=$(CROSS_COMPILE)as
BIN2HEX=/usr/bin/hexdump -v -e '/4 "%08X\n"'

OBJS=vect.o start.o exception.o main.o console.o ping.o ether.o \
	blinky.o monitor.o
ELFS=bootrom.elf
BINS=bootrom_text.bin bootrom_data.bin
LDSCRIPT=ldscript.riscv

AFLAGS=-march=rv32i
CFLAGS=-g -fno-builtin -march=rv32i -mabi=ilp32
LFLAGS=-melf32lriscv -static -T $(LDSCRIPT)
LIBS=$(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

# Optional GDB stub support
# CFLAGS+=-DGDBSTUB
# OBJS+= debug.o

TARGETS=bootrom.mem bootrom.lst

.PHONY: default clean
default: $(TARGETS)

%.o: %.S
	$(AS) $(AFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

bootrom.elf: $(OBJS) $(LDSCRIPT)
	$(LD) $(LFLAGS) -o $@ $(OBJS) $(LIBS)

bootrom_text.bin: bootrom.elf
	$(CROSS_COMPILE)objcopy -j .vects -j .text -j .rodata -O binary $< $@

bootrom_data.bin: bootrom.elf
	$(CROSS_COMPILE)objcopy -j .data -O binary $< $@

bootrom.mem: bootrom_text.bin bootrom_data.bin
	echo "@00000000" > $@
	$(BIN2HEX) bootrom_text.bin >> $@
	$(BIN2HEX) bootrom_data.bin >> $@

%.lst: %.elf
	$(CROSS_COMPILE)objdump --disassemble-all $< > $@

clean:
	rm -f $(TARGETS) $(OBJS) $(BINS) $(ELFS)

