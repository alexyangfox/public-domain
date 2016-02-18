/*----------------------------------------------------------------------------
ADDRESS SPACES

EXPORTS:
aspace_t *create_aspace(void);
void destroy_aspace(aspace_t *as);
int add_section(aspace_t *as, unsigned adr, unsigned size,
		unsigned flags, unsigned long offset);
----------------------------------------------------------------------------*/
#include <string.h> /* NULL */
#include <errno.h> /* ERR_... */
#include "_krnl.h"

#define	AS_MAGIC	0xF512

/* IMPORTS
from MM.C */
void *kcalloc(unsigned size);
void kfree(void *blk);
void *krealloc(void *blk, unsigned size);
/*****************************************************************************
*****************************************************************************/
aspace_t *create_aspace(void)
{
	aspace_t *as;

/* allocate aspace_t and zero it */
	as = (aspace_t *)kcalloc(sizeof(aspace_t));
	if(as == NULL)
		return NULL;
/* set other fields */
	as->magic = AS_MAGIC;
	return as;
}
/*****************************************************************************
*****************************************************************************/
void destroy_aspace(aspace_t *as)
{
	if(as->magic != AS_MAGIC)
		panic("destroy_aspace: bad address space 0x%p\n", as);
	if(as->user_mem != NULL)
		kfree(as->user_mem);
	if(as->sect != NULL)
	{
		memset(as->sect, 0, as->num_sects * sizeof(sect_t));
		kfree(as->sect);
	}
	memset(as, 0, sizeof(aspace_t));
	kfree(as);
}
/*****************************************************************************
*****************************************************************************/
int add_section(aspace_t *as, unsigned adr, unsigned size,
		unsigned flags, unsigned long offset)
{
	sect_t *new_sect;

/* add new memory section */
	new_sect = (sect_t *)krealloc(as->sect,
		sizeof(sect_t) * (as->num_sects + 1));
	if(new_sect == NULL)
		return -ERR_EXEC;//xxx - 1;
	as->sect = new_sect;
/* init section */
	new_sect = as->sect + as->num_sects;
	as->num_sects++;
	new_sect->adr = adr;
	new_sect->size = size;
	new_sect->flags = flags;
	new_sect->offset = offset;
	return 0;
}
