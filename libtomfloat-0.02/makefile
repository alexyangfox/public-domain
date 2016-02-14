#GCC makefile for LibTomFloat
#
#Tom St Denis

default: libtomfloat.a

CFLAGS += -Os -Wall -W -I./

VERSION=0.02

#default files to install
LIBNAME=libtomfloat.a
HEADERS=tomfloat.h

#LIBPATH-The directory for libtomfloat to be installed to.
#INCPATH-The directory to install the header files for libtomfloat.
#DATAPATH-The directory to install the pdf docs.
DESTDIR=
LIBPATH=/usr/lib
INCPATH=/usr/include
DATAPATH=/usr/share/doc/libtomfloat/pdf


OBJECTS = \
mpf_init.o mpf_clear.o mpf_init_multi.o mpf_clear_multi.o mpf_init_copy.o \
\
mpf_copy.o mpf_exch.o mpf_abs.o mpf_neg.o \
\
mpf_cmp.o mpf_cmp_d.o \
\
mpf_normalize.o mpf_normalize_to.o mpf_iterations.o \
\
mpf_const_0.o    mpf_const_1r2.o  mpf_const_2rpi.o  mpf_const_e.o     \
mpf_const_l2e.o  mpf_const_pi.o   mpf_const_pi4.o   mpf_const_1pi.o   \
mpf_const_2pi.o  mpf_const_d.o    mpf_const_l10e.o  mpf_const_le2.o   \
mpf_const_pi2.o  mpf_const_r2.o   mpf_const_ln_d.o                    \
\
mpf_mul_2.o mpf_div_2.o mpf_add.o mpf_sub.o mpf_mul.o mpf_sqr.o mpf_div.o \
mpf_add_d.o mpf_sub_d.o mpf_mul_d.o mpf_div_d.o \
\
mpf_invsqrt.o mpf_inv.o mpf_exp.o mpf_sqrt.o mpf_pow.o mpf_ln.o \
\
mpf_cos.o mpf_sin.o mpf_tan.o mpf_acos.o mpf_asin.o mpf_atan.o

libtomfloat.a: $(OBJECTS)
	$(AR) $(ARFLAGS) libtomfloat.a $(OBJECTS)
	ranlib libtomfloat.a

ex1: libtomfloat.a demos/ex1.o
	$(CC) demos/ex1.o libtomfloat.a -ltommath -o ex1

#LTF user manual
mandvi: float.tex
	echo "hello" > float.ind
	latex float > /dev/null
	latex float > /dev/null
	makeindex float
	latex float > /dev/null

#LTF user manual [pdf]
manual:	mandvi
	pdflatex float >/dev/null
	rm -f float.aux float.dvi float.log float.idx float.lof float.out float.toc

install: libtomfloat.a
	install -d -g root -o root $(DESTDIR)$(LIBPATH)
	install -d -g root -o root $(DESTDIR)$(INCPATH)
	install -g root -o root $(LIBNAME) $(DESTDIR)$(LIBPATH)
	install -g root -o root $(HEADERS) $(DESTDIR)$(INCPATH)

clean:
	rm -f $(OBJECTS) libtomfloat.a *~ demos/*.o demos/*~ ex1
	rm -f float.aux float.dvi float.log float.idx float.lof float.out float.toc float.ilg float.ind float.pdf

zipup: clean manual
	cd .. ; rm -rf ltf* libtomfloat-$(VERSION) ; mkdir libtomfloat-$(VERSION) ; \
	cp -R ./libtomfloat/* ./libtomfloat-$(VERSION)/ ; \
	tar -c libtomfloat-$(VERSION)/* | bzip2 -9vvc > ltf-$(VERSION).tar.bz2 ; \
	zip -9 -r ltf-$(VERSION).zip libtomfloat-$(VERSION)/*
