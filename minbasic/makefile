basic:	basic.c
	cc -g -o basic basic.c

bascomp:
	./basic @ bascomp.bas bascomp.bas 1.c bascomp.rt && \
		cc -o bascomp 1.c && rm 1.c

test:	basic
	./basic @ test.bas

triple:	basic
	./basic @ bascomp.bas bascomp.bas 1.c bascomp.rt && cc 1.c
	./a.out bascomp.bas 2.c bascomp.rt
	diff 1.c 2.c && rm 1.c 2.c a.out

sums: clean
	ls | grep -v _sums | csum >_sums

csums:
	csum -u <_sums >_sums.new && mv -f _sums.new _sums

arc: clean
	zip -9 minbasic.zip *

clean:
	rm -f minbasic.zip basic TEST.DAT bascomp a.out *.core core
