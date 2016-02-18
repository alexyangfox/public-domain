/*****************************************************************************
*****************************************************************************/
int strncmp(const char *str1, const char *str2, unsigned max_len)
{
/* 	while((*str2 != '\0') && (*str1 == *str2) && max_len != 0) */
	while((*str2 != '\0') && (*str1 == *str2) && max_len > 1)
	{
		str1++;
		str2++;
		max_len--;
	}
	return *str1 -  *str2;
}
