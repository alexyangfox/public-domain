#include <_syscall.h> /* SYS_... */
/*****************************************************************************
*****************************************************************************/
int close(unsigned handle)
{
	_EDX = handle;
	_EAX = SYS_CLOSE;
	__emit__(0xCD, SYSCALL_INT); /* asm int SYSCALL_INT */
	return _EAX;
}



