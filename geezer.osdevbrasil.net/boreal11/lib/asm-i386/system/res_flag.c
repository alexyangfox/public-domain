/*****************************************************************************
*****************************************************************************/
void restore_flags(unsigned flags)
{
	__asm__ __volatile__(
		"pushl %0\n"
		"popfl"
		:
		: "m"(flags)
		);
}
