#include <_syscall.h> /* SYS_... */
/*****************************************************************************
*****************************************************************************/
int read(unsigned handle, void *buf, unsigned len)
{
	int ret_val;

	__asm__ __volatile__(
		"int %1\n"
		: "=a"(ret_val)		/* EAX		EBX	ECX	EDX */
		: "i"(SYSCALL_INT), "a"(SYS_READ), "b"(buf), "c"(len), "d"(handle));
	return ret_val;
}
