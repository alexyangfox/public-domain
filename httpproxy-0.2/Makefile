# Linux RedHat 5.0 has header files that generates warnings when compiling
# with -pedantic, so we leave that out per default.
CC = gcc -Wall #-pedantic
INCLUDE-FLAGS =
COPT = -g -O2
LDFLAGS = ${COPT}
CFLAGS = ${COPT} ${INCLUDE-FLAGS}
RANLIB = ranlib
INSTALL = install -c
MKDIR_P = mkdir -p
# Solaris 2 needs to link with "-lsocket -lnsl".  For other unices you might
# need to comment out those libraries.
NETLIBS = -lsocket -lnsl

# Where to install things.
prefix = /usr/local
bindir = ${prefix}/bin
mandir = ${prefix}/man
man1dir = ${mandir}/man1


# ------------------------------------------------------------------
# Things below shouldn't be needed to be changed in order to compile


SUBMAKE = ${MAKE} prefix="${prefix}" CC="${CC}" CFLAGS="${CFLAGS}" AR="${AR}" \
	  RANLIB="${RANLIB}" INSTALL="${INSTALL}" VPATH="${VPATH}" ${MFLAGS}

IPLIB = ip/libiphelp.a


PROGRAMS = httpproxy
MANUALS1 = httpproxy.1


all: ${IPLIB} ${PROGRAMS}


httpproxy: httpproxy.o relay.o proxy.o util.o
	${CC} ${LDFLAGS} $^ ${IPLIB} ${NETLIBS} -o $@

${IPLIB}:
	cd ip; ${SUBMAKE} all

clean:
	${RM}  ${PROGRAMS} *.o *~ core
	cd ip; ${SUBMAKE} clean

install: install-bin install-man

install-bin: ${bindir} #${PROGRAMS}
	${INSTALL} ${PROGRAMS} ${bindir}

install-man: ${man1dir} #${MANUALS1}
	${INSTALL} ${MANUALS1} ${man1dir}

install-lib:
	cd ip; ${SUBMAKE} install

tar:
	VERSION=`cat version.h | grep httpproxy_version | sed -e 's/.*"httpproxy //' -e 's/".*//'`; \
	tar --no-recursion -cvf - \
		`find * \! -path CVS/\* \! -path \*/CVS/\* \! -name CVS \! -name .cvsignore | sort` |\
	tardy --prefix=httpproxy-$$VERSION - |\
	gzip --best >../httpproxy-$$VERSION.tar.gz; \
	ls -l ../httpproxy-$$VERSION.tar.gz

${bindir}:; ${MKDIR_P} $@
${man1dir}:; ${MKDIR_P} $@


${PROGRAMS}: ${IPLIB}
httpproxy.o: ip/ip_misc.h proxy.h relay.h
relay.o: relay.h
