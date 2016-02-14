
#   Unix Makefile for xd

#   For production
CFLAGS = -O
STRIP = strip

#   For debugging
#CFLAGS = -g
#STRIP = @true

xd:	xd.o
	cc $(CFLAGS) xd.o -o xd
	$(STRIP) xd

clean:
	rm -f xd *.bak *.cat core *.out *.o *.shar *.gz *.tar *.zip *.opt *.ncb *.plg

RELEASE = Makefile xd.1 xd.c xd.html xd.gif log.txt

ZIPREL = $(RELEASE) xd.dsp xd.dsw Release/xd.exe

tar:
	rm -f xd.tar xd.tar.gz
	tar cfv xd.tar $(RELEASE)
	gzip xd.tar

zip:
	rm -f xd.zip
	zip -j xd.zip $(ZIPREL)

lint:
	lint -h xd.c

manpage:
	nroff -man xd.1 | more

printman:
	ptroff -man xd.1

catman:
	nroff -man xd.1 | col -b >xd.cat

#   The following tests should produce no error messages from XD or CMP

test:	testb testad testao testdd testdo

testb:	xd
	xd xd /tmp/xd1.bak
	xd -l /tmp/xd1.bak /tmp/xd2.bak
	cmp /tmp/xd2.bak xd
	xd -c xd /tmp/xd3.bak
	xd -l /tmp/xd3.bak /tmp/xd4.bak
	cmp /tmp/xd4.bak xd
	xd -l -s /tmp/xd3.bak /tmp/xd4.bak
	cmp /tmp/xd4.bak xd
	sed 's/........//' /tmp/xd3.bak >/tmp/xd5.bak
	xd -l -s /tmp/xd5.bak /tmp/xd6.bak
	cmp /tmp/xd6.bak xd
	echo Hello there >/tmp/xd10.bak
	xd /tmp/xd10.bak >/tmp/xd7.bak
	sed '1d' /tmp/xd3.bak >>/tmp/xd7.bak
	xd -l -s /tmp/xd7.bak /tmp/xd8.bak
	cp /tmp/xd10.bak /tmp/xd9.bak
	tail +17c xd >>/tmp/xd9.bak
	cmp /tmp/xd8.bak /tmp/xd9.bak
	cat xd | xd -dDeFile >/tmp/xd10.c
	echo "main() {write(1,DeFile,sizeof DeFile); return 0;}" >>/tmp/xd10.c
	cc /tmp/xd10.c -o /tmp/xd10.bak
	chmod 755 /tmp/xd10.bak
	/tmp/xd10.bak >/tmp/xd11.bak
	cmp /tmp/xd11.bak xd

testad: xd
	xd -ad xd /tmp/xd1.bak
	xd -ad -l /tmp/xd1.bak /tmp/xd2.bak
	cmp /tmp/xd2.bak xd
	xd -ad -c xd /tmp/xd3.bak
	xd -ad -l /tmp/xd3.bak /tmp/xd4.bak
	cmp /tmp/xd4.bak xd
	xd -ad -l -s /tmp/xd3.bak /tmp/xd4.bak
	cmp /tmp/xd4.bak xd
	sed 's/........//' /tmp/xd3.bak >/tmp/xd5.bak
	xd -ad -l -s /tmp/xd5.bak /tmp/xd6.bak
	cmp /tmp/xd6.bak xd
	echo Hello there >/tmp/xd10.bak
	xd -ad /tmp/xd10.bak >/tmp/xd7.bak
	sed '1d' /tmp/xd3.bak >>/tmp/xd7.bak
	xd -ad -l -s /tmp/xd7.bak /tmp/xd8.bak
	cp /tmp/xd10.bak /tmp/xd9.bak
	tail +17c xd >>/tmp/xd9.bak
	cmp /tmp/xd8.bak /tmp/xd9.bak

testao: xd
	xd -ao xd /tmp/xd1.bak
	xd -ao -l /tmp/xd1.bak /tmp/xd2.bak
	cmp /tmp/xd2.bak xd
	xd -ao -c xd /tmp/xd3.bak
	xd -ao -l /tmp/xd3.bak /tmp/xd4.bak
	cmp /tmp/xd4.bak xd
	xd -ao -l -s /tmp/xd3.bak /tmp/xd4.bak
	cmp /tmp/xd4.bak xd
	sed 's/........//' /tmp/xd3.bak >/tmp/xd5.bak
	xd -ao -l -s /tmp/xd5.bak /tmp/xd6.bak
	cmp /tmp/xd6.bak xd
	echo Hello there >/tmp/xd10.bak
	xd -ao /tmp/xd10.bak >/tmp/xd7.bak
	sed '1d' /tmp/xd3.bak >>/tmp/xd7.bak
	xd -ao -l -s /tmp/xd7.bak /tmp/xd8.bak
	cp /tmp/xd10.bak /tmp/xd9.bak
	tail +17c xd >>/tmp/xd9.bak
	cmp /tmp/xd8.bak /tmp/xd9.bak

testdd: xd
	xd -nd -ad xd /tmp/xd1.bak
	xd -nd -ad -l /tmp/xd1.bak /tmp/xd2.bak
	cmp /tmp/xd2.bak xd
	xd -nd -ad -c xd /tmp/xd3.bak
	xd -nd -ad -l /tmp/xd3.bak /tmp/xd4.bak
	cmp /tmp/xd4.bak xd
	xd -nd -ad -l -s /tmp/xd3.bak /tmp/xd4.bak
	cmp /tmp/xd4.bak xd
	sed 's/........//' /tmp/xd3.bak >/tmp/xd5.bak
	xd -nd -ad -l -s /tmp/xd5.bak /tmp/xd6.bak
	cmp /tmp/xd6.bak xd
	echo Hello there >/tmp/xd10.bak
	xd -nd -ad /tmp/xd10.bak >/tmp/xd7.bak
	sed '1d' /tmp/xd3.bak >>/tmp/xd7.bak
	xd -nd -ad -l -s /tmp/xd7.bak /tmp/xd8.bak
	cp /tmp/xd10.bak /tmp/xd9.bak
	tail +17c xd >>/tmp/xd9.bak
	cmp /tmp/xd8.bak /tmp/xd9.bak

testdo: xd
	xd -no -ah xd /tmp/xd1.bak
	xd -no -ah -l /tmp/xd1.bak /tmp/xd2.bak
	cmp /tmp/xd2.bak xd
	xd -no -ah -c xd /tmp/xd3.bak
	xd -no -ah -l /tmp/xd3.bak /tmp/xd4.bak
	cmp /tmp/xd4.bak xd
	xd -no -ah -l -s /tmp/xd3.bak /tmp/xd4.bak
	cmp /tmp/xd4.bak xd
	sed 's/........//' /tmp/xd3.bak >/tmp/xd5.bak
	xd -no -ah -l -s /tmp/xd5.bak /tmp/xd6.bak
	cmp /tmp/xd6.bak xd
	echo Hello there >/tmp/xd10.bak
	xd -no -ah /tmp/xd10.bak >/tmp/xd7.bak
	sed '1d' /tmp/xd3.bak >>/tmp/xd7.bak
	xd -no -ah -l -s /tmp/xd7.bak /tmp/xd8.bak
	cp /tmp/xd10.bak /tmp/xd9.bak
	tail +17c xd >>/tmp/xd9.bak
	cmp /tmp/xd8.bak /tmp/xd9.bak
