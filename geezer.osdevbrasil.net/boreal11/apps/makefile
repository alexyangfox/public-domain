include ../common1.mk

# DEFINES
# source files; for automatic dependency checking
ASRC	:= $(wildcard *.s)
CSRC	:= $(wildcard *.c)

# TARGETS
all : tetris.r tetris.x invade.x clock.x hello.x echo.x protect.x	\
	terminal.x slip.x

install : tetris.x invade.x clock.x hello.x echo.x protect.x terminal.x
	$(call INSTALL, tetris.x invade.x clock.x hello.x echo.x protect.x terminal.x)

# IMPLICIT RULES

# DEPENDENCIES

# EXPLICIT RULES
tetris.r : ustart.o tetris.o $(LIBK)
	$(call LDRT, tetris, ustart.o tetris.o $(LIBK))

tetris.x : ustart.o tetris.o $(LIBC) $(USCRIPT)
	$(call LDT, tetris, ustart.o tetris.o $(LIBC))

invade.x : ustart.o invade.o $(LIBC) $(USCRIPT)
	$(call LDT, invade, ustart.o invade.o $(LIBC))

clock.x : ustart.o clock.o $(LIBC) $(USCRIPT)
	$(call LDT, clock, ustart.o clock.o $(LIBC))

hello.x : ustart.o hello.o $(LIBC) $(USCRIPT)
	$(call LDT, hello, ustart.o hello.o $(LIBC))

echo.x : ustart.o echo.o $(LIBC) $(USCRIPT)
	$(call LDT, echo, ustart.o echo.o $(LIBC))

protect.x : ustart.o protect.o $(LIBC) $(USCRIPT)
	$(call LDT, protect, ustart.o protect.o $(LIBC))

terminal.x : ustart.o terminal.o $(LIBC) $(USCRIPT)
	$(call LDT, terminal, ustart.o terminal.o $(LIBC))

slip.x : ustart.o slip.o $(LIBC) $(USCRIPT)
	$(call LDT, slip, ustart.o slip.o $(LIBC))

include ../common2.mk
