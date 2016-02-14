# Set our GCC compiler
CC=i586-elf-gcc
# Set our CC flags
#	Wall		- Enable all warnings
#	Werror		- Exit at first error
#	pedantic	- Warn verbosely
#	pedantic-errors	- pedantic throws errors
#	nostdlib	- No C standard stuff (library, runtime etc)
#	nostartfiles	- With above
#	nodefaultlibs	- With above
#	fno-leading-underscore	- Symbols like 'main', not '_main'
#	ffreestanding	- We're freestanding - eliminates some irrelevant
#			  warnings and errors, etc
#	std=gnu99	- I'm used to C99 compliant code, but need GCC some of
#			  the GCC extensions (like inline ASM)
#	-I./include	- Add our header-include directory
# Some of these may seem irritating, but the code is better for it
CFLAGS=-Wall -Werror -pedantic -pedantic-errors -nostdlib -nostartfiles -nodefaultlibs -fno-leading-underscore -ffreestanding -std=gnu99 -I./include
# Set our compiling command
COMPILE=$(CC) -c $@.c $(CFLAGS)
# Set our assembler
ASM=nasm
# Set our NASM flags
AFLAGS=-f elf
# Set our object file list
SRCS=loader.o kernel.o descriptors.o interrupts.o irqs.o textfunctions.o textmode.o
# Set our linker script
LINK=linker.ld
# Set our linker flags
LFLAGS=-T $(LINK)
# Set our kernel output file
OUT=kernel.bin

bootdisk:	all
		dd if=floppy.img of=/dev/fd0

image:		install
		dd if=/dev/fd0 of=floppy.img

install:	all
		mount -t ext2 /dev/fd0 /mnt/floppy
		cp *.bin /mnt/floppy/boot
		umount /dev/fd0
		
all:	loader kernel descriptors interrupts irqs textfunctions textmode
	ld $(SRCS) $(LFLAGS) -o $(OUT)

loader:	loader.asm
	$(ASM) $(AFLAGS) $@.asm

kernel:	kernel.c
	$(COMPILE)
	
descriptors:	descriptors.c
		$(COMPILE)

interrupts:	interrupts.c
		$(COMPILE)

irqs:		irqs.c
		$(COMPILE)

textfunctions:	textfunctions.c
		$(COMPILE)

textmode:	textmode.c
		$(COMPILE)
		
tidy:	
	rm -f *.o *~

clean:	
	rm -f *.o *.bin *~ parport.out bochsout.txt
