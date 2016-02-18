/*----------------------------------------------------------------------------
Hex dump of hard disk sectors
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
This code is public domain (no copyright).
You can do whatever you want with it.
Initial release: March 30, 2008

Compile with Turbo C or Borland C for DOS.
----------------------------------------------------------------------------*/
#include <stdio.h>
#include <conio.h>
#include <bios.h>
#include <dos.h>

#if defined(__TURBOC__)
#if __TURBOC__<0x300	/* Turbo C++ 1.01 */
#define _DISK_READ	2
#endif
#endif
/*****************************************************************************
*****************************************************************************/
#define BPERL		16	/* byte/line for dump */

static void dump(void *data_p, unsigned count)
{
	unsigned char *data = (unsigned char *)data_p;
	unsigned byte1, byte2;

	while(count != 0)
	{
		for(byte1 = 0; byte1 < BPERL; byte1++)
		{
			if(count == 0)
				break;
			printf("%02X ", data[byte1]);
			count--;
		}
		printf("\t");
		for(byte2 = 0; byte2 < byte1; byte2++)
		{
			if(data[byte2] < ' ')
				printf(".");
			else
				printf("%c", data[byte2]);
		}
		printf("\n");
		data += BPERL;
	}
}
/*****************************************************************************
*****************************************************************************/
static unsigned long read_le32(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] + buf[1] * 0x100u + buf[2] * 0x10000L
		+ buf[3] * 0x1000000L;
}
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	int nc, nh, ns, c = 0, h = 0, s = 1;
	unsigned char buf[512], *p;
	unsigned long lba, size;
	unsigned drive, i;
	union REGS regs;

	if(arg_c != 2)
	{
		printf("Hex dump of hard disk sectors.\nSpecify INT 13h"
			" drive number of hard disk (0x80-0xFF)\n");
		return 1;
	}
	sscanf(arg_v[1], "%x", &drive);
	if(drive < 0x80 || drive >= 0x100)
	{
		printf("Invalid drive number 0x%X -- must be 0x80-0xFF\n",
			drive);
		return 1;
	}
/* get number of heads and sectors/track via INT 13h BIOS call */
	regs.h.ah = 0x08;
	regs.h.dl = drive;
	int86(0x13, &regs, &regs);
	if(regs.x.cflag & 0x0001)
	{
		printf("Error: can't get CHS geometry for drive 0x%02X\n",
			drive);
		return 2;
	}
	nh = regs.h.dh;
	ns = regs.h.cl & 0x3F;
	nc = regs.h.cl & 0xC0;
	nc <<= 2;
	nc |= regs.h.ch;
	clrscr();
	printf("Drive 0x%02X: %u heads, %u sectors/track\n", drive, nh, ns);
/* read MBR/partition table */
	if(biosdisk(_DISK_READ, drive, 0, 0, 1, 1, buf))
	{
		printf("Error: can't reading partition table\n");
		return 1;
	}
/* display primary partition info */
	printf(	"Primary partitions:\n"
		"       -------- start ------- -------- end --------\n"
		"# type LBA       CHS          LBA       CHS\n"
		"- ---- --------- ------------ --------- ------------\n");
	for(i = 0; i < 4; i++)
	{
		unsigned pc, ph, ps;

/* point to entry in partition table */
		p = &buf[446 + 16 * i];
/* partition type 0 means none defined */
		if(p[4] == 0)
			continue;
/* LBA start of partition and size */
		lba  = read_le32(&p[8]);
		size = read_le32(&p[12]);
/* CHS start of partition */
		pc = p[2] & 0xC0;
		pc <<= 2;
		pc |= p[3];
		ph = p[1];
		ps = p[2] & 0x3F;
		printf("%1u 0x%02X %9lu %4u:%3u:%3u", i, p[4], lba,
			pc, ph, ps);
/* display bootsector of first primary partition, if one's defined
(else display the MBR) */
		if(c == 0 && h == 0 && s == 1)
		{
			c = pc;
			h = ph;
			s = ps;
		}
/* CHS end of partition */
		pc = p[6] & 0xC0;
		pc <<= 2;
		pc |= p[7];
		ph = p[5];
		ps = p[6] & 0x3F;
		printf(" %9lu %4u:%3u:%3u\n", lba + size - 1,
			pc, ph, ps);
	}
	printf(	"\nLeft/right arrow keys increment/decrement by sector\n"
		"Up/down    arrow keys increment/decrement by head\n"
		"PgUp/PgDn        keys increment/decrement by cylinder\n"
		"Esc quits\n\n");
	while(1)
	{
		unsigned x, y, key;

		x = wherex();
		y = wherey();
		printf("Cylinder=%u, Head=%u, Sector=%u        \n", c, h, s);
		if(biosdisk(_DISK_READ, drive, h, c, s, 1, buf))
			printf("Error reading sector!\n");
		else
			dump(buf, 192);
		gotoxy(x, y);
		key = getch();
		if(key == 0)
			key = 0x100 | getch();
		if(key == 27)
			break;
/* left arrow decrements sector */
		else if(key == 0x14B)
		{
			if(s > 1)
				s--;
			else
			{
				if(h > 0)
				{
					s = ns;
					h--;
				}
				else
				{
					if(c > 0)
					{
						s = ns;
						h = nh - 1;
						c--;
					}
				}
			}
		}
/* right arrow decrements sector */
		else if(key == 0x14D)
		{
			if(s < ns)
				s++;
			else
			{
				if(h + 1 < nh)
				{
					s = 1;
					h++;
				}
				else
				{
					if(c + 1 < nc)
					{
						s = 1;
						h = 0;
						c++;
					}
				}
			}
		}
/* down arrow decrements head */
		else if(key == 0x150)
		{
			if(h > 0)
				h--;
			else
			{
				if(c > 0)
				{
					h = nh - 1;
					c--;
				}
			}
		}
/* up arrow increments head */
		else if(key == 0x148)
		{
			if(h + 1 < nh)
				h++;
			else
			{
				if(c + 1 < nc)
				{
					h = 0;
					c++;
				}
			}
		}
/* PgDn decrements cylinder */
		else if(key == 0x151)
		{
			if(c > 0)
				c--;
		}
/* PgUp increments cylinder */
		else if(key == 0x149)
		{
			if(c + 1 < nc)
				c++;
		}
	}
	return 0;
}

