
DFLAGS = -DTP_NAMES -DTP_SIGNAL -DTP_FORK -DTP_RUSAGE

CC = gcc

#############################################################################
# Specify the compiler flags; include DFLAGS (above)
#
# optimized

CFLAGS = -O $(DFLAGS)

#
# save symbols for debuggers
# CFLAGS = -g $(DFLAGS)
#
# optimized, debugging  (gcc and gdb can handle this)
# CFLAGS = -O -g $(DFLAGS)
#
# gprof profiling
# CFLAGS = -pg -O $(DFLAGS)

#############################################################################


#############################################################################
#
#  The following is for FormEd, a program for displaying and
#  editing formulas.
#
#
# XLIBS = -lXaw -lXmu -lXext -lXt -lX11 -lm
#
# We used to need the following at MCS; you may need something similar.
# XLIBS = -L/usr/local/X11R5/lib -lXaw -lXmu -lXext -lXt -lX11 -lm
# CFLAGS = -O $(DFLAGS) -I/usr/include/X11
#
# The following works on a RedHat 7.3 computer

XLIBS = -L/usr/X11R6/lib -lXaw 

formed: formed.o display.o callback.o
	$(CC) $(CFLAGS) formed.o display.o callback.o \
	../libotter.a $(XLIBS) -o formed

display.o callback.o formed.o: formed.h

clean realclean:
	/bin/rm -f *.o formed
