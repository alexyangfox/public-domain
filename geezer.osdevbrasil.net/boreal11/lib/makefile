include ../common1.mk

# DEFINES
# how to make an archive (library or .a file)
AR	:= ar rcs

ASRC	:= $(wildcard ctype/*.s) $(wildcard stdio/*.s) \
	$(wildcard stdlib/*.s) $(wildcard string/*.s) \
	$(wildcard asm-i386/system/*.s)  $(wildcard asm-i386/setjmp/*.s) \
	$(wildcard asm-i386/string/*s) $(wildcard asm-i386/os/*.s)
CSRC	:= mingw.c $(wildcard ctype/*.c) $(wildcard stdio/*.c) \
	$(wildcard stdlib/*.c) $(wildcard string/*.c) \
	$(wildcard asm-i386/system/*.c) $(wildcard asm-i386/setjmp/*.c) \
	$(wildcard asm-i386/os/*.c)
OBJS	:= $(ASRC:.s=.o) $(CSRC:.c=.o)

COBJS	:= $(ASRC:.s=.o) $(CSRC:.c=.o)
KSRC	:= mingw.c $(wildcard ctype/*.c) $(wildcard stdio/*.c) \
	$(wildcard stdlib/*.c) $(wildcard string/*.c) \
	$(wildcard asm-i386/system/*.c) $(wildcard asm-i386/setjmp/*.c)
KOBJS	:= $(ASRC:.s=.o) $(KSRC:.c=.o)

# TARGETS
all : libk.a libc.a

clean: rclean

# IMPLICIT RULES

# DEPENDENCIES

# EXPLICIT RULES
libk.a : $(KOBJS)
	$(AR) $@ $(KOBJS)

libc.a : $(COBJS)
	$(AR) $@ $(COBJS)

include ../common2.mk
