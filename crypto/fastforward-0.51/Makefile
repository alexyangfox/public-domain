# Don't edit Makefile! Use conf-* for configuration.

SHELL=/bin/sh

default: it

alloc.a: \
makelib alloc.o alloc_re.o
	./makelib alloc.a alloc.o alloc_re.o

alloc.o: \
compile alloc.c alloc.h error.h
	./compile alloc.c

alloc_re.o: \
compile alloc_re.c alloc.h byte.h
	./compile alloc_re.c

auto-ccld.sh: \
conf-cc conf-ld warn-auto.sh
	( cat warn-auto.sh; \
	echo CC=\'`head -1 conf-cc`\'; \
	echo LD=\'`head -1 conf-ld`\' \
	) > auto-ccld.sh

auto-str: \
load auto-str.o substdio.a error.a str.a
	./load auto-str substdio.a error.a str.a 

auto-str.o: \
compile auto-str.c substdio.h readwrite.h exit.h
	./compile auto-str.c

auto_qmail.c: \
auto-str conf-qmail
	./auto-str auto_qmail `head -1 conf-qmail` > auto_qmail.c

auto_qmail.o: \
compile auto_qmail.c
	./compile auto_qmail.c

byte_chr.o: \
compile byte_chr.c byte.h
	./compile byte_chr.c

byte_copy.o: \
compile byte_copy.c byte.h
	./compile byte_copy.c

byte_cr.o: \
compile byte_cr.c byte.h
	./compile byte_cr.c

byte_diff.o: \
compile byte_diff.c byte.h
	./compile byte_diff.c

case.a: \
makelib case_lowerb.o
	./makelib case.a case_lowerb.o

case_lowerb.o: \
compile case_lowerb.c case.h
	./compile case_lowerb.c

cdb.a: \
makelib cdb_hash.o cdb_unpack.o cdb_seek.o
	./makelib cdb.a cdb_hash.o cdb_unpack.o cdb_seek.o

cdb_hash.o: \
compile cdb_hash.c cdb.h uint32.h
	./compile cdb_hash.c

cdb_seek.o: \
compile cdb_seek.c cdb.h uint32.h
	./compile cdb_seek.c

cdb_unpack.o: \
compile cdb_unpack.c cdb.h uint32.h
	./compile cdb_unpack.c

cdbmake.a: \
makelib cdbmake_pack.o cdbmake_hash.o cdbmake_add.o
	./makelib cdbmake.a cdbmake_pack.o cdbmake_hash.o \
	cdbmake_add.o

cdbmake_add.o: \
compile cdbmake_add.c cdbmake.h uint32.h
	./compile cdbmake_add.c

cdbmake_hash.o: \
compile cdbmake_hash.c cdbmake.h uint32.h
	./compile cdbmake_hash.c

cdbmake_pack.o: \
compile cdbmake_pack.c cdbmake.h uint32.h
	./compile cdbmake_pack.c

cdbmss.o: \
compile cdbmss.c readwrite.h seek.h alloc.h cdbmss.h cdbmake.h \
uint32.h substdio.h
	./compile cdbmss.c

check: \
it instcheck
	./instcheck

coe.o: \
compile coe.c coe.h
	./compile coe.c

compile: \
make-compile warn-auto.sh systype
	( cat warn-auto.sh; ./make-compile "`cat systype`" ) > \
	compile
	chmod 755 compile

control.o: \
compile control.c readwrite.h open.h getln.h stralloc.h gen_alloc.h \
substdio.h error.h control.h alloc.h scan.h
	./compile control.c

env.a: \
makelib envread.o
	./makelib env.a envread.o

envread.o: \
compile envread.c env.h str.h
	./compile envread.c

error.a: \
makelib error.o error_str.o
	./makelib error.a error.o error_str.o

error.o: \
compile error.c error.h
	./compile error.c

error_str.o: \
compile error_str.c error.h
	./compile error_str.c

fastforward: \
load fastforward.o slurpclose.o coe.o strset.o qmail.o auto_qmail.o \
getopt.a cdb.a env.a strerr.a substdio.a stralloc.a alloc.a error.a \
case.a str.a fs.a sig.a wait.a seek.a open.a fd.a
	./load fastforward slurpclose.o coe.o strset.o qmail.o \
	auto_qmail.o getopt.a cdb.a env.a strerr.a substdio.a \
	stralloc.a alloc.a error.a case.a str.a fs.a sig.a wait.a \
	seek.a open.a fd.a 

fastforward.0: \
fastforward.1
	nroff -man fastforward.1 > fastforward.0

fastforward.o: \
compile fastforward.c stralloc.h gen_alloc.h substdio.h subfd.h \
substdio.h strset.h uint32.h sgetopt.h subgetopt.h readwrite.h exit.h \
strerr.h env.h sig.h qmail.h substdio.h fmt.h case.h alloc.h coe.h \
seek.h wait.h fork.h
	./compile fastforward.c

fd.a: \
makelib fd_copy.o fd_move.o
	./makelib fd.a fd_copy.o fd_move.o

fd_copy.o: \
compile fd_copy.c fd.h
	./compile fd_copy.c

fd_move.o: \
compile fd_move.c fd.h
	./compile fd_move.c

find-systype: \
find-systype.sh auto-ccld.sh
	cat auto-ccld.sh find-systype.sh > find-systype
	chmod 755 find-systype

fmt_ulong.o: \
compile fmt_ulong.c fmt.h
	./compile fmt_ulong.c

fork.h: \
compile load tryvfork.c fork.h1 fork.h2
	( ( ./compile tryvfork.c && ./load tryvfork ) >/dev/null \
	2>&1 \
	&& cat fork.h2 || cat fork.h1 ) > fork.h
	rm -f tryvfork.o tryvfork

fs.a: \
makelib fmt_ulong.o scan_ulong.o
	./makelib fs.a fmt_ulong.o scan_ulong.o

getln.a: \
makelib getln.o getln2.o
	./makelib getln.a getln.o getln2.o

getln.o: \
compile getln.c substdio.h byte.h stralloc.h gen_alloc.h getln.h
	./compile getln.c

getln2.o: \
compile getln2.c substdio.h stralloc.h gen_alloc.h byte.h getln.h
	./compile getln2.c

getopt.a: \
makelib subgetopt.o sgetopt.o
	./makelib getopt.a subgetopt.o sgetopt.o

hassgact.h: \
trysgact.c compile load
	( ( ./compile trysgact.c && ./load trysgact ) >/dev/null \
	2>&1 \
	&& echo \#define HASSIGACTION 1 || exit 0 ) > hassgact.h
	rm -f trysgact.o trysgact

haswaitp.h: \
trywaitp.c compile load
	( ( ./compile trywaitp.c && ./load trywaitp ) >/dev/null \
	2>&1 \
	&& echo \#define HASWAITPID 1 || exit 0 ) > haswaitp.h
	rm -f trywaitp.o trywaitp

hier.o: \
compile hier.c auto_qmail.h
	./compile hier.c

install: \
load install.o hier.o auto_qmail.o strerr.a substdio.a error.a open.a \
str.a
	./load install hier.o auto_qmail.o strerr.a substdio.a \
	error.a open.a str.a 

install.o: \
compile install.c substdio.h strerr.h error.h open.h readwrite.h \
exit.h
	./compile install.c

instcheck: \
load instcheck.o hier.o auto_qmail.o strerr.a substdio.a error.a \
str.a
	./load instcheck hier.o auto_qmail.o strerr.a substdio.a \
	error.a str.a 

instcheck.o: \
compile instcheck.c strerr.h error.h readwrite.h exit.h
	./compile instcheck.c

it: \
prog man

load: \
make-load warn-auto.sh systype
	( cat warn-auto.sh; ./make-load "`cat systype`" ) > load
	chmod 755 load

make-compile: \
make-compile.sh auto-ccld.sh
	cat auto-ccld.sh make-compile.sh > make-compile
	chmod 755 make-compile

make-load: \
make-load.sh auto-ccld.sh
	cat auto-ccld.sh make-load.sh > make-load
	chmod 755 make-load

make-makelib: \
make-makelib.sh auto-ccld.sh
	cat auto-ccld.sh make-makelib.sh > make-makelib
	chmod 755 make-makelib

makelib: \
make-makelib warn-auto.sh systype
	( cat warn-auto.sh; ./make-makelib "`cat systype`" ) > \
	makelib
	chmod 755 makelib

man: \
fastforward.0 printforward.0 setforward.0 newaliases.0 \
printmaillist.0 setmaillist.0 newinclude.0

newaliases: \
load newaliases.o auto_qmail.o token822.o control.o cdbmss.o \
cdbmake.a strerr.a getln.a substdio.a stralloc.a alloc.a error.a \
str.a fs.a seek.a open.a case.a
	./load newaliases auto_qmail.o token822.o control.o \
	cdbmss.o cdbmake.a strerr.a getln.a substdio.a stralloc.a \
	alloc.a error.a str.a fs.a seek.a open.a case.a 

newaliases.0: \
newaliases.1
	nroff -man newaliases.1 > newaliases.0

newaliases.o: \
compile newaliases.c substdio.h strerr.h stralloc.h gen_alloc.h \
getln.h open.h readwrite.h token822.h gen_alloc.h control.h \
auto_qmail.h case.h cdbmss.h cdbmake.h uint32.h substdio.h
	./compile newaliases.c

newinclude: \
load newinclude.o auto_qmail.o token822.o control.o getln.a strerr.a \
stralloc.a env.a alloc.a substdio.a error.a str.a fs.a open.a wait.a \
fd.a
	./load newinclude auto_qmail.o token822.o control.o \
	getln.a strerr.a stralloc.a env.a alloc.a substdio.a \
	error.a str.a fs.a open.a wait.a fd.a 

newinclude.0: \
newinclude.1
	nroff -man newinclude.1 > newinclude.0

newinclude.o: \
compile newinclude.c substdio.h strerr.h stralloc.h gen_alloc.h \
getln.h open.h readwrite.h token822.h gen_alloc.h control.h \
auto_qmail.h env.h
	./compile newinclude.c

open.a: \
makelib open_read.o open_trunc.o
	./makelib open.a open_read.o open_trunc.o

open_read.o: \
compile open_read.c open.h
	./compile open_read.c

open_trunc.o: \
compile open_trunc.c open.h
	./compile open_trunc.c

printforward: \
load printforward.o cdb.a strerr.a substdio.a stralloc.a alloc.a \
error.a str.a
	./load printforward cdb.a strerr.a substdio.a stralloc.a \
	alloc.a error.a str.a 

printforward.0: \
printforward.1
	nroff -man printforward.1 > printforward.0

printforward.o: \
compile printforward.c substdio.h subfd.h substdio.h strerr.h \
stralloc.h gen_alloc.h cdb.h uint32.h
	./compile printforward.c

printmaillist: \
load printmaillist.o getln.a strerr.a substdio.a stralloc.a alloc.a \
error.a str.a
	./load printmaillist getln.a strerr.a substdio.a \
	stralloc.a alloc.a error.a str.a 

printmaillist.0: \
printmaillist.1
	nroff -man printmaillist.1 > printmaillist.0

printmaillist.o: \
compile printmaillist.c substdio.h subfd.h substdio.h strerr.h \
stralloc.h gen_alloc.h getln.h
	./compile printmaillist.c

prog: \
fastforward printforward setforward newaliases printmaillist \
setmaillist newinclude

qmail.o: \
compile qmail.c substdio.h readwrite.h wait.h exit.h fork.h fd.h \
qmail.h substdio.h auto_qmail.h
	./compile qmail.c

scan_ulong.o: \
compile scan_ulong.c scan.h
	./compile scan_ulong.c

seek.a: \
makelib seek_set.o
	./makelib seek.a seek_set.o

seek_set.o: \
compile seek_set.c seek.h
	./compile seek_set.c

setforward: \
load setforward.o cdbmss.o cdbmake.a strerr.a substdio.a stralloc.a \
alloc.a error.a str.a seek.a open.a case.a
	./load setforward cdbmss.o cdbmake.a strerr.a substdio.a \
	stralloc.a alloc.a error.a str.a seek.a open.a case.a 

setforward.0: \
setforward.1
	nroff -man setforward.1 > setforward.0

setforward.o: \
compile setforward.c substdio.h subfd.h substdio.h strerr.h \
stralloc.h gen_alloc.h open.h case.h readwrite.h cdbmss.h cdbmake.h \
uint32.h substdio.h
	./compile setforward.c

setmaillist: \
load setmaillist.o getln.a strerr.a substdio.a stralloc.a alloc.a \
error.a str.a open.a
	./load setmaillist getln.a strerr.a substdio.a stralloc.a \
	alloc.a error.a str.a open.a 

setmaillist.0: \
setmaillist.1
	nroff -man setmaillist.1 > setmaillist.0

setmaillist.o: \
compile setmaillist.c substdio.h subfd.h substdio.h strerr.h \
stralloc.h gen_alloc.h getln.h open.h readwrite.h
	./compile setmaillist.c

setup: \
it install
	./install

sgetopt.o: \
compile sgetopt.c substdio.h subfd.h substdio.h sgetopt.h subgetopt.h \
subgetopt.h
	./compile sgetopt.c

sig.a: \
makelib sig_catch.o sig_pipe.o
	./makelib sig.a sig_catch.o sig_pipe.o

sig_catch.o: \
compile sig_catch.c sig.h hassgact.h
	./compile sig_catch.c

sig_pipe.o: \
compile sig_pipe.c sig.h
	./compile sig_pipe.c

slurpclose.o: \
compile slurpclose.c stralloc.h gen_alloc.h readwrite.h slurpclose.h \
error.h
	./compile slurpclose.c

str.a: \
makelib str_len.o str_diff.o str_diffn.o str_cpy.o str_chr.o \
str_rchr.o byte_chr.o byte_diff.o byte_copy.o byte_cr.o
	./makelib str.a str_len.o str_diff.o str_diffn.o str_cpy.o \
	str_chr.o str_rchr.o byte_chr.o byte_diff.o byte_copy.o \
	byte_cr.o

str_chr.o: \
compile str_chr.c str.h
	./compile str_chr.c

str_cpy.o: \
compile str_cpy.c str.h
	./compile str_cpy.c

str_diff.o: \
compile str_diff.c str.h
	./compile str_diff.c

str_diffn.o: \
compile str_diffn.c str.h
	./compile str_diffn.c

str_len.o: \
compile str_len.c str.h
	./compile str_len.c

str_rchr.o: \
compile str_rchr.c str.h
	./compile str_rchr.c

stralloc.a: \
makelib stralloc_eady.o stralloc_pend.o stralloc_copy.o \
stralloc_opys.o stralloc_opyb.o stralloc_cat.o stralloc_cats.o \
stralloc_catb.o stralloc_arts.o
	./makelib stralloc.a stralloc_eady.o stralloc_pend.o \
	stralloc_copy.o stralloc_opys.o stralloc_opyb.o \
	stralloc_cat.o stralloc_cats.o stralloc_catb.o \
	stralloc_arts.o

stralloc_arts.o: \
compile stralloc_arts.c byte.h str.h stralloc.h gen_alloc.h
	./compile stralloc_arts.c

stralloc_cat.o: \
compile stralloc_cat.c byte.h stralloc.h gen_alloc.h
	./compile stralloc_cat.c

stralloc_catb.o: \
compile stralloc_catb.c stralloc.h gen_alloc.h byte.h
	./compile stralloc_catb.c

stralloc_cats.o: \
compile stralloc_cats.c byte.h str.h stralloc.h gen_alloc.h
	./compile stralloc_cats.c

stralloc_copy.o: \
compile stralloc_copy.c byte.h stralloc.h gen_alloc.h
	./compile stralloc_copy.c

stralloc_eady.o: \
compile stralloc_eady.c alloc.h stralloc.h gen_alloc.h \
gen_allocdefs.h
	./compile stralloc_eady.c

stralloc_opyb.o: \
compile stralloc_opyb.c stralloc.h gen_alloc.h byte.h
	./compile stralloc_opyb.c

stralloc_opys.o: \
compile stralloc_opys.c byte.h str.h stralloc.h gen_alloc.h
	./compile stralloc_opys.c

stralloc_pend.o: \
compile stralloc_pend.c alloc.h stralloc.h gen_alloc.h \
gen_allocdefs.h
	./compile stralloc_pend.c

strerr.a: \
makelib strerr_sys.o strerr_die.o
	./makelib strerr.a strerr_sys.o strerr_die.o

strerr_die.o: \
compile strerr_die.c substdio.h subfd.h substdio.h exit.h strerr.h
	./compile strerr_die.c

strerr_sys.o: \
compile strerr_sys.c error.h strerr.h
	./compile strerr_sys.c

strset.o: \
compile strset.c strset.h uint32.h str.h byte.h
	./compile strset.c

subfderr.o: \
compile subfderr.c readwrite.h substdio.h subfd.h substdio.h
	./compile subfderr.c

subfdins.o: \
compile subfdins.c readwrite.h substdio.h subfd.h substdio.h
	./compile subfdins.c

subfdouts.o: \
compile subfdouts.c readwrite.h substdio.h subfd.h substdio.h
	./compile subfdouts.c

subgetopt.o: \
compile subgetopt.c subgetopt.h
	./compile subgetopt.c

substdi.o: \
compile substdi.c substdio.h byte.h error.h
	./compile substdi.c

substdio.a: \
makelib substdio.o substdi.o substdo.o subfderr.o subfdouts.o \
subfdins.o substdio_copy.o
	./makelib substdio.a substdio.o substdi.o substdo.o \
	subfderr.o subfdouts.o subfdins.o substdio_copy.o

substdio.o: \
compile substdio.c substdio.h
	./compile substdio.c

substdio_copy.o: \
compile substdio_copy.c substdio.h
	./compile substdio_copy.c

substdo.o: \
compile substdo.c substdio.h str.h byte.h error.h
	./compile substdo.c

systype: \
find-systype trycpp.c
	./find-systype > systype

token822.o: \
compile token822.c stralloc.h gen_alloc.h alloc.h str.h token822.h \
gen_alloc.h gen_allocdefs.h
	./compile token822.c

uint32.h: \
tryulong32.c compile load uint32.h1 uint32.h2
	( ( ./compile tryulong32.c && ./load tryulong32 && \
	./tryulong32 ) >/dev/null 2>&1 \
	&& cat uint32.h2 || cat uint32.h1 ) > uint32.h
	rm -f tryulong32.o tryulong32

wait.a: \
makelib wait_pid.o
	./makelib wait.a wait_pid.o

wait_pid.o: \
compile wait_pid.c error.h haswaitp.h
	./compile wait_pid.c
