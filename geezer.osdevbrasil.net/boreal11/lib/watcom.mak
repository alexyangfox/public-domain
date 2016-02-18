.EXTENSIONS:
.EXTENSIONS: .o .c .asm

# defines
# MAKEDEP	=watcom.mak
INCDIR	=..\inc
CC	=wcc386 -zq -3 -s -d2 -hw -ox -w=9 -zc -mf -fr=nul -i=$(INCDIR)
AS	=nasm -f obj -i$(INCDIR)/ -d__WATCOMC__=1
LIB	=wlib -b -c -n
KOBJS	=PFXctype\ctype.o PFXstdio\vsprintf.o PFXstdio\fputs.o&
	PFXstdio\sprintf.o PFXstdio\setbuf.o PFXstdio\printf.o&
	PFXstdio\vprintf.o PFXstdio\stdio.o PFXstdio\fflush.o&
	PFXstdio\fputc.o PFXstdio\doprintf.o PFXstdlib\srand.o&
	PFXstdlib\exit.o PFXstdlib\rand.o PFXstring\memcpy.o&
	PFXstring\strlen.o PFXstring\memset.o PFXstring\strcmp.o&
	PFXstring\strncmp.o PFXstring\memcmp.o PFXstring\memsetw.o&
	PFXstring\memmove.o PFXstring\strstr.o PFXnasm\setjmp\setjmp.o&
	PFXnasm\setjmp\longjmp.o PFXnasm\system\inportb.o&
	PFXnasm\system\outportb.o PFXnasm\system\enable.o&
	PFXnasm\system\disable.o PFXnasm\system\crite.o
COBJS	=$(KOBJS) PFXnasm\os\open.o PFXnasm\os\close.o PFXnasm\os\write.o&
	PFXnasm\os\read.o PFXnasm\os\select.o PFXnasm\os\time.o&
	PFXnasm\os\_exit.o

# targets
all : libk.lib libc.lib

clean : .SYMBOLIC
	deltree /y *.a *.lib
	dir /s /b *.d | sed -e s/"^"/"del "/g >delobj.bat
	dir /s /b *.o | sed -e s/"^"/"del "/g >>delobj.bat
	delobj.bat
	del delobj.bat

# implicit rules

# Shit, man, I can't figure this out. No wonder people use batch files...
#.c.o :
.DEFAULT
	$(CC) -fo=$^@ $[@

.asm.o :
	$(AS) -o$^@ $[@

# dependencies
ctype\ctype.o : ctype\ctype.c $(MAKEDEP)

stdio\vsprintf.o : stdio\vsprintf.c $(MAKEDEP)

stdio\fputs.o : stdio\fputs.c $(MAKEDEP)

stdio\sprintf.o : stdio\sprintf.c $(MAKEDEP)

stdio\setbuf.o : stdio\setbuf.c $(MAKEDEP)

stdio\printf.o : stdio\printf.c $(MAKEDEP)

stdio\vprintf.o : stdio\vprintf.c $(MAKEDEP)

stdio\stdio.o : stdio\stdio.c $(MAKEDEP)

stdio\fflush.o : stdio\fflush.c $(MAKEDEP)

stdio\fputc.o : stdio\fputc.c $(MAKEDEP)

stdio\doprintf.o : stdio\doprintf.c $(MAKEDEP)

stdlib\srand.o : stdlib\srand.c $(MAKEDEP)

stdlib\exit.o : stdlib\exit.c $(MAKEDEP)

stdlib\rand.o : stdlib\rand.c $(MAKEDEP)

string\memcpy.o : string\memcpy.c $(MAKEDEP)

string\strlen.o : string\strlen.c $(MAKEDEP)

string\memset.o : string\memset.c $(MAKEDEP)

string\strcmp.o : string\strcmp.c $(MAKEDEP)

string\strncmp.o : string\strncmp.c $(MAKEDEP)

string\memcmp.o : string\memcmp.c $(MAKEDEP)

string\memsetw.o : string\memsetw.c $(MAKEDEP)

string\memmove.o : string\memmove.c $(MAKEDEP)

string\strstr.o : string\strstr.c $(MAKEDEP)

nasm\setjmp\setjmp.o : nasm\setjmp\setjmp.asm $(MAKEDEP)
	$(AS) -o$^@ $[@

nasm\setjmp\longjmp.o : nasm\setjmp\longjmp.asm $(MAKEDEP)
	$(AS) -o$^@ $[@

nasm\system\inportb.o : nasm\system\inportb.asm $(MAKEDEP)
	$(AS) -o$^@ $[@

nasm\system\outportb.o : nasm\system\outportb.asm $(MAKEDEP)
	$(AS) -o$^@ $[@

nasm\system\enable.o : nasm\system\enable.asm $(MAKEDEP)
	$(AS) -o$^@ $[@

nasm\system\disable.o : nasm\system\disable.asm $(MAKEDEP)
	$(AS) -o$^@ $[@

nasm\system\crite.o : nasm\system\crite.asm $(MAKEDEP)
	$(AS) -o$^@ $[@

nasm\os\open.o : nasm\os\open.asm $(MAKEDEP)
	$(AS) -o$^@ $[@

nasm\os\close.o : nasm\os\close.asm $(MAKEDEP)
	$(AS) -o$^@ $[@

nasm\os\write.o : nasm\os\write.asm $(MAKEDEP)
	$(AS) -o$^@ $[@

nasm\os\read.o : nasm\os\read.asm $(MAKEDEP)
	$(AS) -o$^@ $[@

nasm\os\select.o : nasm\os\select.asm $(MAKEDEP)
	$(AS) -o$^@ $[@

nasm\os\time.o : nasm\os\time.asm $(MAKEDEP)
	$(AS) -o$^@ $[@

nasm\os\_exit.o : nasm\os\_exit.asm $(MAKEDEP)
	$(AS) -o$^@ $[@

# explicit rules
libk.lib : $(KOBJS:PFX=) $(MAKEDEP)
	*$(LIB) $@ $(KOBJS:PFX=+)

libc.lib : $(COJS:PFX=) $(MAKEDEP)
	*$(LIB) $@ $(COBJS:PFX=+)

