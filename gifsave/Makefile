# $Id: Makefile,v 1.2 1998/07/05 16:29:55 sverrehu Exp $
DIST		= gifsave
VERMAJ		= 1
VERMIN		= 0
VERPAT		= 2
VERSION		= $(VERMAJ).$(VERMIN).$(VERPAT)

###########################################################################

CC		= gcc

OPTIM		= -O2
CCOPT		= -Wall $(OPTIM) $(CFLAGS)
LDOPT		= -s $(LDFLAGS)

LIBS		= 
OBJS		= gifsave.o example.o

###########################################################################

example: $(OBJS)

.o: $(OBJS)
	$(CC) $(CCOPT) -o $@ $(OBJS) $(LDOPT) $(LIBS)

.c.o:
	$(CC) -o $@ -c $(CCOPT) $<

clean:
	rm -f *.o core depend *~

depend dep:
	$(CC) $(INCDIR) -MM gifsave.c example.c >depend

###########################################################################

# To let the author make a distribution. The rest of the Makefile
# should be used by the author only.
LSMFILE		= $(DIST)-$(VERSION).lsm
DISTDIR		= $(DIST)-$(VERSION)
DISTFILE	= $(DIST)-$(VERSION).tar.gz
DISTFILES	= README $(LSMFILE) ChangeLog $(DIST).lsm.in \
		  Makefile Makefile.bcc \
		  gifsave.h gifsave.c example.c borland.c

$(LSMFILE): $(DIST).lsm.in
	VER=$(VERSION); \
	DATE=`date "+%d%b%y"|tr '[a-z]' '[A-Z]'`; \
	sed -e "s/VER/$$VER/g;s/DATE/$$DATE/g" $(DIST).lsm.in > $(LSMFILE)

chmod:
	chmod -R a+rX *

veryclean: clean
	rm -f example example.gif $(DIST)-$(VERSION).tar.gz $(LSMFILE)

dist: $(LSMFILE) chmod
	mkdir $(DISTDIR)
	chmod a+rx $(DISTDIR)
	for q in $(DISTFILES); do \
	  if test -r $$q; then \
	    ln -s ../$$q $(DISTDIR); \
	  else echo "warning: no file $$q"; fi; \
	  done
	tar -cvhzf $(DISTFILE) $(DISTDIR)
	chmod a+r $(DISTFILE)
	rm -rf $(DISTDIR)

ifeq (depend,$(wildcard depend))
include depend
endif
