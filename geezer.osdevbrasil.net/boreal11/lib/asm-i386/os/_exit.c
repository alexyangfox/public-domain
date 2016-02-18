#include <_syscall.h> /* SYS_... */
/*****************************************************************************
*****************************************************************************/
void _exit(int status)
{
	__asm__ __volatile__(
		"int %0\n"
		:			/* EAX */
		: "i"(SYSCALL_INT), "a"(SYS_EXIT), "b"(status));
}
