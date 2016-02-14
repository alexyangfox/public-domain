# Makefile for public domain tar program.
# @(#)Makefile 1.30	87/11/11

# Berserkeley version
DEFS = -DBSD42
LDFLAGS =
LIBS = 
LINT = lint
LINTFLAGS = -abchx
DEF_AR_FILE = \"/dev/rmt8\"
DEFBLOCKING = 20
O = o

# USG version
#DEFS = -DUSG
#LDFLAGS =
#LIBS = -lndir
#LINT = lint
#LINTFLAGS = -p
#DEF_AR_FILE = \"/dev/rmt8\"
#DEFBLOCKING = 20
#O = o

# UniSoft's Uniplus SVR2 with NFS
#DEFS = -DUSG -DUNIPLUS -DNFS -DSVR2
#LDFLAGS =
#LIBS = -lndir
#LINT = lint
#LINTFLAGS = -bx
#DEF_AR_FILE = \"/dev/rmt8\"
#DEFBLOCKING = 20
#O = o

# MASSCOMP version
#CC = ucb cc
#DEFS = -DBSD42
#LDFLAGS =
#LIBS = 
#LINT = lint
#LINTFLAGS = -bx
#DEF_AR_FILE = \"/dev/rmt0\"
#DEFBLOCKING = 20
#O = o

# (yuk) MS-DOS (Microsoft C) version
#MODEL = S
#DEFS = -DNONAMES -A$(MODEL) -nologo
#LDFLAGS =
#LIBS = $(MODEL)dir.lib
#LINT =	$(CC)
#LINTFLAGS = -W3
#DEF_AR_FILE = \"tar.out\"
#DEFBLOCKING = 20
#O = obj

# V7 version
# Pick open3 emulation or nonexistence.  See open3.h, port.c.
##DEFS = -DV7 -DEMUL_OPEN3 -Dvoid=int
##DEFS = -DV7 -DNO_OPEN3 -Dvoid=int
#LDFLAGS =
#LIBS = -lndir
#LINT = lint
#LINTFLAGS = -abchx
#DEF_AR_FILE = \"/dev/rmt8\"
#DEFBLOCKING = 20
#O = o

# Minix version
# No lint, so no lintflags.  Default file is stdin/out.  (Minix "tar"
# doesn't even take an "f" flag, it assumes argv[2] is the archive name!)
# Minix "make" doesn't expand macros right, so Minix users will have
# to expand CFLAGS, SRCS, O, etc by hand, or fix your make.  Not my problem!
# You'll also need to come up with getopt() and ctime(), the directory
# library, and a fixed doprintf() that handles %*s.  Put this stuff in
# the "SUBSRC/SUBOBJ" macro below if you didn't put it in your C library.
# Note that Minix "cc" produces ".s" files, not .o's, so O = s has been set.
#
# Pick open3 emulation or nonexistence.  See open3.h, port.c.
##DEFS = -DV7 -DMINIX -DEMUL_OPEN3
##DEFS = -DV7 -DMINIX -DNO_OPEN3
#LDFLAGS =
#LIBS =
#DEF_AR_FILE = \"-\"
#DEFBLOCKING = 8	/* No good reason for this, change at will */
#O = s

# Xenix version
#DEFS = -DUSG -DXENIX
#LDFLAGS = 
#LIBS = -lx
#LINT = lint
#LINTFLAGS = -p
#DEF_AR_FILE = \"/dev/rmt8\"
#DEFBLOCKING = 20
#O = o


CFLAGS = $(COPTS) $(ALLDEFS)
ALLDEFS = $(DEFS) \
	-DDEF_AR_FILE=$(DEF_AR_FILE) \
	-DDEFBLOCKING=$(DEFBLOCKING)
# next line for Debugging
COPTS = -g
# next line for Production
#COPTS = -O

# Add things here like getopt, readdir, etc that aren't in your
# standard libraries.  (E.g. MSDOS needs getopt, msd_dir.c, msd_dir.obj)
SUBSRC=	
SUBOBJ=	

# Destination directory and installation program for make install
DESTDIR = /usr/pd
INSTALL = cp
RM = rm -f

SRC1 =	tar.c create.c extract.c buffer.c getoldopt.c
SRC2 =	list.c names.c diffarch.c port.c wildmat.c $(SUBSRC)
SRCS =	$(SRC1) $(SRC2)
OBJ1 =	tar.$O create.$O extract.$O buffer.$O getoldopt.$O list.$O
OBJ2 =	names.$O diffarch.$O port.$O wildmat.$O $(SUBOBJ)
OBJS =	$(OBJ1) $(OBJ2)
AUX =	README PORTING Makefile TODO tar.1 tar.5 tar.h port.h open3.h \
	msd_dir.h msd_dir.c

all:	tar

tar:	$(OBJS)
	$(CC) $(LDFLAGS) -o tar $(COPTS) $(OBJS) $(LIBS)
# command is too long for Messy-Dos (128 char line length limit) so
# this kludge is used...
#	@echo $(OBJ1) + > command
#	@echo $(OBJ2) >> command
#	link @command, $@,,$(LIBS) /NOI;
#	@$(RM) command

install: all
	$(RM) $(DESTDIR)/tar $(DESTDIR)/.man/tar.[15]
	$(INSTALL) tar   $(DESTDIR)/tar
	$(INSTALL) tar.1 $(DESTDIR)/.man/tar.1
	$(INSTALL) tar.5 $(DESTDIR)/.man/tar.5

lint:	$(SRCS)
	$(LINT) $(LINTFLAGS) $(ALLDEFS) $(SRCS)

clean:
	$(RM) errs $(OBJS) tar

tar.shar: $(SRCS) $(AUX)
	shar >tar.shar1 $(AUX)
	shar >tar.shar2 $(SRC1)
	shar >tar.shar3 $(SRC2)

tar.tar.Z: $(SRCS) $(AUX)
	/bin/tar cf - $(AUX) $(SRCS) | compress -v >tar.tar.Z

$(OBJS): tar.h port.h
