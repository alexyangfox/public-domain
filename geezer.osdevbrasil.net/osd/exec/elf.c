/*****************************************************************************
Code snippet to load ELF executables. The executable must be
statically-linked.

This code is public domain (no copyright).
You can do whatever you want with it.

EXPORTS:
int load_elf_exec(task_t *task, unsigned char *image, unsigned *entry);
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
	unsigned char magic[4]	__PACKED__;
	unsigned char bitness	__PACKED__;
	unsigned char endian	__PACKED__;
	unsigned char ver_1	__PACKED__;
	unsigned char res[9]	__PACKED__;
/* multi-byte fields from here on are big endian for big endian CPUs
or little endian for little-endian CPUs. Thus, we can use a packed
struct instead of horrid char array, byte-swapping macros, etc. */
	uint16_t file_type	__PACKED__;
	uint16_t machine	__PACKED__;
	uint32_t ver_2		__PACKED__;
	uint32_t entry		__PACKED__;
	uint32_t phtab_offset	__PACKED__;
	uint32_t shtab_offset	__PACKED__;
	uint32_t flags		__PACKED__;
	uint16_t file_hdr_size	__PACKED__;
	uint16_t phtab_ent_size	__PACKED__;
	uint16_t num_phs	__PACKED__;
	uint16_t shtab_ent_size	__PACKED__;
	uint16_t num_shs	__PACKED__;
	uint16_t shstrtab_index	__PACKED__;
} elf_file_t; /* 52 bytes */

typedef struct
{
	uint32_t type		__PACKED__;
	uint32_t offset		__PACKED__;
	uint32_t virt_adr	__PACKED__;
	uint32_t phys_adr	__PACKED__;
	uint32_t disk_size	__PACKED__;
	uint32_t mem_size	__PACKED__;
	uint32_t flags		__PACKED__;
	uint32_t align		__PACKED__;
} elf_seg_t; /* 32 bytes */
/*****************************************************************************
Creates task from memory-mapped ELF executable file
at address 'image'. Returns:
   +1	if file is not valid ELF
   0	if success (*entry set to entry point)
   <0	if error (dynamically-linked file, or error from add_section())

If your OS has file I/O code, you could replace 'unsigned char *image'
with a file handle, make 'elf' and 'seg' actual structs
(instead of pointers), seek and read from the file, etc.
*****************************************************************************/
int load_elf_exec(task_t *task, unsigned char *image, unsigned *entry)
{
	elf_file_t *elf;
	elf_seg_t *seg;
	int i, j;

/* validate ELF headers */
	elf = (elf_file_t *)image;
	if(elf->magic[0] != 0x7F || elf->magic[1] != 'E' ||
		elf->magic[2] != 'L' || elf->magic[3] != 'F')
			return 1;
	if(elf->bitness != 1 ||		/* 1=32-bit, 2=64-bit */
		elf->endian != 1 ||	/* 1=little-endian, 2=big */
		elf->ver_1 != 1 ||	/* 1=current ELF spec */
		elf->file_type != 2 ||	/* 1=reloc, 2=exec, 3=DLL */
		elf->machine != 3 ||	/* 3=i386 */
		elf->ver_2 != 1)
			return 1;
/* get entry point */
	(*entry) = elf->entry;
/* seek to program headers (segments) */
	image += elf->phtab_offset;
	for(i = 0; i < elf->num_phs; i++)
	{
		seg = (elf_seg_t *)(image + elf->phtab_ent_size * i);
/* choke on 2=DYNAMIC and the forbidden 5=SHLIB segments */
		if(seg->type == 2 || seg->type == 5)
			return -1;
/* handle 1=LOAD segment */
		else if(seg->type == 1)
		{
			j = add_section(task, SF_LOAD | (seg->flags & 7),
				seg->virt_adr, seg->disk_size, seg->offset);
			if(j != 0)
				return j;
/* if size-in-mem > size-on-disk, this segment contains the BSS */
			if(seg->mem_size > seg->disk_size)
			{
				j = add_section(task, SF_ZERO | (seg->flags & 7),
					seg->virt_adr + seg->disk_size,
					seg->mem_size - seg->disk_size,
					seg->offset);
				if(j != 0)
					return j;
			}
		}
/* ignore 0=NULL, 6=PHDR, 3=INTERP, and 4=NOTE segments
		else
			nothing; */
	}
	return 0;
}
