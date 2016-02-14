BIN      = cat rm touch ls echo true false mv wc mkdir sh head tail dirname basename cmp tee cksum comm pwd kill
USABLE   = cat touch echo true false wc mkdir tail dirname basename cmp tee comm pwd
LIB      = lib/*.c
CFLAGS  += -Wall -Wextra $(LIB)
MAKEDEPS = Makefile libhutils.h
PREFIX  ?= $(HOME)

all: $(BIN)

%: %.c $(LIB) $(MAKEDEPS)
	$(CC) $(CFLAGS) -o $@ $<
%: text/%.c $(LIB) $(MAKEDEPS)
	$(CC) $(CFLAGS) -o $@ $<
%: fs/%.c $(LIB) $(MAKEDEPS)
	$(CC) $(CFLAGS) -o $@ $< 
%: proc/%.c $(LIB) $(MAKEDEPS)
	$(CC) $(CFLAGS) -o $@ $<

# Phony targets.
.PHONY: all clean install-all install rebuild

install-all: $(BIN)
	mkdir -p $(PREFIX)/bin
	install $(BIN) $(PREFIX)/bin

install: $(USABLE)
	mkdir -p $(PREFIX)/bin
	install $(USABLE) $(PREFIX)/bin

rebuild: clean all

clean:
	rm -f $(BIN)
