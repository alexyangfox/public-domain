/*****************************************************************************
Code snippet to load DJGPP COFF executables.

This code is public domain (no copyright).
You can do whatever you want with it.

EXPORTS:
int load_dj_exec(task_t *task, unsigned char *image, unsigned *entry);
*****************************************************************************/
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
	uint16_t magic		__PACKED__;
	uint16_t num_sects	__PACKED__;
	uint32_t time_date	__PACKED__;
	uint32_t symtab_offset	__PACKED__;
	uint32_t num_syms	__PACKED__;
	uint16_t aout_size	__PACKED__;
	uint16_t file_flags	__PACKED__;
} coff_file_t; /* 20 bytes; same as Win32 PE COFF */

typedef struct
{
	uint16_t magic		__PACKED__;
	uint16_t version	__PACKED__;
	uint32_t code_size	__PACKED__;
	uint32_t data_size	__PACKED__;
	uint32_t bss_size	__PACKED__;
	uint32_t entry		__PACKED__;
	uint32_t code_offset	__PACKED__;
	uint32_t data_offset	__PACKED__;
} dj_aout_t; /* 28 bytes */

typedef struct
{
	char name[8]		__PACKED__;
	uint32_t phys_adr	__PACKED__;
	uint32_t virt_adr	__PACKED__;
	uint32_t size		__PACKED__;
	uint32_t offset		__PACKED__;
	uint32_t relocs_offset	__PACKED__;
	uint32_t line_nums_offset __PACKED__;
	uint16_t num_relocs	__PACKED__;
	uint16_t num_line_nums	__PACKED__;
	uint32_t flags		__PACKED__;
} dj_sect_t;
/*****************************************************************************
Creates task from memory-mapped DJGPP COFF executable file
at address 'image'. Returns:
   +1	if file is not valid DJGPP COFF
   0	if success (*entry set to entry point)
   <0	if error from add_section()

If your OS has file I/O code, you could replace 'unsigned char *image'
with a file handle, make 'coff', 'aout', and 'sect' actual structs
(instead of pointers), seek and read from the file, etc.
*****************************************************************************/
int load_dj_exec(task_t *task, unsigned char *image, unsigned *entry)
{
	coff_file_t *coff;
	dj_aout_t *aout;
	dj_sect_t *sect;
	int i, s, flags;

/* validate headers */
	coff = (coff_file_t *)image;
	image += sizeof(coff_file_t);
	aout = (dj_aout_t *)image;
	image += sizeof(dj_aout_t);
	if(coff->magic != 0x014C || coff->aout_size != 28 ||
		aout->magic != 0x010B)
			return 1;
/* get entry point */
	(*entry) = aout->entry;
/* read section headers */
	for(s = 0; s < coff->num_sects; s++)
	{
		sect = (dj_sect_t *)(image + sizeof(dj_sect_t) * s);
/* code */
		if((sect->flags & 0xE0) == 0x20)
			flags = SF_LOAD | SF_READ | SF_EXEC;
/* data */
		else if((sect->flags & 0xE0) == 0x40)
			flags = SF_LOAD | SF_READ | SF_WRITE;
/* BSS */
		else if((sect->flags & 0xE0) == 0x80)
			flags = SF_ZERO | SF_READ | SF_WRITE;
/* ignore anything else */
		else
			continue;
		i = add_section(task, flags, sect->virt_adr,
			sect->size, sect->offset);
		if(i != 0)
			return i;
	}
	return 0;
}
