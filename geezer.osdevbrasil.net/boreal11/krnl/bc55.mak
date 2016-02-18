# defines
# MAKEDEP	=bc55.mak
INCDIR	=..\inc
CC	=bcc32 -c -O2 -v -w -I$(INCDIR)
AS	=nasm -f obj -dUNDERBARS=1 -i$(INCDIR)/
LD	=ilink32 -C -Gn -c -x -v -b:0xBFFFE000 -Af:4096 -Ao:4096
OBJS	=PFXkstart.o PFXaspace.o PFXcoff.o PFXcondev.o PFXdebug.o PFXelf.o \
	PFXkbd.o PFXmain.o PFXmodules.o PFXmm.o PFXpecoff.o PFXserdev.o	\
	PFXserial.o PFXsyscalls.o PFXtasks.o PFXthreads.o PFXtime.o	\
	PFXvideo.o PFXzerodev.o PFXkend.o
#	PFXvideo.o PFXzerodev.o PFXkend.o ..\apps\ustart.o ..\apps\tetris.o

LIB	=..\lib\libk.lib

# targets
all : krnl.x

clean :
	deltree /y *.d *.o krnl.x krnl.lst krnl.sym

# implicit rules
.c.o:
	$(CC) -o$@ $<

.asm.o :
	$(AS) -o$@ $<

# dependencies
kstart.o : kstart.asm $(MAKEDEP)

kend.o : kend.asm $(MAKEDEP)

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
	make -f bc55.mak
	cd ..\krnl

krnl.x : $(OBJS:PFX=) $(LIB) $(MAKEDEP)
	$(LD) $(OBJS:PFX=),$@,,$(LIB)
	del krnl.tds
