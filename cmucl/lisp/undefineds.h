/* Routines that must be linked into the core for lisp to work. */
/* $Header: undefineds.h,v 1.8 94/10/30 21:43:03 ram Exp $ */

/* Pick up all the syscalls. */
accept,
access,
acct,
#ifndef hpux
adjtime,
#endif
bind,
brk,
chdir,
chmod,
chown,
chroot,
close,
connect,
creat,
dup,
dup2,
execve,
exit,
fchmod,
fchown,
fcntl,
#if !defined(hpux) && !defined(SVR4)
flock,
#endif
fork,
fstat,
fsync,
ftruncate,
#if !defined(hpux) && !defined(SVR4)
getdtablesize,
#endif
getegid,
geteuid,
getgid,
getgroups,
#ifndef SOLARIS
gethostid,
#endif
gethostname,
getitimer,
#if !defined(hpux) && !defined(SVR4)
getpagesize,
#endif
getpeername,
getpgrp,
getpid,
getppid,
#ifndef SVR4
getpriority,
#endif
getrlimit,
#ifndef SOLARIS
getrusage,
#endif
getsockname,
getsockopt,
gettimeofday,
getuid,
ioctl,
kill,
#ifndef SOLARIS
killpg,
#endif
link,
listen,
lseek,
lstat,
mkdir,
mknod,
mount,
open,
pipe,
profil,
ptrace,
#ifdef mach
quota,
#endif
read,
readlink,
readv,
#ifndef SVR4
reboot,
#endif
recv,
recvfrom,
recvmsg,
rename,
rmdir,
sbrk,
select,
send,
sendmsg,
sendto,
setgroups,
#if !defined(SUNOS) && !defined(SOLARIS)
sethostid,
#endif
#ifndef SVR4
sethostname,
#endif
setitimer,
setpgrp,
#ifndef SVR4
setpriority,
#endif
#ifdef mach
setquota,
#endif
#if !defined(hpux) && !defined(SVR4)
setregid,
setreuid,
#endif
setrlimit,
setsockopt,
settimeofday,
shutdown,
#ifndef SVR4
sigblock,
#endif
sigpause,
#if !defined(ibmrt) && !defined(hpux) && !defined(SVR4)
sigreturn,
#endif
#ifndef SVR4
sigsetmask,
sigstack,
sigvec,
#endif
socket,
socketpair,
stat,
#ifndef SVR4
swapon,
#endif
symlink,
sync,
syscall,
#if defined(hpux) || defined(SVR4)
closedir,
opendir,
readdir,
tcgetattr,
tcsetattr,
#endif
truncate,
umask,
#if !defined(SUNOS) && !defined(parisc) && !defined(SOLARIS)
umount,
#endif
unlink,
#ifndef hpux
utimes,
#endif
#ifndef irix
vfork,
#endif
#ifndef osf1
vhangup,
#endif
wait,
#ifndef SOLARIS
wait3,
#endif
write,
writev,

/* Math routines. */
cos,
sin,
tan,
acos,
asin,
atan,
atan2,
sinh,
cosh,
tanh,
#ifndef hpux
asinh,
acosh,
atanh,
#endif
exp,
#ifndef hpux
expm1,
#endif
log,
log10,
#ifndef hpux
log1p,
#endif
pow,
#ifndef hpux
cbrt,
#endif
sqrt,
hypot,

/* Network support. */
gethostbyname,
gethostbyaddr,

/* Other random things. */
#ifdef SVR4
setpgid,
getpgid,
timezone,
altzone,
daylight,
tzname,
dlopen,
dlsym,
dlclose,
dlerror,
#endif
#ifndef SOLARIS
getwd,
#endif
ttyname

#ifdef irix
,_getpty
#endif
