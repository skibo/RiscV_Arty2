
ifndef RISCV
$(error RISCV must be set to point to RiscV toolchain)
endif

CROSS_COMPILE=$(RISCV)/bin/riscv64-unknown-elf-
BIN2HEX=/usr/bin/hexdump -v -e '/4 "%08X\n"'

TARGETS=bootrom.mem bootrom.lst

OBJS=vect.o start.o exception.o main.o console.o monitor.o
ELFS=bootrom.elf
BINS=bootrom_text.bin bootrom_data.bin
LDSCRIPT=ldscript.riscv

AFLAGS=-march=rv32i
CFLAGS=-nostdinc -fno-builtin -march=rv32i -mabi=ilp32
LFLAGS=-melf32lriscv -static -T $(LDSCRIPT)

default: $(TARGETS)

%.o: %.S
	$(CROSS_COMPILE)as $(AFLAGS) -o $@ $<

%.o: %.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $<

bootrom.elf: $(OBJS) $(LDSCRIPT)
	$(CROSS_COMPILE)ld $(LFLAGS) -o $@ $(OBJS)

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

