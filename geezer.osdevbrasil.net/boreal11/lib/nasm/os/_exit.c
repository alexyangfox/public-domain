#include <_syscall.h> /* SYS_... */
/*****************************************************************************
*****************************************************************************/
void _exit(int status)
{
	_EBX = status;
	_EAX = SYS_EXIT;
	__emit__(0xCD, SYSCALL_INT); /* asm int SYSCALL_INT */
}
