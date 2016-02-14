# Makefile for k3

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/man/man1

CC=gcc

CFLAGS=-O -Wall

all: k3

k3: k3.o
	$(CC) $(CFLAGS) -o k3 k3.o

installdirs:
	/bin/sh ./mkinstalldirs $(BINDIR) $(MANDIR)

install: k3 installdirs
	install -m 755 k3 $(BINDIR)
	install -m 644 k3.1 $(MANDIR)

uninstall:
	$(RM) $(BINDIR)/k3 $(MANDIR)/k3.1*

clean:
	rm -f *.o *~ k3


VERS=1.5

tgz: ../k3-$(VERS).tar.gz

../k3-$(VERS).tar.gz: clean
	$(RM) ../k3-$(VERS)
	@cd ..;ln -s k3 k3-$(VERS)
	cd ..;tar zchvf k3-$(VERS).tar.gz k3-$(VERS)
	@cd ..;$(RM) k3-$(VERS)
