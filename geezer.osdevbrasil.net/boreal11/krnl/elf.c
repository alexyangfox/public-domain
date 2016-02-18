/*----------------------------------------------------------------------------
ELF FILES

EXPORTS:
int load_elf_reloc(char *image, unsigned *entry);
int load_elf_exec(char *image, unsigned *entry, aspace_t *as);
----------------------------------------------------------------------------*/
#include <string.h> /* NULL */
#include <errno.h> /* ERR_... */
#include <elf.h>
#include "_krnl.h" /* aspace_t, kprintf() */

/* IMPORTS
from SYSCALLS.C */
int lookup(char *sym_name, unsigned *adr, unsigned uscore);

/* from ASPACE.C */
int add_section(aspace_t *as, unsigned adr, unsigned size,
		unsigned flags, unsigned long offset);

/* from MM.C */
void *kcalloc(unsigned size);
/*****************************************************************************
get value of symbol #i
*****************************************************************************/
static int get_elf_sym(char *image, unsigned i,
		unsigned *sym_val, unsigned symtab_sect)
{
	elf_file_t *file;
	elf_sect_t *sect;
	elf_sym_t *sym;
	char *sym_name;
	unsigned adr;
	int err;

/* point to symbol table */
	file = (elf_file_t *)image;
	if(symtab_sect >= file->num_sects)
	{
		kprintf("get_elf_sym: bad symbol table section number %d "
			"(max %u)\n", symtab_sect, file->num_sects - 1);
		return -ERR_RELOC;
	}
	sect = (elf_sect_t *)(image + file->shtab_offset +
		file->sh_size * symtab_sect);
/* get symbol */
	if(i >= sect->size)
	{
		kprintf("get_elf_sym: offset into symbol table %u exceeds "
			"symbol table size (%lu)\n", i, sect->size);
		return -ERR_RELOC;
	}
	sym = (elf_sym_t *)(image + sect->offset) + i;
/* external symbol */
	if(sym->section == 0)
	{
/* point to string table for this symbol table */
		sect = (elf_sect_t *)(image + file->shtab_offset +
			file->sh_size * sect->link);
/* get symbol name */
		sym_name = (char *)image + sect->offset + sym->name;
/* ELF binutils for DJGPP: leading underscore
		err = lookup(sym_name, sym_val, 1); */
/* Linux: no leading underscore */
		err = lookup(sym_name, sym_val, 0);
		if(err)
			return err;
	}
/* internal symbol */
	else
	{
		sect = (elf_sect_t *)(image + file->shtab_offset +
			file->sh_size * sym->section);
		adr = (unsigned)image + sect->offset;
		*sym_val = sym->value + adr;
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int do_elf_reloc(char *image, elf_reloc_t *reloc, elf_sect_t *sect)
{
	unsigned t_adr, sym_val;
	elf_sect_t *t_sect;
	elf_file_t *file;
	uint32_t *where;
	int err;

/* get address of target section */
	file = (elf_file_t *)image;
	t_sect = (elf_sect_t *)(image + file->shtab_offset +
		file->sh_size * sect->info);
	t_adr = (unsigned)image + t_sect->offset;
/* point to relocation */
	where = (uint32_t *)(t_adr + reloc->adr);
/* get symbol */
	err = get_elf_sym(image, reloc->symtab_index, &sym_val, sect->link);
	if(err)
		return err;
	switch(reloc->type)
	{
/* absolute reference
Both ELF.H and objdump call this "R_386_32" */
	case 1: /* S + A */
		*where += sym_val;
		break;
/* EIP-relative reference
Both ELF.H and objdump call this "R_386_PC32" */
	case 2: /* S + A - P */
		*where += sym_val - (unsigned)where;
		break;
	default:
		kprintf("do_elf_reloc: unknown/unsupported relocation "
			"type %u (must be 1 or 2)\n", reloc->type);
		return -ERR_RELOC;
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int load_elf_reloc(char *image, unsigned *entry)
{
	unsigned s, r, reloc_size;
	char *bss = NULL, saw_text = 0;
	elf_reloc_t *reloc;
	elf_sect_t *sect;
	elf_file_t *file;
	int err;

/* validate */
	file = (elf_file_t *)image;
	if(file->magic != 0x464C457FL) /* "ELF" */
	{
/*		kprintf("load_elf_reloc: file magic is 0x%lX; "
			"should be 0x464C457F\n", file->magic); */
		return +1;
	}
	if(file->bitness != 1)
	{
		kprintf("load_elf_reloc: got 64-bit file, "
			"wanted 32-bit\n");
		return -ERR_RELOC;
	}
	if(file->endian != 1)
	{
		kprintf("load_elf_reloc: got big endian file, "
			"wanted little endian\n");
		return -ERR_RELOC;
	}
	if(file->elf_ver_1 != 1)
	{
		kprintf("load_elf_reloc: bad ELF version %u\n",
			file->elf_ver_1);
		return -ERR_RELOC;
	}
	if(file->file_type != 1)
	{
		kprintf("load_elf_reloc: file is not relocatable ELF\n");
		return -ERR_RELOC;
	}
	if(file->machine != 3)
	{
		kprintf("load_elf_reloc: file is not i386 ELF\n");
		return -ERR_RELOC;
	}
	if(file->elf_ver_2 != 1)
	{
		kprintf("load_elf_reloc: bad ELF version %lu\n",
			file->elf_ver_2);
		return -ERR_RELOC;
	}
/* scan sections */
	for(s = 0; s < file->num_sects; s++)
	{
		sect = (elf_sect_t *)(image + file->shtab_offset +
				file->sh_size * s);
/* find the BSS and allocate memory for it
This must be done BEFORE doing any relocations,
otherwise you end up relocating unallocated memory. */
		if(bss == NULL && sect->type == 8) /* NOBITS */
		{
			r = sect->size;
			bss = kcalloc(r);
			if(bss == NULL)
			{
				kprintf("load_elf_reloc: can't "
					"allocate %u bytes for BSS\n", r);
				return -ERR_NO_MEM;
			}
/* xxx - if 'image' is read-only, this will fault: */
			sect->offset = bss - image;
		}
/* entry point is start of first executable section; usually .text */
		else if(!saw_text && (sect->flags & 0x0004))
		{
			(*entry) = (unsigned)image + sect->offset;
			saw_text = 1;
		}
	}
	if(!saw_text)
	{
		kprintf("load_elf_reloc: can't find section .text, "
			"so entry point is unknown\n");
		return -ERR_RELOC;
	}
/* for each section... */
	for(s = 0; s < file->num_sects; s++)
	{
		sect = (elf_sect_t *)(image + file->shtab_offset +
				file->sh_size * s);
/* is it a relocation section?
xxx - we don't handle the extra addend for RELA relocations */
		if(sect->type == 4)	/* RELA */
			reloc_size = 12;
		else if(sect->type == 9)/* REL */
			reloc_size = 8;
		else
			continue;
/* for each relocation... */
		for(r = 0; r < sect->size / reloc_size; r++)
		{
			reloc = (elf_reloc_t *)(image + sect->offset +
				reloc_size * r);
			err = do_elf_reloc(image, reloc, sect);
			if(err)
				return err;
		}
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int load_elf_exec(char *image, unsigned *entry, aspace_t *as)
{
	elf_file_t *elf;
	elf_seg_t *seg;
	int i, err;

/* validate ELF headers */
	elf = (elf_file_t *)image;
	if(elf->magic != 0x464C457FL) /* "ELF" */
		return 1;
	if(elf->bitness != 1 ||		/* 1=32-bit, 2=64-bit */
		elf->endian != 1 ||	/* 1=little-endian, 2=big */
		elf->elf_ver_1 != 1 ||	/* 1=current ELF spec */
		elf->file_type != 2 ||	/* 1=reloc, 2=exec, 3=DLL */
		elf->machine != 3 ||	/* 3=i386 */
		elf->elf_ver_2 != 1)
			return 1;
/* get entry point */
	(*entry) = elf->entry;
/* seek to program headers (segments) */
	image += elf->phtab_offset;
	for(i = 0; i < elf->num_ph; i++)
	{
		seg = (elf_seg_t *)(image + elf->ph_size * i);
/* choke on 2=DYNAMIC and the forbidden 5=SHLIB segments */
		if(seg->type == 2 || seg->type == 5)
			return -ERR_EXEC;
/* handle 1=LOAD segment */
		else if(seg->type == 1)
		{
			err = add_section(as, seg->virt_adr, seg->disk_size,
				SF_LOAD | (seg->flags & 7), seg->offset);
			if(err)
				return err;
/* if size-in-mem > size-on-disk, this segment contains the BSS */
			if(seg->mem_size > seg->disk_size)
			{
				err = add_section(as,
					seg->virt_adr + seg->disk_size,
					seg->mem_size - seg->disk_size,
					SF_ZERO | (seg->flags & 7),
					seg->offset);
				if(err)
					return err;
			}
		}
/* ignore 0=NULL, 6=PHDR, 3=INTERP, and 4=NOTE segments
		else
			nothing; */
	}
	return 0;
}
