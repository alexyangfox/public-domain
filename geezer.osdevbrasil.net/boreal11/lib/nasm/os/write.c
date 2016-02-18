#include <_syscall.h> /* SYS_... */
/*****************************************************************************
*****************************************************************************/
int write(unsigned handle, void *buf, unsigned len)
{
	_EDX = handle;
	_ECX = len;
	_EBX = (unsigned)buf;
	_EAX = SYS_WRITE;
	__emit__(0xCD, SYSCALL_INT); /* asm int SYSCALL_INT */
	return _EAX;
}
