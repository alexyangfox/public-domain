#include <kernel/utility.h>




/*
 * Function prototype and Structure passed to DoPrintf();
 */

static void SnprintfPrintChar (char ch, void *arg);

struct SnprintfArg
{
	char *str;
	int size;
	int pos;
};




/*
 * Snprintf();
 */

int Snprintf (char *str, size_t size, const char *format, ...)
{
	va_list ap;

	struct SnprintfArg sa;
	
	va_start (ap, format);
	
	sa.str = str;
	sa.size = size;
	sa.pos = 0;

	DoPrintf (&SnprintfPrintChar, &sa, format, &ap);
	
	va_end (ap);
	
	return sa.pos - 1;
}




/*
 * Vsnprintf();
 */

int Vsnprintf(char *str, size_t size, const char *format, va_list args)
{
	struct SnprintfArg sa;
		
	sa.str = str;
	sa.size = size;
	sa.pos = 0;

	DoPrintf (&SnprintfPrintChar, &sa, format, &args);
	
	return sa.pos - 1;
}




/*
 *
 */

static void SnprintfPrintChar (char ch, void *arg)
{
	struct SnprintfArg *sa;

	sa = (struct SnprintfArg *)arg;
	
	if (sa->pos < sa->size)
		*(sa->str + sa->pos) = ch;
	else
		*(sa->str + sa->size) = '\0';
	
	sa->pos ++;
}








