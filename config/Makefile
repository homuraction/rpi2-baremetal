PREFIX      = arm-none-eabi
GCC         = $(PREFIX)-gcc
OBJCOPY     = $(PREFIX)-objcopy
OBJDUMP     = $(PREFIX)-objdump
CFLAGS      = -O2 -march=armv7-a -mtune=cortex-a7 -mcpu=cortex-a7
HEADER      = ../*.h
SOURCE      = *.c
LINKER      = memorymap.lnk
OUTPUT_SREC = -O srec --srec-forceS3 --srec-len 32

rominfo.mod: rominfo
	$(OBJCOPY) $(OUTPUT_SREC) $< $@

rominfo: $(SOURCE) $(HEADER)
	$(GCC) $(CFLAGS) -c $(SOURCE)
	$(GCC) $(CFLAGS) -T $(LINKER) -Wl,-Map=memory.map -o rominfo -nostdlib -nostartfiles *.o

clean:
	rm *.o *.elf *.txt
