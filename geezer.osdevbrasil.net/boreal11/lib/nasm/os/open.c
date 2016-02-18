#include <_syscall.h> /* SYS_... */
/*****************************************************************************
*****************************************************************************/
int open(const char *path, unsigned access)
{
	_ECX = access;
	_EBX = (unsigned)path;
	_EAX = SYS_OPEN;
	__emit__(0xCD, SYSCALL_INT); /* asm int SYSCALL_INT */
	return _EAX;
}
