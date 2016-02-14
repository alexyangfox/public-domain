PROG = portscan
SRCS = common.c hosts.c network.c parse.c ports.c portscan.c scan.c
OBJS = $(SRCS:.c=.o)

CFLAGS  += -Wall -pedantic -g
LDFLAGS += -lpcap -lpthread

PREFIX = /usr/local

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) -o $(PROG) $(OBJS) $(LDFLAGS)

install: $(PROG) portscan.1
	cp $(PROG) $(DESTDIR)$(PREFIX)/bin/$(PROG)
	chmod 755 $(DESTDIR)$(PREFIX)/bin/$(PROG)
	cp portscan.1 $(DESTDIR)$(PREFIX)/share/man/man1/portscan.1
	chmod 644 $(DESTDIR)$(PREFIX)/share/man/man1/portscan.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(PROG)
	rm -f $(DESTDIR)$(PREFIX)/share/man/man1/portscan.1

clean:
	rm -f $(OBJS) $(PROG)

.PHONY: all clean install
