# defines
# MAKEDEP	=bc55.mak
INCDIR	=..\inc
CC	=bcc32 -c -O2 -v -w -I$(INCDIR)
AS	=nasm -f obj -dUNDERBARS=1 -i$(INCDIR)/
LD	=ilink32 -C -Gn -c -s -v -b:0x3FE000 -Af:4096 -Ao:4096
LIBC	=..\lib\libc.lib

# targets
all : tetris.x invade.x clock.x hello.x echo.x modem.x
#protect.x

clean :
	deltree /y *.o *.x

# implicit rules
.c.o:
	$(CC) -o$@ $<

.asm.o :
	$(AS) -o$@ $<

# dependencies
ustart.o : ustart.asm $(MAKEDEP)

tetris.o : tetris.c $(MAKEDEP)

invade.o : invade.c $(MAKEDEP)

clock.o : clock.c $(MAKEDEP)

hello.o : hello.c $(MAKEDEP)

echo.o : echo.c $(MAKEDEP)

protect.o : protect.c $(MAKEDEP)

modem.o : modem.c $(MAKEDEP)

# EXPLICIT RULES
tetris.x : ustart.o tetris.o $(LIBC)
	$(LD) ustart.o tetris.o,$@,,$(LIBC)
	del $*.tds

invade.x : ustart.o invade.o $(LIBC)
	$(LD) ustart.o invade.o,$@,,$(LIBC)
	del $*.tds

clock.x : ustart.o clock.o $(LIBC)
	$(LD) ustart.o clock.o,$@,,$(LIBC)
	del $*.tds

hello.x : ustart.o hello.o $(LIBC)
	$(LD) ustart.o hello.o,$@,,$(LIBC)
	del $*.tds

echo.x : ustart.o echo.o $(LIBC)
	$(LD) ustart.o echo.o,$@,,$(LIBC)
	del $*.tds

protect.x : ustart.o protect.o $(LIBC)
	$(LD) ustart.o protect.o,$@,,$(LIBC)
	del $*.tds

modem.x : ustart.o modem.o $(LIBC)
	$(LD) ustart.o modem.o,$@,,$(LIBC)
	del $*.tds
