#include <kernel/types.h>
#include <kernel/utility.h>
#include <kernel/kmalloc.h>
#include <kernel/vm.h>






/*
 * StrDup();
 */
 
char *StrDup (const char *src)
{
	char *dst;
	int len;
	
	
	len = StrLen (src) + 1;
	
	if ((dst = KMalloc (len)) != NULL)
	{
		MemCpy (dst, src, len);
		return dst;
	}
	
	return NULL;
}




/*
 * StrLen();
 */
 
size_t StrLen (const char *s)
{
	size_t len;
	
	for (len = 0; *s++ != '\0'; len++);
	return len;
}




/*
 * StrCmp();
 */

int StrCmp (const char *s, const char *t)
{
	while (*s == *t)
	{
		/* Should it not return the difference? */
	
		if (*s == '\0')
		{
			return *s - *t;
			/* return 0; */
		}
		
		 s++;
		 t++;
	}
	
	return *s - *t;
}




/* 
 * StrChr();
 */

char *StrChr (char *str, char ch)
{
	char *c = str;
	
	while (*c != '\0')
	{
		if (*c == ch)
			return c;
		
		c++;
	}
	
	return NULL;
}




/*
 * AtoI(); 
 *
 * FIXME:  Improve AtoI();
 *
 * Simple ASCII to Integer.  Doesn't handle signs or space
 * around number.
 *
 * Create StrToL/StrToUL ????????
 */

int AtoI (const char *str)
{
	const char *ch = str;
	int val = 0;
	
	while (*ch >= '0' &&  *ch <= '9')
	{
		val = (val * 10) + (*ch - '0');
		ch++;
	}
	
	return val;
}

