#include <_null.h>
/*****************************************************************************
*****************************************************************************/
const char *strstr(const char *s1, const char *s2)
{
	const char *start_s1 = NULL;
	const char *in_s2 = NULL;

	for(; *s1 != '\0'; s1++)
	{
		if(start_s1 == NULL)
		{
/* first char of match */
			if(*s1 == *s2)
			{
/* remember start of matching substring in s1 */
				start_s1 = s1;
				in_s2 = s2 + 1;
/* done already? */
				if(*in_s2 == '\0')
					return start_s1;
			}
/* continued mis-match
			else
				nothing ; */
		}
		else
		{
/* continued match */
			if(*s1 == *in_s2)
			{
				in_s2++;
/* done */
				if(*in_s2 == '\0')
					return start_s1;
			}
			else
/* first char of mis-match */
				start_s1 = NULL;
		}
	}
	return NULL;
}
