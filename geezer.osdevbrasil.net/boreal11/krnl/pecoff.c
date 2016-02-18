/*----------------------------------------------------------------------------
WIN32 PE COFF FILES

EXPORTS
int load_pe_reloc(char *image, unsigned *entry);
int load_pe_exec(char *image, unsigned *entry, aspace_t *as);
----------------------------------------------------------------------------*/
#include <string.h> /* NULL, strcmp(), memcmp() */
#include <errno.h> /* ERR_... */
#include <coff.h> /* coff_sect_t, coff_reloc_t, coff_file_t */
#include "_krnl.h" /* aspace_t, kprintf() */

/* IMPORTS
from COFF.C */
int get_coff_sym(char *image, unsigned i,
		unsigned *sym_val, unsigned *sect_num);

/* from ASPACE.C */
int add_section(aspace_t *as, unsigned adr, unsigned size,
		unsigned flags, unsigned long offset);

/* from MM.C */
void *kcalloc(unsigned size);

/* two fields in the COFF section header are re-defined by Win32 PE COFF: */
#define	virt_size	phys_adr
#define	raw_size	size

#pragma pack(1)
typedef struct
{
	char magic[2];
/* not really unused, but we don't care about them for PE */
	char unused[6];
	uint16_t hdr_size;
	char unused2[50];
	uint32_t new_exe_offset;
} exe_file_t;

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
	uint32_t image_base;
	char unused[192];
} pe_aout_t;
/*****************************************************************************
*****************************************************************************/
static int do_pe_reloc(char *image, coff_sect_t *sect, unsigned r)
{
	unsigned sym_sect, sym_val, t_adr = 0;
	coff_reloc_t *reloc;
	coff_file_t *file;
	uint32_t *where;
	int r_delta, err;

/* r_delta = delta (i.e. LMA - VMA) for section containing this relocation */
	r_delta = (int)image + sect->offset - sect->virt_adr;
/* point to the item to be relocated */
	reloc = (coff_reloc_t *)(image + sect->relocs_offset) + r;
	where = (uint32_t *)(reloc->adr + r_delta);
/* get symbol */
	err = get_coff_sym(image, reloc->symtab_index, &sym_val, &sym_sect);
	if(err)
		return err;
/* t_adr = address of target section (section in symbol table entry) */
	file = (coff_file_t *)image;
	if(sym_sect != 0) /* internal symbol */
	{
		sect = (coff_sect_t *)(image + sizeof(coff_file_t) +
			file->aout_hdr_size) + (sym_sect - 1);
		t_adr = (unsigned)image + sect->offset;
	}
	switch(reloc->type)
	{
/* absolute reference
COFF.H calls this "RELOC_ADDR32"; objdump calls it "dir32" */
	case 6:
		if(sym_sect == 0) /* external symbol */
			*where = sym_val;
		else
			*where += (sym_val + t_adr);
		break;
/* EIP-relative reference
COFF.H calls this "RELOC_REL32"; objdump calls it "DISP32" */
	case 20:
		if(sym_sect == 0) /* external symbol */
/* 4 = width of relocated item, in bytes. Why should this code
have any knowledge of that? Maybe I'm doing something wrong.
Code seems to work, though. */
			*where = sym_val - ((unsigned)where + 4);
		else
			*where += (sym_val + t_adr - ((unsigned)where + 4));
		break;
	default:
		kprintf("do_pe_reloc: unknown i386 relocation type %u "
			"(must be 6 or 20)\n", reloc->type);
		return -ERR_RELOC;
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int load_pe_reloc(char *image, unsigned *entry)
{
	char *bss, saw_text = 0;
	coff_sect_t *sect;
	coff_file_t *file;
	unsigned s, r;
	int err;

/* validate */
	file = (coff_file_t *)image;
	if(file->magic != 0x14C)
	{
/*		kprintf("load_pe_reloc: file magic is 0x%X; "
			"should be 0x14C\n", file->magic); */
		return +1;
	}
	if(file->flags & 0x0001)
	{
/*		kprintf("load_pe_reloc: relocations have been "
			"stripped from file\n"); */
		return +1;
	}
	if(file->aout_hdr_size != 0)
	{
		kprintf("load_pe_reloc: executable file "
			"(aout_hdr_size=%u, should be 0)\n",
			file->aout_hdr_size);
		return +1;
	}
/* scan sections */
	for(s = 0; s < file->num_sects; s++)
	{
		sect = (coff_sect_t *)(image + sizeof(coff_file_t) +
			file->aout_hdr_size) + s;
/* find the BSS and allocate memory for it
This must be done BEFORE doing any relocations,
otherwise you end up relocating unallocated memory. */
		if((sect->flags & 0xE0) == 0x80)
		{
			r = sect->virt_size;
			bss = kcalloc(r);
			if(bss == NULL)
			{
				kprintf("load_pe_reloc: can't "
					"allocate %u bytes for BSS\n", r);
				return -ERR_NO_MEM;
			}
/* xxx - if 'image' is read-only, this will fault: */
			sect->offset = bss - image;
		}
/* entry point is start of first executable section; usually .text */
		else if(!saw_text && (sect->flags & 0xE0) == 0x20)
		{
			(*entry) = (unsigned)image + sect->offset;
			saw_text = 1;
		}
	}
	if(!saw_text)
	{
		kprintf("load_pe_reloc: can't find section .text, "
			"so entry point is unknown\n");
		return -ERR_RELOC;
	}
/* for each section... */
	for(s = 0; s < file->num_sects; s++)
	{
		sect = (coff_sect_t *)(image + sizeof(coff_file_t) +
			file->aout_hdr_size) + s;
/* for each relocation... */
		for(r = 0; r < sect->num_relocs; r++)
		{
			err = do_pe_reloc(image, sect, r);
			if(err)
				return err;
		}
		sect++;
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int load_pe_exec(char *image, unsigned *entry, aspace_t *as)
{
	uint32_t bss_adr = 0, bss_size = 0;
	int s, flags, err, saw_bss = 0;
	exe_file_t *exe;
	coff_file_t *coff;
	pe_aout_t *aout;
	coff_sect_t *sect;

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
	if(image[0] != 'P' ||
		image[1] != 'E' ||
		image[2] != 0 ||
		image[3] != 0)		/* new-style .EXE but not PE .EXE */
			return 1;
	image += 4;
/* COFF and a.out */
	coff = (coff_file_t *)image;
	image += sizeof(coff_file_t);
	aout = (pe_aout_t *)image;
	image += sizeof(pe_aout_t);
	if(coff->magic != 0x014C ||		/* not COFF */
		coff->aout_hdr_size != sizeof(pe_aout_t) ||
		aout->magic != 0x010B)		/* not executable PE COFF */
			return 1;
/* get entry point */
	(*entry) = aout->entry + aout->image_base;
/* seek to section headers */
	for(s = 0; s < coff->num_sects; s++)
	{
		sect = (coff_sect_t *)(image + sizeof(coff_sect_t) * s);
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
/* xxx - if 'image' is read-only, this will fault: */
			sect->raw_size = sect->virt_size;
			saw_bss = 1;
		}
/* ignore other sections */
		else
			continue;
		err = add_section(as, aout->image_base + sect->virt_adr,
			sect->raw_size, flags, sect->offset);
		if(err)
			return err;
	}
	if(!saw_bss)
	{
		err = add_section(as, bss_adr, bss_size, SF_BSS, 0);
		if(err)
			return err;
	}
	return 0;
}
