#   PAMAKE makefile by Roundhill Computer Systems Limited   September 1989
#
#   This PAMAKE makefile builds the PAMAKE programs for DOS and OS/2,
#   which are named PAMAKER and PAMAKEP respectively, using Microsoft
#   C version 5.1.  We recommend that you rename these to PAMAKE and 
#   place them in your BIN directories for DOS and OS/2 executables.  
#   You can make a bound version if you wish, but the Microsoft version
#   is quite a bit larger than either of the other versions.  If you 
#   need a bound version which runs under OS/2 and DOS, you will find
#   that Lattice C is *much* better than Microsoft for this.  There's
#   a makefile called MAKEFILE.LC which builds a family mode program.
#   If you only have the Microsoft compiler, you will probably find it
#   preferable, as we did, to build a dual-mode version PAMAKE.EXE by 
#   defining the DOS version as the stub for the OS/2 version.  The EXE
#   file is bigger, but you do not have the Microsoft family-mode 
#   run-time overhead.  This file contains command to build such a 
#   dual-mode version.
#
#   Of course, you will need one of these programs to run this makefile
#   to build them.  This phenomenon is known as `self-reference'.
#
#   For information on the PAMAKE make utility, see PAMAKE.DOC
#
#   The directory structure for which this makefile was constructed is
#   as follows:
#
#   \msc51\bin      :  DOS C compiler executables
#   \msc51\binp     :  OS/2 C compiler executables
#   \msc51\binb     :  miscellaneous bound executables
#   \msc51\include  :  .H files
#   \msc51\lib      :  compiler library files
#
#   This makefile is invoked from the \msc51\pamake directory which
#   contains the C and H files for PAMAKE.
#
#   Note: use a renamed version of one of the targets of this make
#         in order to rebuild it under OS/2.  OS/2 will not allow
#         writing to the EXE file while it is actually running, and
#         you will get an error when LINK writes to it or BIND tries
#         to delete it.
#
#   --------------------------------------------------------------------

target: pamakep.exe pamaker.exe pamake.exe
	+@echo make complete:  target is up-to-date

#   --------------------------------------------------------------------
#
#   The following macro definitions are placed in the DOS environment
#   during execution of PAMAKE.  Modify them to specify the pathnames
#   which apply in your system.  Note that the PATH must include all
#   directories which contain commands to be executed by PAMAKE, since
#   it overrides your previous path during the execution of the make.
#
#   --------------------------------------------------------------------

RAM=h:\             # ram drive: change this line if yours is different
#TMP=$(RAM)         # set TMP environment variable for MSC: delete if none

#if PAMAKE_OS=OS2
+PATH=\msc51\binb;\msc51\binp
#else
+PATH=\msc51\binb;\msc51\bin;e:\bin
#endif

+INCLUDE=\msc51\include
+LIB=\msc51\lib

#if TEST=1
O=/Od          # disable optimisation
Z=/Zi          # generate CV records in OBJ
C=/CO          # generate CV records in EXE
#else
O=/Olt         # maximum optimisation, without intrinsics
#endif

#   --------------------------------------------------------------------
#
#   The following macro definitions are used to specify to PAMAKE the 
#   overrides required in Microsoft C v5.1 if both real and protected
#   mode libraries are in use with different names.  
#
#   --------------------------------------------------------------------

REALLIB=/nod:$Mlibce $Mlibcer           # real-mode libary override
PROTLIB=/nod:$Mlibce $Mlibcep           # protect-mode library override

#   --------------------------------------------------------------------
#
#   PAMAKE has a built-in command of $(CC) $(CFLAGS) $(CFILES) for C 
#   compilations.  Here we set these macros for the Microsoft compiler.
#
#   --------------------------------------------------------------------

M=S                                     # small model
CC=cl                                   # Microsoft C compilation
CFLAGS=/c $O /A$M /W3 $Z /DMSC
CFILES=$<

.SUFFIXES:                              # clear suffixes list
.SUFFIXES: .c                           # not .asm and .pnl as built-in

#   --------------------------------------------------------------------
#
#   Here are the programs to build
#
#   --------------------------------------------------------------------

OBJS= check.obj input.obj macro.obj reader.obj rules.obj ifproc.obj mtime.obj

#   Protected Mode Version

pamakep.exe: mainp.obj makep.obj $(OBJS)
	link @$-
mainp+check+input+macro+makep+reader+rules+ifproc+mtime+$(LIB)\apilmr
pamakep
pamakep $C
$(PROTLIB) doscalls
;
<

#   Real Mode Version

pamaker.exe: mainr.obj maker.obj $(OBJS)
	link @$-
mainr+check+input+macro+maker+reader+rules+ifproc+mtime/cp:1/st:4096
pamaker
pamaker $C
$(REALLIB)
;
<

#   Dual-mode Version

pamake.exe: mainp.obj makep.obj $(OBJS) pamaker.exe
	copy $- pamake.def
NAME PAMAKE
STUB 'PAMAKER.EXE'
<
	link @$-
mainp+check+input+macro+makep+reader+rules+ifproc+mtime+$(LIB)\apilmr
pamake
pamake $C
$(PROTLIB) doscalls
pamake.def;
<

#   --------------------------------------------------------------------
#
#   Commands to build objects.
#
#   --------------------------------------------------------------------

mainr.obj: main.c h.h 
	$(CC) $(CFLAGS) /DPAMDOS /Fomainr.obj main.c
mainp.obj: main.c h.h
	$(CC) $(CFLAGS) /DPAMOS2 /Fomainp.obj main.c
maker.obj: make.c h.h 
	$(CC) $(CFLAGS) /DPAMDOS /Fomaker.obj make.c 
makep.obj: make.c h.h
	$(CC) $(CFLAGS) /DPAMOS2 /Fomakep.obj make.c
$(OBJS): h.h

#   --------------------------------------------------------------------
#
#   end of makefile
#
#   --------------------------------------------------------------------
