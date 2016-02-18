#ifndef __TL__SYSCALL_H
#define	__TL__SYSCALL_H

#ifdef __cplusplus
extern "C"
{
#endif

#define	SYSCALL_INT	0x20

#define	SYS_OPEN	0
#define	SYS_CLOSE	1
#define	SYS_WRITE	2
#define	SYS_READ	3
#define	SYS_SELECT	4
#define	SYS_TIME	5
#define	SYS_EXIT	6

#define	SYS_IOCTL	7
#define	SYS_STAT	8
#define	SYS_MMAP	9
#define	SYS_SLEEP	10
#define	SYS_SBRK	11

#ifdef __cplusplus
}
#endif

#endif
