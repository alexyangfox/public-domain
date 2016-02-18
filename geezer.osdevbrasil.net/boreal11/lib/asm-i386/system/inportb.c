/*****************************************************************************
*****************************************************************************/
unsigned char inportb(unsigned port)
{
	unsigned ret_val;

	__asm__ __volatile__(
		"inb %w1,%b0"
		: "=a"(ret_val)
		: "d"(port));
	return ret_val;
}
