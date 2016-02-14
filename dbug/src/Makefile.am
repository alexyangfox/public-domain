

INCLUDES =		-I$(srcdir)/../include -I../include
pkglib_LIBRARIES =	libdbug.a
noinst_HEADERS =	dbug_long.h
libdbug_a_SOURCES =	dbug.c sanity.c
#EXTRA_DIST =		user.r monty.doc readme.prof main.c factorial.c

OMIT_DEPENDENCIES =	pthread.h stdio.h __stdio.h stdlib.h __stdlib.h math.h\
			__math.h time.h __time.h unistd.h __unistd.h types.h \
			xtypes.h ac-types.h posix.h string.h __string.h \
			errno.h socket.h inet.h dirent.h netdb.h \
			cleanup.h cond.h debug_out.h fd.h kernel.h mutex.h \
			prio_queue.h pthread_attr.h pthread_once.h queue.h\
			sleep.h specific.h version.h pwd.h timers.h uio.h \
			cdefs.h machdep.h signal.h __signal.h util.h

# Don't update the files from bitkeeper
%::SCCS/s.%

