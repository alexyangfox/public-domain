#include <system.h> /* inportb(), outportb(), disable() */
#include <stdio.h>/* printf() */
#include <os.h> /* read() */
/*****************************************************************************
It's surprisingly difficult to prevent GCC from optimizing this function
into an infinite JMP loop, like this:

	400010:	55	push   %ebp
	400011:	89 e5	  mov    %esp,%ebp
	400013:	eb fe	  jmp    400013 <_recurse+0x3>
*****************************************************************************/
int recurse(int arg); /* forward */

int recurse2(int arg)
{
	static int salt;

	salt++;
	return recurse(arg + salt);
}
/*****************************************************************************
*****************************************************************************/
int recurse(int arg)
{
	static int salt;

	salt--;
	return recurse2(arg) - salt;
}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	unsigned char key;

	printf("\n\nWhere do you want it to blow today? :)\n"
		"1. Chose an illegal operation from the list below\n"
		"2. Press the corresponding key\n"
		"3. See if app crashes and burns\n\n");
	printf(	"[1] Read memory location 0xFFFF0000\n"
		"[2] Write memory location 0xFFFF0000\n"
		"[3] Read memory location 0 (NULL pointer)\n"
		"[4] Write memory location 0 (NULL pointer)\n"
		"[5] Read from I/O port 0x80\n"
		"[6] Write to I/O port 0x80\n"
		"[7] Infinite recursive call causing stack overflow\n"
		"[8] Stack underflow\n"
		"[9] disable() (CLI)\n"
		"[0] Divide by zero\n");
	while(1)
	{
		if(read(0, &key, 1) != 1)
			continue;
		printf("Key pressed was '%c'\n", key);
		switch(key)
		{
			case '1':
				key = *(volatile unsigned char *)0xFFFF0000;
				break;
			case '2':
				*(volatile unsigned char *)0xFFFF0000 = 0;
				break;
			case '3':
				key = *(volatile unsigned char *)0;
				break;
			case '4':
				*(volatile unsigned char *)0 = 0;
				break;
			case '5':
				(void)inportb(0x80);
				break;
			case '6':
				outportb(0x80, 0);
				break;
			case '7':
				recurse(42);
				break;
			case '8':
				while(1)
					__asm__ __volatile__("pop %eax");
				break;
			case '9':
				disable();
				break;
			case '0':
				printf("%u\n", key / 0);
				break;
			default:
				printf("Invalid key, try again\n");
				goto SKIP;
		}
		printf("Hey, it worked! Better fix the kernel\n");
SKIP:
	}
	return 0;
}
