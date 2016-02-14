#include "kernel.h"

size_t strlen(const char *s)
{
	size_t i;
	
	for(i = 0; *s != '\0'; s++)
	{
		i++;
	}
	return i;
}

char *dtoc(int value, char buf[])
{
	char hex[18] = "0123456789ABCDEFNS";
	
	if( !((value >= 0) && (value <= 15)) )
	{
		if(value < 0)
		{
			buf[0] = hex[16];
			return buf;
		}
		else
		{
			buf[0] = hex[17];
			return buf;
		}
	}
	
	buf[0] = hex[value];

	return buf;
}

char *itoa(long value, char buf[], int base)
{
	long current = value;
	int pos = 0;
	base--; /* Bases will implemented later - stop GCC complaining! */
	int digits[9] = {0};
	
	while(current)
	{
		digits[pos] = current % 16;
		current /= 16;
		pos++;
	}

	char mbuf[1];

	for(int i = 8; i >= 0; i--)
	{
		int pos = 8 - i;
		dtoc(digits[pos], mbuf);
		buf[i] = mbuf[0];
	}
	
	return buf;
}
