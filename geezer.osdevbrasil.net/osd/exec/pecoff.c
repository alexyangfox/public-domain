/*****************************************************************************
Code snippet to load Win32 PE COFF executables. The executable must be
statically-linked. Base relocations in the executable will be ignored.
Tested with MinGW 3.2 and Borland C 5.5. Broken executables made by
MinGW 2.95 are not supported.

This code is public domain (no copyright).
You can do whatever you want with it.

EXPORTS:
int load_pe_exec(task_t *task, unsigned char *image, unsigned *entry);
*****************************************************************************/
#include <string.h> /* memcmp() */

/* flags used to describe task memory sections */
#define	SF_ZERO		0x10	/* BSS */
#define	SF_LOAD		0x08	/* load from file */
#define	SF_READ		0x04	/* readable */
#define	SF_WRITE	0x02	/* writable */
#define	SF_EXEC		0x01	/* executable */
#define	SF_BSS		(SF_ZERO | SF_READ | SF_WRITE)
/* user-defined task structure, used here as tramp data for add_section() */
typedef struct { char whatever; } task_t;
/* user-defined function to add memory section to task
should return 0 if success, <0 if error */
int add_section(task_t *task, unsigned flags, unsigned long adr,
		unsigned long size, unsigned long offset);

#if defined(__GNUC__)
#define	__PACKED__	__attribute__((packed))
#else
#define	__PACKED__	/* nothing */
#endif

/* or use STDINT.H, if you have it... */
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;

typedef struct
{
	char magic[2]		__PACKED__;
/* not really unused, but we don't care about them for PE */
	char unused[6]		__PACKED__;
	uint16_t hdr_size	__PACKED__;
	char unused2[50]	__PACKED__;
	uint32_t new_exe_offset	__PACKED__;
} exe_file_t; /* 64 bytes */

typedef struct
{
	uint16_t coff_magic	__PACKED__;
	uint16_t num_sects	__PACKED__;
	uint32_t time_date	__PACKED__;
	uint32_t symtab_offset	__PACKED__;
	uint32_t num_syms	__PACKED__;
	uint16_t aout_size	__PACKED__;
	uint16_t file_flags	__PACKED__;
} coff_file_t; /* 20 bytes; same as DJGPP COFF */

typedef struct
{
	uint16_t aout_magic	__PACKED__;
	uint16_t version	__PACKED__;
	uint32_t code_size	__PACKED__;
	uint32_t data_size	__PACKED__;
	uint32_t bss_size	__PACKED__;
	uint32_t entry		__PACKED__;
	uint32_t code_offset	__PACKED__;
	uint32_t data_offset	__PACKED__;
	uint32_t image_base	__PACKED__;
	char unused[192]	__PACKED__;
} pe_aout_t; /* 224 bytes */

typedef struct
{
	char name[8]		__PACKED__;
	uint32_t virt_size	__PACKED__; /* size-in-mem */
	uint32_t virt_adr	__PACKED__; /* RVA */
	uint32_t raw_size	__PACKED__; /* size-on-disk */
	uint32_t offset		__PACKED__;
	uint32_t relocs_offset	__PACKED__;
	uint32_t line_nums_offset __PACKED__;
	uint16_t num_relocs	__PACKED__;
	uint16_t num_line_nums	__PACKED__;
	uint32_t flags		__PACKED__;
} pe_sect_t; /* 40 bytes */
/*****************************************************************************
Creates task from memory-mapped Win32 PE COFF executable file
at address 'image'. Returns:
   +1	if file is not valid Win32 PE COFF
   0	if success (*entry set to entry point)
   <0	if error (dynamically-linked file, or error from add_section())

If your OS has file I/O code, you could replace 'unsigned char *image'
with a file handle, make 'exe', 'coff', 'aout', etc. actual structs
(instead of pointers), seek and read from the file, etc.
*****************************************************************************/
int load_pe_exec(task_t *task, unsigned char *image, unsigned *entry)
{
	uint32_t bss_adr = 0, bss_size = 0;
	int i, saw_bss = 0, flags;
	exe_file_t *exe;
	coff_file_t *coff;
	pe_aout_t *aout;
	pe_sect_t *sect;

/* validate headers
DOS .EXE */
	exe = (exe_file_t *)image;
	image += exe->new_exe_offset;
	if(exe->magic[0] != 'M' ||
		exe->magic[1] != 'Z' ||		/* not .EXE */
		exe->hdr_size < 4 ||
		exe->new_exe_offset == 0)	/* not new-style .EXE */
			return 1;
/* PE */
	if(image[0] != 'P' || image[1] != 'E' || image[2] != 0 ||
		image[3] != 0)		/* new-style .EXE but not PE .EXE */
			return 1;
	image += 4;
/* COFF and a.out */
	coff = (coff_file_t *)image;
	image += sizeof(coff_file_t);
	aout = (pe_aout_t *)image;
	image += sizeof(pe_aout_t);
	if(coff->coff_magic != 0x014C ||
		coff->aout_size != sizeof(pe_aout_t) ||
		aout->aout_magic != 0x010B)
			return 1;
/* get entry point */
	(*entry) = aout->entry + aout->image_base;
/* seek to section headers */
	for(i = 0; i < coff->num_sects; i++)
	{
		sect = (pe_sect_t *)(image + sizeof(pe_sect_t) * i);
/* code */
		if(!memcmp(sect->name, ".text", 5) &&
			(sect->flags & 0xE0) == 0x20)
				flags = SF_LOAD | SF_READ | SF_EXEC;
/* data */
		else if(!memcmp(sect->name, ".data", 5) &&
			(sect->flags & 0xE0) == 0x40)
		{
			flags = SF_LOAD | SF_READ | SF_WRITE;
/* is BSS part of .data ? */
			bss_adr = aout->image_base +
				sect->virt_adr + sect->raw_size;
			bss_size = sect->virt_size - sect->raw_size;
		}
/* BSS */
		else if(!memcmp(sect->name, ".bss", 4) &&
			(sect->flags & 0xE0) == 0x80)
		{
			flags = SF_ZERO | SF_READ | SF_WRITE;
/* xxx - is 'image' read-only? this might cause a fault... */
			sect->raw_size = sect->virt_size;
			saw_bss = 1;
		}
/* import table means file is dynamically-linked */
		else if(!memcmp(sect->name, ".idata", 6))
			return -1;
/* ignore other sections */
		else
			continue;
		i = add_section(task, flags, aout->image_base +
			sect->virt_adr, sect->raw_size, 0);
		if(i != 0)
			return i;
	}
	if(!saw_bss)
	{
		i = add_section(task, SF_BSS, bss_adr, bss_size, 0);
		if(i != 0)
			return i;
	}
	return 0;
}
