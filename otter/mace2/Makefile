DFLAGS = -DTP_NAMES -DTP_SIGNAL -DTP_RUSAGE

CC = gcc

CFLAGS = -O $(DFLAGS)

# CFLAGS = -g $(DFLAGS)

# Checks for gcc:
# CFLAGS = -ansi -pedantic -Wall

OBJECTS = clock.o avail.o opts.o stats.o miscellany.o flatten.o\
	dp.o generate.o print.o part.o

all: mace2 anldp install clean

mace2: mace2.o $(OBJECTS)
	$(CC) $(CFLAGS) mace2.o $(OBJECTS) ../source/libotter.a -o mace2 

anldp: anldp.o $(OBJECTS)
	$(CC) $(CFLAGS) anldp.o $(OBJECTS) ../source/libotter.a -o anldp 

mace2.o anldp.o $(OBJECTS): Mace2.h Clock.h Avail.h Opts.h Stats.h\
	Miscellany.h Dp.h Generate.h Flatten.h Part.h

install:
	/bin/mv mace2 anldp ../bin

clean:
	/bin/rm -f *.o

realclean:
	/bin/rm -f *.o mace2 anldp *~


