#include <_syscall.h> /* SYS_... */
/*****************************************************************************
*****************************************************************************/
int select(unsigned handle, unsigned access, unsigned *timeout)
{
	int ret_val;

	__asm__ __volatile__(
		"int %1\n"
		: "=a"(ret_val)		/* EAX		EBX		ECX	EDX */
		: "i"(SYSCALL_INT), "a"(SYS_SELECT), "b"(timeout), "c"(access), "d"(handle));
	return ret_val;
}
