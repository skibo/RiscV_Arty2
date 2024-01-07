
ifdef RISCV
   CROSS_COMPILE=$(RISCV)/bin/riscv64-unknown-elf-
else
   CROSS_COMPILE=riscv64-unknown-elf-
endif

LD=$(CROSS_COMPILE)ld
CC=$(CROSS_COMPILE)gcc
AS=$(CROSS_COMPILE)as
BIN2HEX=/usr/bin/hexdump -v -e '/4 "%08X\n"'

LDSCRIPT=ldscript.riscv

COMMON=../common

AFLAGS=-I$(COMMON) -march=rv32i
CFLAGS=-g -I$(COMMON) -fno-builtin -march=rv32i -mabi=ilp32
LFLAGS=-melf32lriscv -static -T $(LDSCRIPT)

LIBS=$(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

TARGETS=helloworld.srec pingtest.srec

.PHONY: default clean
default: $(TARGETS)

%.o: %.S
	$(AS) $(AFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.o: $(COMMON)/%.c
	$(CC) $(CFLAGS) -c $<

helloworld.elf: helloworld.o console.o start.o
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS)

pingtest.elf: pingtest.o ether.o console.o start.o
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS)

%.srec: %.elf
	$(CROSS_COMPILE)objcopy -O srec $< $@

%.lst: %.elf
	$(CROSS_COMPILE)objdump --disassemble $< > $@

clean:
	rm -f $(TARGETS) *.o *.elf *.lst

