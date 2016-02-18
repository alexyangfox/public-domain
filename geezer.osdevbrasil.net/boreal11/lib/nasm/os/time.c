#include <time.h> /* time_t */
#include <_syscall.h> /* SYS_... */
#include <_null.h> /* NULL */
/*****************************************************************************
*****************************************************************************/
time_t time(time_t *timer)
{
	_EAX = SYS_TIME;
	__emit__(0xCD, SYSCALL_INT); /* asm int SYSCALL_INT */
	if(timer != NULL)
		*timer = _EAX;
	return _EAX;
}
