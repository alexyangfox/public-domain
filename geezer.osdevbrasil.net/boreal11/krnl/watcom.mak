.EXTENSIONS:
.EXTENSIONS: .o .c
# defines
# MAKEDEP	=watcom.mak
CC	=wcc386 -zq -3 -s -d2 -hw -ox -w=9 -zc -mf -fr=nul -i=..\inc
LD      =wlink OP q OP nod D w a FORM WIndows NT &
	OP off=0xC0000000 OP start=kentry
OBJS    =PFXaspace.o PFXcoff.o PFXcondev.o PFXdebug.o PFXelf.o PFXkbd.o &
        PFXmain.o PFXmm.o PFXpecoff.o PFXserdev.o PFXserial.o PFXsyscalls.o &
	PFXtasks.o PFXthreads.o PFXtime.o PFXvideo.o PFXzerodev.o
LIB	=..\lib\libk.lib

# targets
all : krnl.x

clean : .SYMBOLIC
	deltree /y *.d *.o krnl.x krnl.lst krnl.sym

# implicit rules
.c.o :
	$(CC) -fo=$@ $[@

# dependencies
aspace.o : aspace.c _krnl.h $(MAKEDEP)

coff.o : coff.c _krnl.h $(MAKEDEP)

condev.o : condev.c _krnl.h $(MAKEDEP)

debug.o : debug.c _krnl.h $(MAKEDEP)

elf.o : elf.c _krnl.h $(MAKEDEP)

kbd.o : kbd.c _krnl.h $(MAKEDEP)

main.o : main.c _krnl.h $(MAKEDEP)

mm.o : mm.c _krnl.h $(MAKEDEP)

pecoff.o : pecoff.c _krnl.h $(MAKEDEP)

serdev.o : serdev.c _krnl.h $(MAKEDEP)

serial.o : serial.c _krnl.h $(MAKEDEP)

syscalls.o : syscalls.c _krnl.h $(MAKEDEP)

tasks.o : tasks.c _krnl.h $(MAKEDEP)

threads.o : threads.c _krnl.h $(MAKEDEP)

time.o : time.c _krnl.h $(MAKEDEP)

video.o : video.c _krnl.h $(MAKEDEP)

zerodev.o : zerodev.c _krnl.h $(MAKEDEP)

# explicit rules
$(LIB) :
	cd ..\lib
	wmake /f watcom.mak
	cd ..\krnl

krnl.x : $(OBJS:PFX=) $(LIB) $(MAKEDEP)
	*$(LD) N $@ $(OBJS:PFX=F ) L $(LIB)
