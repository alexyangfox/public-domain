#include <_syscall.h> /* SYS_... */
/*****************************************************************************
*****************************************************************************/
int select(unsigned handle, unsigned access, unsigned *timeout)
{
	_EDX = handle;
	_ECX = access;
	_EBX = (unsigned)timeout;
	_EAX = SYS_SELECT;
	__emit__(0xCD, SYSCALL_INT); /* asm int SYSCALL_INT */
	return _EAX;
}
