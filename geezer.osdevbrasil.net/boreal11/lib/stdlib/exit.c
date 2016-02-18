#include <os.h> /* _exit() */
/*****************************************************************************
Before termination, exit does the following:
- closes all files
- writes buffered output (waiting to be output)
- calls any registered "exit functions" (posted with atexit)

_exit terminates execution without closing any files,
flushing any output, or calling any exit functions.
*****************************************************************************/
void exit(int exit_code)
{
	_exit(exit_code);
}
