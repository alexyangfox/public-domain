/*----------------------------------------------------------------------------
Zeroes un-initialized portions of DJGPP COFF executable files
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: Sep 21, 2002
This code is public domain (no copyright).
You can do whatever you want with it.

Revised Apr 14, 2004
- got rid of dependency on COFF.H
- replaced all COFF data structures with original code


This program has not been well-tested. Before using this program, either
- make a backup copy of the original executable, or
- be prepared to re-build the executable from source code

I assume the file contents are stored in this order:
- .EXE, COFF, and a.out file headers, and section headers
  (for this program, the .EXE stub is optional)
- "raw data" for each section
- optional line numbers, for each section that has them
- optional symbol table and string table
----------------------------------------------------------------------------*/
/* FILE, SEEK_..., NULL, EOF, L_tmpnam, printf(), tmpnam(), remove(),
rename(), fopen(), fseek(), ftell(), fread(), fgetc(), fputc(), fclose() */
#include <stdio.h>

#if 1
#define	DEBUG(X)	X
#else
#define	DEBUG(X)	/* nothing */
#endif

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned long		uint32_t;

#pragma pack(1)
typedef struct
{
	uint16_t magic;
	uint16_t num_sects;
	uint32_t time_date;
	uint32_t symtab_offset;
	uint32_t num_syms;
	uint16_t aout_hdr_size;
	uint16_t flags;
/* for executable COFF file, a.out header starts here */
} coff_file_t; /* 20 bytes */

/* Win32 PE COFF a.out header is longer than this */
#pragma pack(1)
typedef struct
{
	uint16_t magic;
	uint16_t version;
	uint32_t code_size;
	uint32_t data_size;
	uint32_t bss_size;
	uint32_t entry;
	uint32_t code_offset;
	uint32_t data_offset;
} coff_aout_t; /* 28 bytes */

#pragma pack(1)
typedef struct
{
	char name[8];
	uint32_t phys_adr;
	uint32_t virt_adr;
	uint32_t size;
	uint32_t offset;
	uint32_t relocs_offset;
	uint32_t line_nums_offset;
	uint16_t num_relocs;
	uint16_t num_line_nums;
	uint32_t flags;
} coff_sect_t; /* 40 bytes */

/* partial .EXE file header */
#pragma pack(1)
typedef struct
{
	char magic[2];
	uint16_t mod_file_size;
	uint16_t file_size;
	uint16_t num_relocs;
	uint16_t hdr_size;
} exe_hdr_t;
/*****************************************************************************
*****************************************************************************/
static void zero_region(FILE *file, unsigned long start, unsigned long end)
{
	if(end <= start)
		return;
	fseek(file, start, SEEK_SET);
	for(end -= start; end != 0; end--)
		fputc(0, file);
}
/*****************************************************************************
*****************************************************************************/
static int tidy_coff(const char *file_name)
{
	unsigned long coff_pos, sect_pos, pos1, pos2;
	char outfile_name[L_tmpnam];
	FILE *infile, *outfile;
	coff_file_t coff_hdr;
	coff_aout_t aout_hdr;
	const char *err_msg;
	exe_hdr_t exe_hdr;
	coff_sect_t sect;
	int i;

	DEBUG(printf("File '%s':\n", file_name);)
/* open input and (temporary) output files */
	infile = fopen(file_name, "rb");
	if(infile == NULL)
	{
		printf("Can't open file '%s'\n", file_name);
		return -1;
	}
	tmpnam(outfile_name);
	outfile = fopen(outfile_name, "wb");
	if(outfile == NULL)
	{
		printf("Can't open temporary output file '%s'\n",
			outfile_name);
		fclose(infile);
		return -1;
	}
/* copy entire input file to output file
isn't there a library function for this...? */
	while(1)
	{
		i = fgetc(infile);
		if(i == EOF)
			break;
		fputc(i, outfile);
	}
	fseek(infile, 0, SEEK_SET);
/* .EXE FILE HEADER */
	if(fread(&exe_hdr, 1, sizeof(exe_hdr), infile) != sizeof(exe_hdr))
	{
		err_msg = "File is not COFF (too short)";
ERR:
		printf("Error reading file '%s': %s\n", file_name, err_msg);
		fclose(infile);
		fclose(outfile);
		remove(outfile_name);
		return -2;
	}
	if(exe_hdr.magic[0] == 'M' && exe_hdr.magic[1] == 'Z')
	{
		coff_pos = exe_hdr.file_size * 512uL;
		if(exe_hdr.mod_file_size != 0)
			coff_pos = coff_pos - 512 + exe_hdr.mod_file_size;
	}
/* if no .EXE header, assume unstubbed COFF file */
	else
		coff_pos = 0;
/* COFF FILE HEADER */
	fseek(infile, coff_pos, SEEK_SET);
	if(fread(&coff_hdr, 1, sizeof(coff_hdr), infile) != sizeof(coff_hdr))
	{
		err_msg = "File is not COFF (too short)";
		goto ERR;
	}
	if(coff_hdr.magic != 0x14C)
	{
		err_msg = "File is not COFF (bad COFF magic value)";
		goto ERR;
	}
	if(coff_hdr.aout_hdr_size == 0)
	{
		err_msg = "Missing a.out header";
		goto ERR;
	}
	if((coff_hdr.flags & 0x0002) == 0)
	{
		err_msg = "Executable bit not set";
		goto ERR;	/* could be relocatable (.o) file */
	}
/* AOUT FILE HEADER */
	if(fread(&aout_hdr, 1, sizeof(aout_hdr), infile) != sizeof(aout_hdr))
	{
		err_msg = "Error reading a.out header";
		goto ERR;
	}
	if(aout_hdr.magic != 0x10B)
	{
		err_msg = "Bad a.out magic value";
		goto ERR;
	}
/* SECTION RAW DATA */
	sect_pos = coff_pos + sizeof(coff_hdr) + coff_hdr.aout_hdr_size;
	DEBUG(
	if(coff_pos != 0)
		printf(".EXE and COFF file headers:\t0-0x%lX\n", sect_pos);
	else
		printf("COFF file headers:\t\t0-0x%lX\n", sect_pos);
	)
/* point to end of section header table
If no gap, this is start of raw data for first section */
	pos1 = sect_pos + coff_hdr.num_sects * 40;
	DEBUG(printf("Section headers:\t\t0x%lX-0x%lX\n", sect_pos, pos1);)
	fseek(infile, sect_pos, SEEK_SET);
	DEBUG(printf("Section raw data:\n");)
	for(i = coff_hdr.num_sects; i != 0; i--)
	{
		if(fread(&sect, 1, 40, infile) != 40)
		{
			err_msg = "Error reading section header";
			goto ERR;
		}
		DEBUG(printf("\tSection '%-8.8s':\t", sect.name);)
/* skip BSS and sections of zero size */
		if((sect.flags & 0x80) || sect.size == 0)
		{
			DEBUG(printf("\n");)
			continue;
		}
/* handle possible gap before raw data */
		pos2 = coff_pos + sect.offset;
		DEBUG(printf("0x%lX-0x%lX\n", pos2, pos2 + sect.size);)
		if(pos1 < pos2)
		{
			DEBUG(printf("--- zero-filling gap before raw data "
				"of section '%-8.8s': 0x%lX-0x%lX\n",
				sect.name, pos1, pos2);)
			zero_region(outfile, pos1, pos2);
		}
		pos1 = pos2 + sect.size;
	}
/* SECTION LINE NUMBERS */
	if(!(coff_hdr.flags & 0x0004))
	{
		DEBUG(printf("Section line numbers:\n");)
		fseek(infile, sect_pos, SEEK_SET);
		for(i = coff_hdr.num_sects; i != 0; i--)
		{
			if(fread(&sect, 1, 40, infile) != 40)
			{
				err_msg = "Error reading section header";
				goto ERR;
			}
			DEBUG(printf("\tSection '%-8.8s':\t", sect.name);)
/* line numbers for executable section only */
			if(!(sect.flags & 0x20))
			{
				DEBUG(printf("\n");)
				continue;
			}
/* handle possible gap between raw data and line numbers */
			pos2 = coff_pos + sect.line_nums_offset;
			DEBUG(printf("0x%lX-0x%lX\n",
				pos2, pos2 + sect.num_line_nums * 6);)
			if(pos1 < pos2)
			{
				DEBUG(printf("--- zero-filling gap "
					"after raw data of section "
					"'%-8.8s': 0x%lX-0x%lX\n",
					sect.name, pos1, pos2);)
				zero_region(outfile, pos1, pos2);
			}
			pos1 = pos2 + sect.num_line_nums * 6;
		}
	}
/* SYMBOL TABLE */
	if(coff_hdr.num_syms)
	{
/* handle possible gap between end of last section and symbol table */
		pos2 = coff_pos + coff_hdr.symtab_offset;
		DEBUG(printf("Symbol table:\t\t\t0x%lX-0x%lX\n",
			pos2, pos2 + coff_hdr.num_syms * 18);)
		if(pos1 < pos2)
		{
			DEBUG(printf("--- zero-filling gap between last "
				"section and symbol table: 0x%lX-0x%lX\n",
				pos1, pos2);)
			zero_region(outfile, pos1, pos2);
		}
		pos1 = pos2 + coff_hdr.num_syms * 18;
	}
	else
	{
/* handle possible gap between end of last section and end of file
If there's a symbol table, this region is used for the string table,
so don't zero it */
		fseek(infile, 0, SEEK_END);
		pos2 = ftell(infile);
		DEBUG(printf("File end:\t\t\t0x%lX\n", pos2);)
		if(pos1 < pos2)
		{
			DEBUG(printf("--- zero-filling gap between last "
				"section and end of file: 0x%lX-0x%lX\n",
				pos1, pos2);)
			zero_region(outfile, pos1, pos2);
		}
	}
	fclose(infile);
	fclose(outfile);
/* let the new file replace the old */
	remove(file_name);
	rename(outfile_name, file_name);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	int i;

	if(arg_c < 2)
	{
		printf("Zeroes unused areas of DJGPP COFF executable files\n");
		return 1;
	}
	for(i = 1; i < arg_c; i++)
		(void)tidy_coff(arg_v[i]);
	return 0;
}
