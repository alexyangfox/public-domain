#include <_syscall.h> /* SYS_... */
/*****************************************************************************
*****************************************************************************/
int open(const char *path, unsigned access)
{
	int ret_val;

	__asm__ __volatile__(
		"int %1\n"
		: "=a"(ret_val)		/* EAX		EBX	ECX */
		: "i"(SYSCALL_INT), "a"(SYS_OPEN), "b"(path), "c"(access));
	return ret_val;
}
