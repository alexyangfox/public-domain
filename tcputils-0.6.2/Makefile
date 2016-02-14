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


PROGRAMS = tcpconnect tcplisten tcpbug mini-inetd getpeername
MANUALS1 = tcpconnect.1 tcplisten.1 tcpbug.1 mini-inetd.1 getpeername.1


all: ${IPLIB} ${PROGRAMS}


tcpconnect: tcpconnect.o relay.o
	${CC} ${LDFLAGS} $^ ${IPLIB} ${NETLIBS} -o $@
tcplisten: tcplisten.o relay.o
	${CC} ${LDFLAGS} $^ ${IPLIB} ${NETLIBS} -o $@
tcpbug: tcpbug.o relay.o
	${CC} ${LDFLAGS} $^ ${IPLIB} ${NETLIBS} -o $@
mini-inetd: mini-inetd.o
	${CC} ${LDFLAGS} $^ ${IPLIB} ${NETLIBS} -o $@
getpeername: getpeername.o
	${CC} ${LDFLAGS} $^ ${NETLIBS} -o $@

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

${bindir}:; ${MKDIR_P} $@
${man1dir}:; ${MKDIR_P} $@


${PROGRAMS}: ${IPLIB}
tcpconnect.o: ip/ip_misc.h relay.h
tcplisten.o: ip/ip_misc.h relay.h
tcpbug.o: ip/ip_misc.h relay.h
mini-inetd.o: ip/ip_misc.h
relay.o: relay.h
getpeername.o:
