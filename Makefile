PREFIX     = arm-none-eabi
GCC        = $(PREFIX)-gcc
OBJCOPY    = $(PREFIX)-objcopy
OBJDUMP    = $(PREFIX)-objdump
CFLAGS     = -O2 -march=armv7-a -mtune=cortex-a7 -mcpu=cortex-a7
HEADER     = *.h
SOURCE     = boot.s *.c
LINKER     = memorymap.lnk

kernel7.img: $(SOURCE) $(HEADER)
	$(GCC) $(CFLAGS) -c $(SOURCE)
	$(GCC) $(CFLAGS) -T $(LINKER) -Wl,-Map=memory.map -o kernel.elf -nostdlib -nostartfiles *.o
	$(OBJCOPY) kernel.elf -O binary kernel7.img
	$(OBJDUMP) -D kernel.elf 2>&1 | tee asmlist.txt

clean:
	rm *.o *.elf *.txt
