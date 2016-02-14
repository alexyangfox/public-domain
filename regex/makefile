#
# public-domain regex (bsd) routines
#
CFLAGS = -O

RSRC = regex.o re_fail.o

regex:  $(RSRC)
#	if necessary, insert code to put these into a library

rlint:
	lint -phc regex.c

debug:
	cc -O -DDEBUG -o ogrep grep.c regex.c re_fail.c

lgrep:  grep.o
	cc -o lgrep grep.o

ogrep:  grep.o $(RSRC)
	cc -o ogrep grep.o $(RSRC)
clean:
	rm -f *.o lgrep ogrep core

