
BCDIR = c:\bc45

# memory model, most common are 's' (small) and 'l' (large)
MEMMOD = l

# compiler options
COPT= -3 -c -wnod -wpro -wuse -C -m$(MEMMOD)

# linker options
LOPT= /m

example2.exe: example2.obj \
              ataioint.obj ataioisa.obj ataiopci.obj ataiopio.obj \
              ataioreg.obj ataiosub.obj ataiotmr.obj ataiotrc.obj
   tlink $(LOPT) @&&!
$(BCDIR)\lib\c0$(MEMMOD) +
example2 +
ataioint +
ataioisa +
ataiopci +
ataiopio +
ataioreg +
ataiosub +
ataiotmr +
ataiotrc
example2
nul
$(BCDIR)\lib\emu +
$(BCDIR)\lib\math$(MEMMOD) +
$(BCDIR)\lib\c$(MEMMOD)
!

example2.obj: example2.c ataio.h
   bcc $(COPT) example2.c

ataioint.obj: ataioint.c  ataio.h
   bcc -S $(COPT) ataioint.c
   tasm ataioint.asm

ataioisa.obj: ataioisa.c  ataio.h
   bcc $(COPT) ataioisa.c

ataiopci.obj: ataiopci.c  ataio.h
   bcc $(COPT) ataiopci.c

ataiopio.obj: ataiopio.c  ataio.h
   bcc -S $(COPT) ataiopio.c
   tasm ataiopio.asm

ataioreg.obj: ataioreg.c  ataio.h
   bcc $(COPT) ataioreg.c

ataiosub.obj: ataiosub.c  ataio.h
   bcc $(COPT) ataiosub.c

ataiotmr.obj: ataiotmr.c  ataio.h
   bcc $(COPT) ataiotmr.c

ataiotrc.obj: ataiotrc.c  ataio.h
   bcc $(COPT) ataiotrc.c

