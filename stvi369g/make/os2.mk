#
# Makefile for OS/2 HPFS, for use with MSC's NMAKE
#   G. Roelofs, 8 Oct 91
#

#####################
# MACRO DEFINITIONS #
#####################

MODEL = -AC  # Compact model lets us edit large files but keep small-model code
CC = cl
CFLAGS = $(MODEL) -Ox -nologo $(FP) -G2s -Lp -DOS2
LD = link
LDFLAGS = /noe /nol /st:6000	# /noi ??
LDFLAGS2 = ,stevie,,,stevie.def
EXE = .exe
O = .obj

MACH =	os2$O isfat$O
ARGV =	$(LIB)\setargv.obj	# keep separate so `make clean' won't delete it

OBJ1 =	alloc$O cmdline$O edit$O enveval$O fileio$O
OBJ2 =	help$O hexchars$O linefunc$O main$O mark$O
OBJ3 =	misccmds$O normal$O ops$O param$O ptrfunc$O
OBJ4 =	regexp$O regsub$O screen$O search$O sentence$O
OBJ5 =	tagcmd$O undo$O version$O $(MACH)

OBJ =	$(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5)
OBJS =	$(OBJ) $(ARGV)

###############################################
# BASIC COMPILE INSTRUCTIONS AND DEPENDENCIES #
###############################################

default:	stevie$(EXE)

.c$O :
	$(CC) -c $(CFLAGS) $*.c

stevie$(EXE):	$(OBJS) stevie.def
	echo $(LDFLAGS)+ > response.vi
	echo $(OBJ1)+ >> response.vi
	echo $(OBJ2)+ >> response.vi
	echo $(OBJ3)+ >> response.vi
	echo $(OBJ4)+ >> response.vi
	echo $(OBJ5)+ >> response.vi
	echo $(ARGV) $(LDFLAGS2) >> response.vi
	$(LD) @response.vi
	del response.vi

# Line is too long (~280 characters) with this version:
#	$(LD) $(LDFLAGS) $(OBJS) $(LDFLAGS2)

alloc$O:	alloc.c stevie.h env.h ascii.h keymap.h param.h term.h
cmdline$O:	cmdline.c stevie.h env.h ascii.h keymap.h param.h term.h
edit$O:		edit.c stevie.h env.h ascii.h keymap.h param.h term.h
fileio$O:	fileio.c stevie.h env.h ascii.h keymap.h param.h term.h
help$O:		help.c stevie.h env.h ascii.h keymap.h param.h term.h
hexchars$O:	hexchars.c stevie.h env.h ascii.h keymap.h param.h term.h
linefunc$O:	linefunc.c stevie.h env.h ascii.h keymap.h param.h term.h
main$O:		main.c stevie.h env.h ascii.h keymap.h param.h term.h
mark$O:		mark.c stevie.h env.h ascii.h keymap.h param.h term.h
misccmds$O:	misccmds.c stevie.h env.h ascii.h keymap.h param.h term.h
normal$O:	normal.c stevie.h env.h ascii.h keymap.h param.h term.h
ops$O:		ops.c stevie.h env.h ascii.h keymap.h param.h term.h
param$O:	param.c stevie.h env.h ascii.h keymap.h param.h term.h
ptrfunc$O:	ptrfunc.c stevie.h env.h ascii.h keymap.h param.h term.h
regexp$O:	regexp.c env.h regexp.h regmagic.h
regsub$O:	regsub.c env.h regexp.h regmagic.h
screen$O:	screen.c stevie.h env.h ascii.h keymap.h param.h term.h
search$O:	search.c stevie.h env.h ascii.h keymap.h param.h term.h
sentence$O:	sentence.c stevie.h env.h ascii.h keymap.h param.h term.h
tagcmd$O:	tagcmd.c stevie.h env.h ascii.h keymap.h param.h term.h
undo$O:		undo.c stevie.h env.h ascii.h keymap.h param.h term.h
version$O:	version.c
os2$O:		os2.c stevie.h env.h ascii.h keymap.h param.h term.h
isfat$O:	isfat.c

clean:
	rm -f $(OBJ) stevie$(EXE)
