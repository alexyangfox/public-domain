#include <_size_t.h>
/*****************************************************************************
*****************************************************************************/
int memcmp(const void *left_p, const void *right_p, size_t count)
{
	const unsigned char *left = (const unsigned char *)left_p;
	const unsigned char *right = (const unsigned char *)right_p;

	for(; count != 0; count--)
	{
		if(*left != *right)
			return *left -  *right;
		left++;
		right++;
	}
	return 0;
}
