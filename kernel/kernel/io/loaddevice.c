#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/fs.h>
#include <kernel/kmalloc.h>
#include <kernel/i386/elf.h>
#include <kernel/i386/i386.h>
#include <kernel/utility.h>
#include <kernel/error.h>
#include <kernel/symbol.h>
#include <kernel/resident.h>



Elf32_EHdr *Elf32ModuleLoad (int fd);
void Elf32ModuleUnload (Elf32_EHdr *ehdr);
int ReadELF (int fd, int32 offset, int32 bytes, void *dst);
int Elf32SimplifySymbols (Elf32_EHdr *ehdr);
int Elf32Relocate (Elf32_EHdr *ehdr);
int Elf32Protect (Elf32_EHdr *elf);
int ElfBoundsCheck (uint32 base, uint32 ceiling, uint32 addr, uint32 sz);

Elf32_Sym *Elf32FindSymbol (char *name, Elf32_EHdr *ehdr);

int CheckElfModuleHeaders (Elf32_EHdr *ehdr);




char module_filename[256];

/*
 * LoadDevice();
 *
 * device list is maintained by IOManager, no need for device_list_mutex at all,
 * unless other threads wish to browse/acquire it without going through IOManager.
 * Maybe in iomanager-context of OpenDevice it is acquired?
 */

void *LoadDevice (char *name)
{
	int fd;
	Elf32_EHdr *ehdr;

	
	KPRINTF ("LoadDevice(%s)", name);
	
	
	/* Remove this pathname code,  upto caller to ensure correct pathname */
	
	if (*name == '/')
	{
		StrLCpy (module_filename, name, 256);
	}
	else
	{
		StrLCpy (module_filename, "/sys/devs/", 256);
		StrLCat (module_filename, name, 256);
	}
	
		
	
	
	if ((fd = Open (module_filename, O_RDONLY, 0)) != -1)
	{
		if ((ehdr = Elf32ModuleLoad (fd)) != NULL)
		{
			if ((Elf32SimplifySymbols (ehdr)) == 0)
			{
				if ((Elf32Relocate (ehdr)) == 0)
				{
					if ((Elf32Protect (ehdr)) == 0)
					{
						Close (fd);
						return ehdr;
					}
				}
			}
		
			Elf32ModuleUnload (ehdr);
		}
		
		Close (fd);
	}
	
	
	return NULL;
}




/*
 *
 */

void UnloadDevice (void *elf)
{
	Elf32ModuleUnload (elf);
}




/*
 * Elf32Load();
 *
 * Loads Elf Header, section table and sections into memory.
 */

Elf32_EHdr *Elf32ModuleLoad (int fd)
{
	Elf32_EHdr *ehdr;
	void *shdr_table;
	Elf32_SHdr *shdr;
	int32 t;
		
	
	ehdr = (Elf32_EHdr *) KMalloc (sizeof (Elf32_EHdr));
	
	ReadELF (fd, 0, sizeof (Elf32_EHdr), ehdr);
	
	/* Check ELF is valid */
	
	
	if (CheckElfModuleHeaders (ehdr) == -1)
		return NULL;
	
	
	shdr_table = KMalloc (ehdr->e_shnum * ehdr->e_shentsize);
	
	ReadELF (fd, ehdr->e_shoff, ehdr->e_shnum * ehdr->e_shentsize, (void *)shdr_table);
	
	ehdr->e_shoff = (uint32)shdr_table;
	

	

	/* Hope we don't depend on the original sh_addr when relocating symbols */
	
	for (t=1; t < ehdr->e_shnum; t++)
	{
		shdr = (Elf32_SHdr *)(shdr_table + (t * ehdr->e_shentsize));
				
		if (shdr->sh_type == SHT_PROGBITS)
		{
			if ((shdr->sh_flags & SHF_ALLOC) == SHF_ALLOC)
			{
				if (shdr->sh_size == 0)
					continue;
			
				if ((shdr->sh_addr = KMap (shdr->sh_size, VM_PROT_ALL)) == MAP_FAILED)
					return NULL;
					
				ReadELF (fd, shdr->sh_offset, shdr->sh_size, (void *)shdr->sh_addr);
			}
			else
			{
				shdr->sh_addr = 0;
			}
			
		}
		else if (shdr->sh_type == SHT_NOBITS)
		{
			if (shdr->sh_flags & SHF_ALLOC)
			{
				shdr->sh_addr = 0;
				if (shdr->sh_size == 0)
					continue;
			
				if ((shdr->sh_addr = KMap (shdr->sh_size, VM_PROT_ALL)) == MAP_FAILED)
					return NULL;
			}
			else
			{
				shdr->sh_addr = 0;
			}
		}
		else
		{
			if (shdr->sh_size == 0)
					continue;
			
			/* Are these symbol tables/string tables? */
			
			if ((shdr->sh_addr = KMap (shdr->sh_size, VM_PROT_ALL)) == MAP_FAILED)
				return NULL;
			
			ReadELF (fd, shdr->sh_offset, shdr->sh_size, (void *)shdr->sh_addr);
		}
	}

	return ehdr;
}




/*
 *
 */

void Elf32ModuleUnload (Elf32_EHdr *ehdr)
{
	int t;
	void *shdr_table;
	Elf32_SHdr *shdr;
		
	shdr_table = (void *)ehdr->e_shoff;
	
	for (t=1; t < ehdr->e_shnum; t++)
	{
		shdr = (Elf32_SHdr *)(shdr_table + (t * ehdr->e_shentsize));
				
		if (shdr->sh_addr != 0)
			KUnmap (shdr->sh_addr);
	}
	
	KFree (shdr_table);
	KFree (ehdr);
}




/*
 *
 */

int CheckElfModuleHeaders (Elf32_EHdr *ehdr)
{
	if (ehdr->e_ident[EI_MAG0] == ELFMAG0 && 
		ehdr->e_ident[EI_MAG1] == 'E' && 
		ehdr->e_ident[EI_MAG2] == 'L' && 
		ehdr->e_ident[EI_MAG3] == 'F' && 
		ehdr->e_ident[EI_CLASS] == ELFCLASS32 &&
		ehdr->e_ident[EI_DATA] == ELFDATA2LSB &&
		ehdr->e_type == ET_REL &&
		ehdr->e_shnum > 0)
	{
		return 0;
	}

		
	SetError (ENOEXEC);
	return -1;
}






/*
 * ReadELF();
 *
 */

int ReadELF (int fd, int32 offset, int32 nbytes, void *dst)
{
	if (Seek (fd, offset, SEEK_SET) != -1)
	{
		if (Read (fd, dst, nbytes) == nbytes)
		{
			return 0;
		}
	}
	
	return -1;
}


	
	
/*
 * ElfSimplifySymbols();
 *
 * Search for symbol tables and simplify symbols
 * Change symbols so that sh_value contains the real pointer.
 */

int Elf32SimplifySymbols (Elf32_EHdr *ehdr)
{
	uint32 shdr_table;
	Elf32_SHdr *shdr, *sym_shdr;
	int32 t, s;
	int32 max_sym;
	uint32 sym_table = 0;
	Elf32_Sym *sym;
	uint32 string_table = 0;
	uint32 sym_name;
	struct KSym *ksym;
	
		
	
	shdr_table = ehdr->e_shoff;
	
	
	
	/* Find location of String Table */
		
	for (t=0; t < ehdr->e_shnum; t++)
	{
		shdr = (Elf32_SHdr *)(shdr_table + (t * ehdr->e_shentsize));
		
		if (shdr->sh_type == SHT_STRTAB)
		{
			if (ehdr->e_shstrndx)
			{
				string_table = shdr->sh_addr;
			}
		}
	}
	
	if (string_table == 0)
		return -1;
	
		
	
	
		
	for (t = 1; t < ehdr->e_shnum; t++)
	{
		shdr = (Elf32_SHdr *)(shdr_table + (t * ehdr->e_shentsize));
		
		if (shdr->sh_type == SHT_SYMTAB)
		{
			sym_table = shdr->sh_addr;
			max_sym = shdr->sh_size / shdr->sh_entsize;
			
			for (s=1; s < max_sym; s++)
			{
				sym = (Elf32_Sym *)(sym_table + (s * shdr->sh_entsize));
				
				if (sym->st_shndx == SHN_UNDEF)
				{
					sym_name = string_table +sym->st_name;

					if ((ksym = FindKernelSymbol ((char *)sym_name)) != NULL)
					{
						sym->st_value = (uint32)ksym->addr;
					}
					else
					{
						KPRINTF ("Kernel symbol (%s) not found", (char *)sym_name);
						return -1;
					}
				}
				else if (sym->st_shndx == SHN_ABS)
				{
				}
				else if (sym->st_shndx == SHN_COMMON)
				{
					return -1;
				}
				else
				{
					sym_shdr = (Elf32_SHdr *)(shdr_table + sym->st_shndx * ehdr->e_shentsize);
										
					if (sym_shdr->sh_addr != 0)
					{
						sym->st_value = sym_shdr->sh_addr + sym->st_value;
					}
				}
			}
		}
	}
	
	return 0;
}



	
/*
 * ELFRelocate();
 *
 * Search for SHT_REL relocation sections and perform relocations
 */
	
int Elf32Relocate (Elf32_EHdr *ehdr)
{
	uint32 shdr_table;
	Elf32_SHdr *shdr, *symtable_shdr, *section_shdr;
	int32 t, s;
	int32 sym_num, rel_num;
	int32 sym_idx, rel_type;
	uint32 sym_table, rel_table;
	Elf32_Sym *sym;
	Elf32_Rel *rel;
	uint32 loc_r32, loc_pc32;
	uint32 section_base, section_ceiling;
	
	
		
	shdr_table = ehdr->e_shoff;
	
	for (t=0; t < ehdr->e_shnum; t++)
	{
		/* could use a reltable_shdr, as just shdr is confusing with symtable_shdr
			Also use reltable_addr, symtable_addr, reltable_entries, symtable_entries
		*/
		
		shdr = (Elf32_SHdr *)(shdr_table + (t * ehdr->e_shentsize));
		
		
		if (shdr->sh_type == SHT_REL)
		{
			rel_table = shdr->sh_addr;
			rel_num = shdr->sh_size / shdr->sh_entsize;
			
			/* FIXME 
			if (shdr->sh_link < ehdr->e_shnum || shdr->sh_info < ehdr->e_shnum)
				return -1;
			*/
			
			
			symtable_shdr = (Elf32_SHdr *)(shdr_table
							+ (shdr->sh_link * ehdr->e_shentsize));
							
			sym_table = symtable_shdr->sh_addr;
			sym_num = symtable_shdr->sh_size / symtable_shdr->sh_entsize;
			
			section_shdr = (Elf32_SHdr *)(shdr_table
							+ (shdr->sh_info * ehdr->e_shentsize));
			
			
			if ((section_shdr->sh_flags & SHF_ALLOC)
					&& section_shdr->sh_addr != 0)
			{
				section_base = section_shdr->sh_addr;
				section_ceiling = section_base + section_shdr->sh_size;
			
				for (s=0; s < rel_num; s++)
				{
					rel = (Elf32_Rel *)(rel_table + (s * shdr->sh_entsize));
					
					sym_idx = Elf32_R_SYM (rel->r_info);
					rel_type = Elf32_R_TYPE (rel->r_info);
					
					sym = (Elf32_Sym *)(sym_table + (sym_idx * symtable_shdr->sh_entsize));
					
					
					if (sym_idx > sym_num)
						return -1;
										
					
					switch (rel_type)
					{
						case R_386_NONE:
							break;
							
						case R_386_32:
							loc_r32 = section_base + rel->r_offset;
							
							if (ElfBoundsCheck (section_base, section_ceiling, loc_r32, 4) != 0)
								return -1;
							
							*(uint32 *)loc_r32 = *(uint32 *)loc_r32 + sym->st_value;
							break;
							
						case R_386_PC32:
							loc_pc32 = section_base + rel->r_offset;
							
							if (ElfBoundsCheck (section_base, section_ceiling, loc_pc32, 4) != 0)
								return -1;
							
							*(uint32 *)loc_pc32 = *(uint32 *)loc_pc32
										+ sym->st_value - (Elf32_Word)loc_pc32;
							break;
							
						default:
							break;
					}
				}
			}
		}
	}
	
	return 0;
}




/*
 * Elf32FindSymbol();
 */

Elf32_Sym *Elf32FindSymbol (char *name, Elf32_EHdr *ehdr)
{
	uint32 shdr_table;
	Elf32_SHdr *shdr;
	int32 t, s;
	
	int32 sym_num;
	
	uint32 sym_table = 0;
	uint32 string_table = 0;
	uint32 sym_name;
	Elf32_Sym *sym;

	
	shdr_table = ehdr->e_shoff;
		
	for (t = 1; t < ehdr->e_shnum; t++)
	{
		shdr = (Elf32_SHdr *)(shdr_table + (t * ehdr->e_shentsize));
		
		
		if (shdr->sh_type == SHT_STRTAB)
		{
			if (ehdr->e_shstrndx)
			{
				string_table = shdr->sh_addr;
			}
		}
	}
	
	if (string_table == 0)
		return NULL;
	
		
	/* The offsets point into second string table */
	/* Need to ignore global string table, whose index is in ELF header */
	
	
	
	for (t=0; t < ehdr->e_shnum; t++)
	{
		shdr = (Elf32_SHdr *)(shdr_table + (t * ehdr->e_shentsize));
		
		if (shdr->sh_type == SHT_SYMTAB)
		{
			sym_table = shdr->sh_addr;
			sym_num = shdr->sh_size / shdr->sh_entsize;
			
			for (s=0; s < sym_num; s++)
			{
				sym = (Elf32_Sym *)(sym_table + (s * shdr->sh_entsize));
				
				sym_name = string_table +sym->st_name;
				
				if (StrCmp((char *)sym_name, name) == 0)
				{
					return sym;
				}
			}
		}
	}
	
	return NULL;
}




/*
 *
 */
 
int ElfBoundsCheck (uint32 base, uint32 ceiling, uint32 addr, uint32 sz)
{
	return 0;
}




int Elf32Protect (Elf32_EHdr *ehdr)
{
	return 0;
}




/*
 *
 */

struct Resident *FindElfResident (Elf32_EHdr *ehdr)
{
	int t;
	uint32 shdr_table;
	vm_addr addr;
	Elf32_SHdr *shdr;
	struct Resident *resident;
	
	KPRINTF ("FindElfResident()");
	
	
	shdr_table = ehdr->e_shoff;
	
	for (t = 0; t < ehdr->e_shnum; t++)
	{
		shdr = (Elf32_SHdr *)(shdr_table + (t * ehdr->e_shentsize));

		if (shdr->sh_addr != 0)
		{
			addr = shdr->sh_addr;
						
			while (addr <= (shdr->sh_addr + shdr->sh_size) - sizeof (struct Resident))
			{
				resident = (struct Resident *)addr;
								
				if (resident->magic1 == RESIDENT_MAGIC1 && resident->magic2 == RESIDENT_MAGIC2 &&
					resident->magic3 == RESIDENT_MAGIC3 && resident->magic4 == RESIDENT_MAGIC4 &&
					resident->self == resident)
				{
					return resident;
				}
				
				addr++;
			}
		}
	}

	return NULL;
}

