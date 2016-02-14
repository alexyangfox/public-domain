VER = b199
CC = gcc -s -Wall -O2  -fomit-frame-pointer
CPROGS = sym.c dict.c builtins.c load.c read.c write.c engine.c ru.c
OBS = sym.o dict.o builtins.o load.o engine.o ru.o read.o write.o
EXECS = ru ru prof debug trace gru 
PPROGS = top.pl co.pl lib.pl read.pl dcg.pl write.pl extra.pl other.pl headers.pl
BINDIR = /usr/local/bin
OTHER_PROLOG = sicstus -f

it:	bp

ru: $(OBS)
	$(CC) -o ru $(OBS)

headers:	co.pl
	echo "boot_to('tmp.bp'). " | bp
	echo "[headers]. ru. " | bp tmp.bp

bp:	wam.bp
	echo '$(PWD)/ru $$* $(PWD)/wam.bp' > bp
	chmod ugo+x bp
	@echo mv bp $(BINDIR)
	@echo TYPE bp TO run BinProlog!

wam.bp:	ru otherwam.bp
	echo "boot. " | ru otherwam.bp

$(OBS):	defs.h prof.h global.h

debug:
	$(CC) -DPROF -DDEBUG $(GG) -c $(CPROGS) debug.c
	$(CC) -g -o deb $(OBS) debug.o /usr/lib/debug/mallocmap.o

trace:
	$(CC) -DPROF -DTRACE $(GG) -c $(CPROGS) debug.c
	$(CC) -g -o deb $(OBS) debug.o /usr/lib/debug/mallocmap.o

#sun lint
lint:	$(CPROGS)
	lint -s -fd -errchk=%all -Ncheck=%all -Nlevel=3 $(CPROGS)

tags:	$(CPROGS)
	ctags $(CPROGS)

# bootstaping

remake:	old
	echo "boot. " | ru.old wam.old
	echo "[headers]. ru." ru.old wam.bp
	$(CC) -o ru $(CPROGS)
	echo "boot. " | ru wam.bp

old:
	cp ru ru.old
	cp wam.bp wam.old

from_other:
	@echo finished generating BinProlog using $(OTHER_PROLOG)...
	@echo bootrapping BinProlog...
	echo "boot. " | ru otherwam.bp

other:	clean_other otherwam.bp
	cp otherwam.bp wam.bp

clean_other:
	rm -f otherwam.bp

otherwam.bp: $(PPROGS)
	echo "compile([co,other]). boot_to('otherwam.bp'). "| $(OTHER_PROLOG)

clean:
	rm -f $(OBS) debug.o newappl.bp $(EXECS) wam.bp deb bp

realclean: clean clean_other
