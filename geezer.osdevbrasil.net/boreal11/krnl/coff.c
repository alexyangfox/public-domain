/*----------------------------------------------------------------------------
(DJGPP) COFF FILES

EXPORTS
int get_coff_sym(char *image, unsigned i,
		unsigned *sym_val, unsigned *sect_num);
int load_coff_reloc(char *image, unsigned *entry);
int load_coff_exec(aspace_t *as, unsigned *entry, char *image);
----------------------------------------------------------------------------*/
#include <string.h> /* NULL, strcmp() */
#include <errno.h> /* ERR_... */
#include <coff.h> /* coff_sect_t, coff_reloc_t, coff_file_t */
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
*****************************************************************************/
int get_coff_sym(char *image, unsigned i,
		unsigned *sym_val, unsigned *sect_num)
{
	char *sym_name, *strtab;
	coff_file_t *file;
	coff_sym_t *sym;
/*	unsigned len; */
	int err;

	file = (coff_file_t *)image;
/* number of symbol table entries */
	if(i >= file->num_syms)
	{
		kprintf("get_coff_sym: index into symbol table %u "
			"is too large (max %lu)\n", i, file->num_syms);
		return -ERR_RELOC;
	}
/* point to symtab entry, get name */
	sym = (coff_sym_t *)(image + file->symtab_offset) + i;
	sym_name = sym->x.name;
/*	len = 8; */
	if(sym->x.x.zero == 0)
	{
		strtab = (char *)image + file->symtab_offset +
			file->num_syms * sizeof(coff_sym_t);
		sym_name = strtab + sym->x.x.strtab_index;
/*		len = strlen(sym_name); */
	}
/* get section and check it */
	if(sym->sect_num > file->num_sects)
	{
		kprintf("get_coff_sym: symbol '%-8s' has bad section %d "
			"(max %u)\n", sym_name, sym->sect_num,
			file->num_sects);
		return -ERR_RELOC;
	}
	*sect_num = sym->sect_num;
/* external symbol */
	if(*sect_num == 0)
	{
		err = lookup(sym_name, sym_val, 1);
		if(err)
			return err;
	}
/* internal symbol */
	else
		*sym_val = sym->val;
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int do_coff_reloc(char *image, coff_sect_t *sect, unsigned r)
{
	int err, r_delta, t_delta = 0;
	unsigned sym_val, sym_sect;
	coff_reloc_t *reloc;
	coff_file_t *file;
	uint32_t *where;

/* r_delta = delta (i.e. LMA - VMA) for section containing this relocation */
	r_delta = (int)image + sect->offset - sect->virt_adr;
/* point to the item to be relocated */
	reloc = (coff_reloc_t *)(image + sect->relocs_offset) + r;
	where = (uint32_t *)(reloc->adr + r_delta);
/* get symbol */
	err = get_coff_sym(image, reloc->symtab_index, &sym_val, &sym_sect);
	if(err)
		return err;
/* t_delta = delta for target section (section in symbol table entry) */
	file = (coff_file_t *)image;
	if(sym_sect != 0) /* internal symbol */
	{
		sect = (coff_sect_t *)(image + sizeof(coff_file_t) +
			file->aout_hdr_size) + (sym_sect - 1);
		t_delta = (int)image + sect->offset - sect->virt_adr;
	}
	switch(reloc->type)
	{
/* absolute reference
COFF.H calls this "RELOC_ADDR32"; objdump calls it "dir32" */
	case 6:
		if(sym_sect == 0) /* external symbol */
			*where = sym_val;
		else
			*where += t_delta;
		break;
/* EIP-relative reference
COFF.H calls this "RELOC_REL32"; objdump calls it "DISP32" */
	case 20:
		if(sym_sect == 0) /* external symbol */
			*where += sym_val - r_delta;
/* type 20 relocation for internal symbol
does not occur with normal DJGPP code */
		else
			*where += t_delta - r_delta;
		break;
	default:
		kprintf("do_coff_reloc: unknown i386 relocation type %u "
			"(must be 6 or 20)\n", reloc->type);
		return -ERR_RELOC;
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int load_coff_reloc(char *image, unsigned *entry)
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
/*		kprintf("load_coff_reloc: file magic is 0x%X; "
			"should be 0x14C\n", file->magic); */
		return +1;
	}
	if(file->flags & 0x0001)
	{
		kprintf("load_coff_reloc: relocations have been "
			"stripped from file\n");
		return +1;
	}
	if(file->aout_hdr_size != 0)
	{
		kprintf("load_coff_reloc: executable file "
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
			r = sect->size;
			bss = kcalloc(r);
			if(bss == NULL)
			{
				kprintf("load_coff_reloc: can't "
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
		kprintf("load_coff_reloc: can't find section .text, "
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
			err = do_coff_reloc(image, sect, r);
			if(err)
				return err;
		}
		sect++;
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int load_coff_exec(char *image, unsigned *entry, aspace_t *as)
{
	coff_file_t *coff;
	coff_aout_t *aout;
	coff_sect_t *sect;
	int s, flags, err;

/* validate headers */
	coff = (coff_file_t *)image;
	image += sizeof(coff_file_t);
	aout = (coff_aout_t *)image;
	image += sizeof(coff_aout_t);
	if(coff->magic != 0x014C ||
		coff->aout_hdr_size != 28 || aout->magic != 0x010B)
			return 1;
/* get entry point */
	(*entry) = aout->entry;
/* read section headers */
	for(s = 0; s < coff->num_sects; s++)
	{
		sect = (coff_sect_t *)(image + sizeof(coff_sect_t) * s);
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
		err = add_section(as, sect->virt_adr,
			sect->size, flags, sect->offset);
		if(err)
			return err;
	}
	return 0;
}
