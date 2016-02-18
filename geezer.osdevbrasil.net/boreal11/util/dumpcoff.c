/*****************************************************************************
Dumps COFF and Win32 PE COFF relocatable and executable files
*****************************************************************************/
#include <stdio.h>

#define min(X,Y)	(((X) < (Y)) ? (X) : (Y))

typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;

static char g_sh, g_win32;
static unsigned (*g_read_16)(char *buf);
static unsigned long (*g_read_32)(char *buf);
/*****************************************************************************
*****************************************************************************/
unsigned swap16(unsigned arg)
{
	return ((arg >> 8) & 0x00FF) | ((arg << 8) & 0xFF00);
}
/*****************************************************************************
*****************************************************************************/
unsigned long swap32(unsigned long arg)
{
	return ((arg >> 24) & 0x00FF) | ((arg >> 8) & 0xFF00) |
		((arg << 8) & 0x00FF0000L) | ((arg << 24) & 0xFF000000L);
}
/*****************************************************************************
*****************************************************************************/
#if 1 /* little endian CPU like Intel x86 */
static unsigned read_le16(char *buf)
{
	return *(uint16_t *)buf;
}
/*****************************************************************************
*****************************************************************************/
static unsigned read_be16(char *buf)
{
	return swap16(*(uint16_t *)buf);
}
/*****************************************************************************
*****************************************************************************/
static unsigned long read_le32(char *buf)
{
	return *(uint32_t *)buf;
}
/*****************************************************************************
*****************************************************************************/
static unsigned long read_be32(char *buf)
{
	return swap32(*(uint32_t *)buf);
}
/*****************************************************************************
*****************************************************************************/
#else /* big endian CPU like Motorola 680x0 */
static unsigned read_le16(char *buf)
{
	return swap16(*(uint16_t *)buf);
}
/*****************************************************************************
*****************************************************************************/
static unsigned read_be16(char *buf)
{
	return *(uint16_t *)buf;
}
/*****************************************************************************
*****************************************************************************/
static unsigned long read_le32(char *buf)
{
	return swap32(*(uint32_t *)buf);
}
/*****************************************************************************
*****************************************************************************/
static unsigned long read_be32(char *buf)
{
	return *(uint32_t *)buf;
}
#endif
/*****************************************************************************
xxx - dump imports and base relocations for Win32 PE COFF executable, too:

typedef struct
{
//	uint16_t magic;
//	uint16_t version;
//	uint32_t code_size;
//	uint32_t data_size;
//	uint32_t bss_size;
//	uint32_t entry;
//	uint32_t code_offset;
//	uint32_t data_offset;
//
//	uint32_t image_base;
	uint32_t res0[18];
	uint32_t import_table_adr;
	uint32_t import_table_size;
	uint32_t res1[6];
	uint32_t reloc_table_adr;
	uint32_t reloc_table_size;
} pe_file_t;
*****************************************************************************/
static int dump_coff_file(const char *in_name)
{
	unsigned long sect_tab_off, entry, image_base;
	char file_hdr[64], buf[144], sect_hdr[40];
	unsigned aout_hdr_size, s, r, i;
	FILE *in;

/* open input file */
	in = fopen(in_name, "rb");
	if(in == NULL)
	{
		printf("Can't open file '%s'\n", in_name);
		return -1;
	}
/* read 20-byte COFF file header */
	if(fread(file_hdr, 1, 20, in) != 20)
SHORT:	{
		printf("File '%s' is not COFF (too short)\n", in_name);
		fclose(in);
		return -1;
	}
/* identify COFF sub-type */
	if(read_be16(file_hdr + 0) == 0x150) /* coff-m68k */
	{
		printf("Big endian COFF magic=0x150 (m68k)\n");
		g_read_16 = read_be16;
		g_read_32 = read_be32;
	}
	else if(read_le16(file_hdr + 0) == 0x14C) /* coff-go32 */
	{
		printf("Little endian COFF magic=0x14C (i386)\n");
		g_read_16 = read_le16;
		g_read_32 = read_le32;
	}
	else if(read_be16(file_hdr + 0) == 0x500) /* coff-sh */
	{
		printf("Big endian COFF magic=0x500 (sh)\n");
		g_read_16 = read_be16;
		g_read_32 = read_be32;
/* relocation records are 16 bytes each instead of 10
Someone at Hitachi forget to pack their structs? */
		g_sh = 1;
	}
	else if(read_be16(file_hdr + 0) == 0x701) /* coff-arm */
	{
		printf("Big endian COFF magic=0x701 (arm)\n");
		g_read_16 = read_be16;
		g_read_32 = read_be32;
	}
/* check if Win32 PE COFF executable */
	else
	{
		unsigned long new_exe_off;

/* DOS .EXE magic */
		if(file_hdr[0] != 'M' || file_hdr[1] != 'Z')
NOT:		{
			printf("File '%s' is not COFF\n", in_name);
			fclose(in);
			return -1;
		}
/* is the header big enough (64 bytes == 4 paragraphs)
to contain the 'New Executable' file offset? */
		if(read_le16(file_hdr + 8) < 4)
			goto NOT;
/* read another 64 bytes of the header */
		if(fread(file_hdr + 20, 1, 44, in) != 44)
			goto NOT;
		new_exe_off = read_le32(file_hdr + 60);
		if(new_exe_off == 0)
			goto NOT;
/* PE magic */
		fseek(in, new_exe_off, SEEK_SET);
		if(fread(file_hdr, 1, 4, in) != 4)
			goto NOT;
		if(file_hdr[0] != 'P' || file_hdr[1] != 'E' ||
			file_hdr[2] != 0 || file_hdr[3] != 0)
				goto NOT;
/* NOW the COFF header... */
		if(fread(file_hdr, 1, 20, in) != 20)
			goto NOT;
		if(read_le16(file_hdr + 0) != 0x14C) /* coff-go32 */
			goto NOT;
		printf("Win32 PE COFF executable\n");
		g_read_16 = read_le16;
		g_read_32 = read_le32;
		g_win32 = 1;
	}
	printf("COFF file flags: 0x%08X\n", g_read_16(file_hdr + 18));
/* executable COFF files also have an aout header */
	aout_hdr_size = g_read_16(file_hdr + 16);
	sect_tab_off = ftell(in) + aout_hdr_size;
	if(aout_hdr_size != 0)
	{
		i = min(144, aout_hdr_size);
		if(fread(buf, 1, i, in) != i)
ERR:		{
			printf("Error reading COFF file '%s'\n", in_name);
			fclose(in);
			return -1;
		}
/* xxx - is this different for non-x86 COFFs? */
		if(g_read_16(buf + 0) != 0x010B)
		{
			printf("*** Warning: aout magic is 0x%X; "
				"expected 0x10B\n", g_read_16(buf + 0));
		}
		entry = g_read_32(buf + 16);
		printf("Entry=0x%lX", entry);
		if(g_win32)
		{
			if(aout_hdr_size < 144)
			{
				printf("*** Warning: strange aout header "
					"size %u for Win32 file\n", aout_hdr_size);
			}
			else
			{
				image_base = read_le32(buf + 28);
				printf(", image base (IB)=0x%lX, entry plus "
					"IB=0x%lX", image_base,
					entry + image_base);
			}
		}
		printf("\n");
	}
/* dump section headers
xxx - I may have raw data size and virtual data size switched */
	if(g_win32)
	{
		printf("              raw     virt  virtual                           num\n");
		printf("           (file)    (mem)  address                     num  line\n");
		printf(" section     size     size    (VMA)   VMA+IB   offset reloc  nums\n");
		printf("-------- -------- -------- -------- -------- -------- ----- -----\n");
	}
	else
	{
		printf("                  physical  virtual                  num\n");
		printf("                   address  address            num  line\n");
		printf(" section     size    (LMA)    (VMA)   offset reloc  nums\n");
		printf("-------- -------- -------- -------- -------- ----- -----\n");
	}
	fseek(in, sect_tab_off, SEEK_SET);
	for(s = 0; s < g_read_16(file_hdr + 2); s++)
	{
		if(fread(sect_hdr, 1, 40, in) != 40)
			goto ERR;
		if(g_win32)
			printf("%8.8s %8lX %8lX %8lX %8lX %8lX %5u %5u\n",
			sect_hdr + 0, g_read_32(sect_hdr + 16),
			g_read_32(sect_hdr + 8), g_read_32(sect_hdr + 12),
			g_read_32(sect_hdr + 12) + image_base,
			g_read_32(sect_hdr + 20),
			g_read_16(sect_hdr + 32), g_read_16(sect_hdr + 34));
		else
			printf("%8.8s %8lX %8lX %8lX %8lX %5u %5u\n",
			sect_hdr + 0, g_read_32(sect_hdr + 16),
			g_read_32(sect_hdr + 8), g_read_32(sect_hdr + 12),
			g_read_32(sect_hdr + 20), g_read_16(sect_hdr + 32),
			g_read_16(sect_hdr + 34));
// printf("section flags=0x%lX\n", g_read_32(sect_hdr + 36));
	}
/* dump object file relocations */
	for(s = 0; s < g_read_16(file_hdr + 2); s++)
	{
		fseek(in, sect_tab_off + 40 * s, SEEK_SET);
		if(fread(sect_hdr, 1, 40, in) != 40)
			goto ERR;
		i = g_read_16(sect_hdr + 32);
		if(i == 0)
			continue;
		printf("Section %u has %u relocations\n", s, i);
		i = (g_sh ? 16 : 10);
/* for each relocation... */
		for(r = 0; r < g_read_16(sect_hdr + 32); r++)
		{
			fseek(in, g_read_32(sect_hdr + 24) + r * i, SEEK_SET);
			if(fread(buf, 1, i, in) != i)
				goto ERR;
			printf("sect %1u, reloc %3u: adr=0x%08lX, type=%u\n",
				s, r, g_read_32(buf + 0), g_read_16(buf + 8));
		}
	}
	fclose(in);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	unsigned i;

	if(arg_c < 2)
	{
		printf("COFF file dumper\n");
		return 1;
	}
	for(i = 1; i < arg_c; i++)
		(void)dump_coff_file(arg_v[i]);
	return 0;
}
