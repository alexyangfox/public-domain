# Top Makefile for Otter 3.3 / Mace 2.2

install:
	./QuickInstall

test-otter:
	bin/otter < examples/auto/steam.in | awk '/PROOF/,/end of proof/'
	@echo "If you see 'end of proof', the test was successful."
	@echo "Now try 'make test-mace2'."

test-mace test-mace2:
	bin/mace2 -n6 -p < examples-mace2/basic/noncommutative_group.in | \
		awk '/Model/,/end_of_model/'
	@echo "If you see a group table, the test was successful."

realclean:	
	cd source         && $(MAKE)  realclean
	cd mace2          && $(MAKE)  realclean
	cd examples       && $(MAKE)  realclean
	cd examples-mace2 && $(MAKE)  realclean
	/bin/rm -f bin/*
	/bin/rm -f *~

all:
	$(MAKE) realclean
	cd source   && $(MAKE) CC=gcc all
	cd mace2    && $(MAKE) CC=gcc all

# The following cleans up, then makes a .tar.gz file of the current
# directory, leaving it in the parent directory.

DIR = $(shell basename $(PWD))

dist:
	$(MAKE) realclean
	cd .. && tar cvf $(DIR).tar $(DIR)
	gzip -f ../$(DIR).tar
	ls -lt ../$(DIR).tar.gz
