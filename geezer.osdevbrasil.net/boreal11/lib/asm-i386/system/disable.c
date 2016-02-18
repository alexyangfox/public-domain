/*****************************************************************************
*****************************************************************************/
unsigned disable(void)
{
	unsigned ret_val;

	__asm__ __volatile__(
		"pushfl\n"
		"popl %0\n"
		"cli"
		: "=a"(ret_val)
		:);
	return ret_val;
}
