# Usage
#
# make                - make everything, kernel, init, format, assign ...
# make depend         - update all dependencies
# make clean          - remove all 
# make disk           - copy all files to disk image with mtools
#                       (ensure directories c:/bin, c:/etc c:/home exist)
#
# Can also update specific parts like this...
# 
# make kernel         - make the kernel
# make kernel/depend  - update kernel dependencies
# make kernel/clean   - remove kernel exe and object files
# make kernel/disk    - Copy kernel to disk image with mtools
#
#
# FIXME: Makedepend uses the default compiler include path when
# including stdarg.h, which is different to the Kielder-elf cross compiler one.
# May need to modify Makedepend source and recompile to use a compiler
# other than the default GCC.
#
# FIXME: Removed -D_KIELDEROS from CFLAGS, was used by ACPI acenv.h header

# ----------------------------------------------------------------------------

LINK:= i386-kielder-ld
CC:= i386-kielder-gcc
AS:=i386-kielder-as
STRIP:= i386-kielder-strip
OBJDUMP:= i386-kielder-objdump
ARCHIVE:= i386-kielder-ar
OFLAGS:=
AFLAGS:= -r
CFLAGS:= -I./h -I./h/acpi -fno-common -Wall -O2 
LFLAGS:= -v
DFLAGS:= -I./h -I./h/acpi
DEPEND:= makedepend 
APPLINK:= i386-kielder-gcc
APPLDFLAGS:= 


%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.o : %.s
	$(AS) $< -o $@

%.o : %.S
	$(CC) -c $(CFLAGS) $< -o $@


# ----------------------------------------------------------------------------

.PHONY : everything
.PHONY : all
.PHONY : depend
.PHONY : clean
.PHONY : disk
.PHONY : dump

# ----------------------------------------------------------------------------
# default rule when calling make,  placed before any makefiles are included.

everything : all

# ----------------------------------------------------------------------------
# Include subdirectory makefiles here

include kernel/makefile.in
include bin/makefile.in
include boot/makefile.in
include etc/makefile.in
include home/makefile.in
include devs/makefile.in


# ----------------------------------------------------------------------------
# add targets for "make all" here.
        
all: kernel bin boot etc home devs


# ----------------------------------------------------------------------------


disk: kernel/disk bin/disk boot/disk etc/disk home/disk devs/disk


# ----------------------------------------------------------------------------

dump: kernel/dump 


# ----------------------------------------------------------------------------


clean: kernel/clean bin/clean boot/clean devs/clean
	
	
# ----------------------------------------------------------------------------


depend: kernel/depend bin/depend boot/depend devs/depend


# ----------------------------------------------------------------------------

# DO NOT DELETE
