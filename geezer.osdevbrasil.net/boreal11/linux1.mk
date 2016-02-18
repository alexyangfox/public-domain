.SUFFIXES:
.SUFFIXES: .d .c .s .asm .o .x .r

# DEFINES
MAKEFILE:= makefile
MAKEDEP	:= $(MAKEFILE)
INCDIR	:= ../inc
USCRIPT	:= user.ld
LIBK	:= ../lib/libk.a
LIBC	:= ../lib/libc.a



# Installs file(s) onto floppy disk
define INSTALL
	-umount ../mnt
	pwd
	mount /dev/fd0 ../mnt
	for F in $(1) ; do cp -f $$F ../mnt ; done
endef

# How to link relocatable task
# Called like this:	$(call LDRT, tetris, ustart.o tetris.o $(LIBK))
define LDRT
	ld -g -d -r -o $(1).r $(2)
	objdump --source --reloc $(1).r >$(1).lst
	nm -n $(1).r >$(1).sym
	strip --strip-unneeded $(1).r
endef



# How to link executable task
# Called like this:	$(call LDT, tetris, ustart.o tetris.o $(LIBC))
define LDT
	ld -g -T $(USCRIPT) -o $(1).x $(2)
	objdump --source $(1).x >$(1).lst
	nm -n $(1).x >$(1).sym
	strip $(1).x
endef



# TARGETS

# IMPLICIT RULES

# DEPENDENCIES

# EXPLICIT RULES
