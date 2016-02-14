#include <stdarg.h>
#include <kernel/types.h>
#include <kernel/dbg.h>


void ItoA32 (va_list *ap, int32 base, char *buf);
void UItoA32 (va_list *ap, int32 base, char *buf);
void PrintFormattedInteger (char *buf, int32 base, uint32 flags, int32 width,
							void (*printchar_fp)(char, void *), void *ar);


#define ASCII_TO_INT(n)		((n)-48)
#define ASCII_TO_UPPER(n) 	((n)-32)

#define FLAG_ENDFORMAT			(1<<0)
#define FLAG_LEFT				(1<<1)
#define FLAG_SIGN				(1<<2)
#define FLAG_ZERO				(1<<3)
#define FLAG_UPPERCASE			(1<<4)
#define FLAG_ALTERNATE			(1<<5)

#define MIN_INT32					(0x80000000L)


/* DoPrintf();
 *
 * A kernel implementation of the printf() function.  Used by the kernel
 * debugger or other parts of the kernel that needs to print formatted text on
 * the screen.
 *
 *
 * FORMAT SPECIFICATION
 *
 * (%)[-+#0][w](cdiuoxXs%)
 *
 * () - required, [] - optional
 *
 * - : Left justify the output (default is right adjustment, flag width = 10)
 * + : Always display the sign
 * # : Display leading '0x' on hexadecimal numbers and '0' on octal numbers
 * 0 : Pad to the flag width with leading zeros
 *
 * w : decimal digits specifying field width
 *
 * c : Treat argument as an ASCII character
 * d : Treat argument as a signed decimal long integer
 * i : Treat argument as a signed decimal long integer
 * o : Treat argument as a signed octal long integer
 * p : Treat argument as an unsigned 32-bit pointer
 * u : Treat argument as an unsigned decimal long integer
 * x : Treat argument as an unsigned hexadecimal lower-case long integer
 * X : Treat argument as an unsigned hexadecimal upper-case long integer
 * s : Treat argument as a string
 * % : Display a '%' symbol
 *
 * Example %#010x for value of 15 prints  "0x0000000f"
 */

void DoPrintf (void (*printchar_fp)(char, void *),
						void *printchar_arg, const char *format, va_list *ap)
{
	char *c, *s, chr;
	uint32 flags;
	int32 width;
	int32 base;
	void (*uitoa_fp)(va_list *, int32, char *);
	void (*itoa_fp)(va_list *, int32, char *);
	char buf[36];
	
	for (c=format; *c != 0 ; c++)
	{
		if (*c != '%')
		{
			printchar_fp (*c, printchar_arg);
			continue;
		}
		else
			c++;
		
		
		flags = 0x00000000;
		
		do
		{
			switch (*c)
			{
				case '-':
					flags |= FLAG_LEFT;
					c++;
					break;
			
				case '+':
					flags |= FLAG_SIGN;
					c++;
					break;
			
				case '#':
					flags |= FLAG_ALTERNATE;
					c++;
					break;
			
				case '0':
					flags |= FLAG_ZERO;
					c++;
					break;
			
				default:
					flags |= FLAG_ENDFORMAT;
					break;
			}
		} while ((flags & FLAG_ENDFORMAT) == 0);
		
		
		width = 0;
	
		if (('0' <= *c) && (*c <= '9'))
		{
			while (('0' <= *c) && (*c <= '9'))
			{
				width = (width * 10) + ASCII_TO_INT(*c);
				c++;
			}
		}
		else
			width = 1;
		
					
		itoa_fp = &ItoA32;
		uitoa_fp = &UItoA32;

		
		switch (*c)
		{
			case '%':
				printchar_fp ('%', printchar_arg);
				break;

			case 'c':
				chr = (char) va_arg (*ap, int);
				if (chr == '\0') chr = '#';
				printchar_fp (chr, printchar_arg);
				break;
				
			case 's':
				s = va_arg (*ap, char *);
				
				/* Should we support align and field width */
				
				while (*s != '\0')
				{
					printchar_fp (*s, printchar_arg);
					s++;
				}
				break;

			case 'X':
				base = 16;
				flags |= FLAG_UPPERCASE;
				uitoa_fp (ap, base, buf);
				PrintFormattedInteger (buf, base, flags, width,
										printchar_fp, printchar_arg);
				break;

			case 'x':
				base = 16;
				uitoa_fp (ap, base, buf);
				PrintFormattedInteger (buf, base, flags, width,
										printchar_fp, printchar_arg);
				break;

			case 'p':
				base = 16;
				uitoa_fp (ap, base, buf);
				PrintFormattedInteger (buf, base, flags, width,
										printchar_fp, printchar_arg);
				break;
				

			case 'd':
				base = 10;
				itoa_fp (ap, base, buf);
				PrintFormattedInteger (buf, base, flags, width,
										printchar_fp, printchar_arg);
				break;

			case 'i':
				base = 10;
				itoa_fp (ap, base, buf);
				PrintFormattedInteger (buf, base, flags, width,
										printchar_fp, printchar_arg);
				break;

			case 'u':
				base = 10;
				uitoa_fp (ap, base, buf);
				PrintFormattedInteger (buf, base, flags, width,
										printchar_fp, printchar_arg);
				break;

			case 'o':
				base = 8;
				itoa_fp (ap, base, buf);
				PrintFormattedInteger (buf, base, flags, width,
										printchar_fp, printchar_arg);
				break;
		}
	}

	printchar_fp ('\0', printchar_arg);
}




/*
 * ItoA32()
 *
 * Signed Int to ASCII Buffer, LSD-first, buf = [lsd]...[msd][+][0]
 */

void ItoA32 (va_list *ap, int32 base, char *buf)
{
	int32 i=0;   
	char conv[] = {"0123456789abcdef@OVRPHLOW"};
	int32 n;
	char sign;
	int32 carry = 0;
	
	n = va_arg(*ap, int32);
	
	if (n < 0)
	{
		if (n == MIN_INT32)
		{
			carry = 1;
			n = n + 1;
		}
		n = -n;
		sign = '-';
	}
	else
		sign = '+';
	
	
	do
	{	
		if ((i == 0) && (carry == 1))
			buf[i++] = conv[((n % base) + carry) % base];
		else
			buf[i++] = conv[(n+carry) % base];
		
		if (((n % base) + carry) >= base)
			carry = 1;
		else
			carry = 0; 
	}
	while ((n /= base) > 0);	
	
	buf[i++] = sign;
	buf[i] = '\0';
}




/*
 * UItoA32()
 *
 * Unsigned Int to ASCII Buffer, LSD-first, buf = [lsd]...[msd][+][0]
 */

void UItoA32 (va_list *ap, int32 base, char *buf)
{
	int32 i=0;
	char conv[] = {"0123456789abcdef@OVRPHLOW"};
	uint32 n;
	
	n = va_arg(*ap, uint32);
	
	do
	{
		buf[i++] = conv[n % base];
			
	} while ((n /= base) > 0);
	
	buf[i++] = '+';
	buf[i] = '\0';
}




/*
 * PrintFormattedInteger();
 * 
 * Prints buf in the correct direction. Handles flags, padding and field width
 */
				
void PrintFormattedInteger (char *buf, int32 base, uint32 flags, int32 width,
							void (*printchar_fp)(char, void *), void *printchar_arg)
{
	int32 forepadding=0;
	int32 zeropadding=0;
	int32 tailpadding=0;
	char *signstr = "";
	char *annotatestr = "";
	int32 len, i, t;
	char sign;
	char *s;
	
	
	for (i = 0; buf[i] != '\0'; i++);
	
	sign = buf[i-1];
	len = i-1;
	
		
	if (sign == '-')
	{
		len +=1;
		signstr = "-";
	}
	else if (flags & FLAG_SIGN)
	{
		len += 1;
		signstr = (sign == '+') ? "+" : "-";
	}
	
	
	if (flags & FLAG_ALTERNATE)
	{
		if (base == 16)
		{
			len += 2;
			annotatestr = "0x";
		}
		else if (base == 8)
		{
			len += 1;
			annotatestr = "0";
		}
	}
		
	
	if (len < width)
	{
		if (flags & FLAG_ZERO)
			zeropadding = width - len;
		else if ((flags & FLAG_LEFT) == 0)
			forepadding = width - len;			
		else
			tailpadding = width - len;
	}
	
	
	for (t=0; t<forepadding; t++)			/* Space padding */
		printchar_fp (' ', printchar_arg);
	

	for (s = signstr; *s != '\0'; s++)		/* Sign */
		printchar_fp (*s, printchar_arg);


	for (s = annotatestr; *s != '\0'; s++)  /* Base prefix */
		printchar_fp (*s, printchar_arg);
	

	for (t=0; t<zeropadding; t++)			/* Zero padding */
		printchar_fp ('0', printchar_arg);


	for (s = &buf[i-2]; s >= &buf[0]; s--)	/* Digits in Buf */
	{
		if ((flags & FLAG_UPPERCASE) && *s >= 'a'  && *s <= 'z')
			printchar_fp (ASCII_TO_UPPER(*s), printchar_arg);
		else
			printchar_fp (*s, printchar_arg);
	}
		
	
	for (t=0; t<tailpadding; t++)			/* Space padding */
		printchar_fp (' ', printchar_arg);
}
