#include <_syscall.h> /* SYS_... */
/*****************************************************************************
*****************************************************************************/
int close(unsigned handle)
{
	int ret_val;

	__asm__ __volatile__(
		"int %1\n"
		: "=a"(ret_val)		/* EAX		EDX */
		: "i"(SYSCALL_INT), "a"(SYS_CLOSE), "d"(handle));
	return ret_val;
}



