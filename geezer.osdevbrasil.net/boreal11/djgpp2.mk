# DEFINES
DEPS	:= $(ASRC:.s=.d) $(CSRC:.c=.d)

# SED command to prepend the dependency file itself to the dependency,
# i.e. convert
#            tasks.o: tasks.c _krnl.h ../inc/setjmp.h
# to
#    tasks.d tasks.o: tasks.c _krnl.h ../inc/setjmp.h
#
FIXDEP	:= sed -e s/^\(.*\)\.o/"\1.d \1.o"/g

# TARGETS
# delete intermediate and output files in current directory (kernel)
clean :
	deltree /y *.d *.o *.r *.x *.sym *.lst *.map

# delete intermediate and output files in and under current directory (libc)
rclean :
	deltree /y *.a *.lib
	dir /s /b *.d | sed -e s/"^"/"del "/g >delobj.bat
	dir /s /b *.o | sed -e s/"^"/"del "/g >>delobj.bat
	-delobj.bat
	del delobj.bat

dep : $(DEPS)
	@echo Updated dependencies

$(LIBK) : ../lib/$(MAKEFILE)
	make -C ../lib -f $(MAKEFILE) libk.a

$(LIBC) : ../lib/$(MAKEFILE)
	make -C ../lib -f $(MAKEFILE) libc.a

# IMPLICIT RULES
# how to create Make dependency rule from .s (AS) files
# DOS grabs the filename after >, creating files named "*$.o", "$@", etc.
# so use 'temp1.tmp' and 'temp2.tmp' as temporary files
%.d : %.s
	as -I$(INCDIR) --defsym UNDERBARS=1 -MD temp1.tmp -o$*.o $<
	$(FIXDEP) <temp1.tmp >temp2.tmp
	$(call MV,temp2.tmp,$@)
	del temp1.tmp

# how to create Make dependency rule from .asm (NASM) files
# Requires NASM 0.98. For reference only -- this code no longer uses NASM
#%.d : %.asm
#	nasm -M -i$(INCDIR)/ -o$*.o $< | $(FIXDEP) >temp1.tmp
#	$(call MV,temp1.tmp,$@)

# how to create Make dependency rule from .c files
# Use "gcc -MM ..." to exclude system header files (#include <blah.h>)
%.d : %.c
	gcc -M -nostdinc -I$(INCDIR) $< | $(FIXDEP) >temp1.tmp
	$(call MV,temp1.tmp,$@)

# how to assemble *.s files with AS
# Use "--defsym UNDERBARS=1" for DOS and Windows systems. (Or add
# "-fno-leading-underscore" to CC macro, below, if your GCC supports it.)
%.o : %.s
	as -I$(INCDIR) --defsym UNDERBARS=1 -o$@ $<

# how to assemble *.asm files with NASM
# Use "-f coff" for DJGPP, "-f win32" for MinGW, "-f elf" for Linux GCC
#%.o : %.asm
#	nasm -f coff -i$(INCDIR)/ -dUNDERBARS=1 -o$@ $<

# how to compile
%.o : %.c
	gcc -c -g -O2 -Wall -W -nostdinc -I$(INCDIR) -fno-builtin\
	-march=i386 -o$@ $<

# DEPENDENCIES
ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

# EXPLICIT RULES
krnl.x : $(KOBJS) $(KSCRIPT)
	ld -g -T $(KSCRIPT) -okrnl.x $(KOBJS)
	objdump --source krnl.x >krnl.lst
	nm -n krnl.x >krnl.sym
	strip krnl.x
