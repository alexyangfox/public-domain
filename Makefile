
#	Unix makefile for building SMARTALLOC demo and test programs

#	To build with SMARTALLOC defined, leave the line "SMART ="
#	defined.  To test without SMARTALLOC, comment it out.  When
#	you change SMART, you must do a "make clean" and then a
#	"make".

#	Make targets:
#		       <default>    Rebuild everything
#		       clean	    Delete all objects for full rebuild
#		       lint	    Run lint on SMARTALLOC libraries
#		       release	    Make .zip release file
#		       check	    Run test, verify output against expected


SMART = -DSMARTALLOC
# SMART = -DSMARTALLOC -DDUMPBUF

OBJS = smtest.o smartall.o alloc.o

LIBS =

smtest: $(OBJS)
	cc -g $(OBJS) -o smtest $(LIBS) $(SMART)

clean:
	rm -f *.o *.obj *.exe *.bak smtest core core.* *.zip

lint:
	lint smartall.c alloc.c -DSMARTALLOC
	lint smartall.c alloc.c

RELFILES = Makefile README alloc.c alloc.h \
	   smartall.c smartall.png smartall.h \
	   smartall.html standard_screen.css \
	   smtest.c test.dat

release:
	rm -f smartall.zip
	zip smartall.zip $(RELFILES)

check:	smtest
	./smtest 2>test.bak
	diff test.dat test.bak

.c.o:
	cc -g -c $*.c $(SMART)
