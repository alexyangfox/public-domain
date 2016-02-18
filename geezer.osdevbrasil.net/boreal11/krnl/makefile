include ../common1.mk

# DEFINES
# source files; for automatic dependency checking
ASRC	:= $(wildcard *.s)
CSRC	:= $(wildcard *.c)
# kernel object files
KOBJS	:= $(ASRC:.s=.o) $(CSRC:.c=.o) ../apps/ustart.o ../apps/tetris.o $(LIBK)
# kernel linker script
KSCRIPT	:= krnl.ld

# TARGETS
all : krnl.x ../apps/tetris.r

../apps/tetris.r :
	make -C ../apps tetris.r

install : krnl.x ../apps/tetris.r
	$(call INSTALL, krnl.x ../apps/tetris.r)

# IMPLICIT RULES

# DEPENDENCIES

# EXPLICIT RULES

include ../common2.mk
