# defines
# MAKEDEP	=bc55.mak
INCDIR	=..\inc
CC	=bcc32 -c -O2 -v -w -I$(INCDIR)
AS	=nasm -f obj -dUNDERBARS=1 -i$(INCDIR)/
LIB	=tlib /C
KOBJS	=PFXctype\ctype.o						\
	\
	PFXstdio\vsprintf.o PFXstdio\fputs.o PFXstdio\sprintf.o	\
	PFXstdio\setbuf.o PFXstdio\printf.o PFXstdio\vprintf.o		\
	PFXstdio\stdio.o PFXstdio\fflush.o PFXstdio\fputc.o		\
	PFXstdio\doprintf.o						\
	\
	PFXstdlib\srand.o PFXstdlib\exit.o PFXstdlib\rand.o		\
	\
	PFXstring\memcpy.o PFXstring\strlen.o PFXstring\memset.o	\
	PFXstring\strcmp.o PFXstring\strncmp.o PFXstring\memcmp.o	\
	PFXstring\memsetw.o PFXstring\memmove.o PFXstring\strstr.o	\
	\
	PFXnasm\setjmp\setjmp.o PFXnasm\setjmp\longjmp.o		\
	\
	PFXnasm\system\inportb.o PFXnasm\system\outportb.o		\
	PFXnasm\system\enable.o PFXnasm\system\disable.o		\
	PFXnasm\system\res_flag.o
COBJS	=$(KOBJS) PFXnasm\os\open.o PFXnasm\os\close.o PFXnasm\os\write.o \
	PFXnasm\os\read.o PFXnasm\os\select.o PFXnasm\os\time.o		\
	PFXnasm\os\_exit.o

# targets
all : libk.lib libc.lib

clean :
	deltree /y *.a *.lib
	dir /s /b *.d | sed -e s/"^"/"del "/g >delobj.bat
	dir /s /b *.o | sed -e s/"^"/"del "/g >>delobj.bat
	delobj.bat
	del delobj.bat

# implicit rules
.c.o:
	$(CC) -o$@ $<

.asm.o :
	$(AS) -o$@ $<

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

nasm\setjmp\longjmp.o : nasm\setjmp\longjmp.asm $(MAKEDEP)

nasm\system\inportb.o : nasm\system\inportb.asm $(MAKEDEP)

nasm\system\outportb.o : nasm\system\outportb.asm $(MAKEDEP)

nasm\system\enable.o : nasm\system\enable.asm $(MAKEDEP)

nasm\system\disable.o : nasm\system\disable.asm $(MAKEDEP)

nasm\system\res_flag.o : nasm\system\res_flag.asm $(MAKEDEP)

nasm\os\open.o : nasm\os\open.c $(MAKEDEP)

nasm\os\close.o : nasm\os\close.c $(MAKEDEP)

nasm\os\write.o : nasm\os\write.c $(MAKEDEP)

nasm\os\read.o : nasm\os\read.c $(MAKEDEP)

nasm\os\select.o : nasm\os\select.c $(MAKEDEP)

nasm\os\time.o : nasm\os\time.c $(MAKEDEP)

nasm\os\_exit.o : nasm\os\_exit.c $(MAKEDEP)

# explicit rules
libk.lib : $(KOBJS:PFX=) $(MAKEDEP)
	-del $@
	$(LIB) $@ $(KOBJS:PFX=+)

libc.lib : $(COBJS:PFX=) $(MAKEDEP)
	-del $@
	$(LIB) $@ $(COBJS:PFX=+)

