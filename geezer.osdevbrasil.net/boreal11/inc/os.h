#ifndef __TL_OS_H
#define	__TL_OS_H

#ifdef __cplusplus
extern "C"
{
#endif

#define	O_RDONLY	0x01
#define	O_WRONLY	0x02
#define	O_RDWR		(O_RDONLY | O_WRONLY)

int open(const char *path, unsigned access);
int close(unsigned handle);
int write(unsigned handle, void *buf_p, unsigned len);
int read(unsigned handle, void *buf_p, unsigned want);
int select(unsigned handle, unsigned access, unsigned *timeout);
/* time() is defined in TIME.H */
void _exit(int exit_code);

//int ioctl(unsigned handle, unsigned opcode, unsigned arg);
//unsigned long sbrk(long delta);

#ifdef __cplusplus
}
#endif

#endif
