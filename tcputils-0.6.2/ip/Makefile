OBJECTS = addr.o tcp_connect.o tcp_open.o tcp_listen.o
IPLIB = libiphelp.a

CC = gcc -pedantic -Wall
COPT = -g -O2
CFLAGS = ${COPT}
RANLIB = ranlib

all: ${IPLIB}

${IPLIB}: ${OBJECTS}
	${AR} ruv $@ ${OBJECTS}
	${RANLIB} $@

clean:
	${RM} *.o ${IPLIB} *~ *_pure_*

install:
	${INSTALL} ${IPLIB} ${prefix}/lib
	${INSTALL} ip_misc.h ${prefix}/include


tcp_connect.o: ip_misc.h
tcp_open.o: ip_misc.h
tcp_listen.o: ip_misc.h
addr.o: ip_misc.h
