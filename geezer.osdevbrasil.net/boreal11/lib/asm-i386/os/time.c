#include <time.h> /* time_t */
#include <_syscall.h> /* SYS_... */
#include <_null.h> /* NULL */
/*****************************************************************************
*****************************************************************************/
time_t time(time_t *timer)
{
	time_t ret_val;

	__asm__ __volatile__(
		"int %1\n"
		: "=a"(ret_val)		/* EAX */
		: "i"(SYSCALL_INT), "a"(SYS_TIME));
	if(timer != NULL)
		*timer = ret_val;
	return ret_val;
}
