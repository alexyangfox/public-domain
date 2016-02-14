#ifndef KERNEL_I386_MULTIBOOT_H
#define KERNEL_I386_MULTIBOOT_H


#include <kernel/types.h>




/* -----------------------------------------------------------------------------
** struct MultibootHeader
** -----------------------------------------------------------------------------
*/

struct MultibootHeader
{
	uint32 magic;
	uint32 flags;
	uint32 checksum;
	uint32 header_addr;
	uint32 load_addr;
	uint32 load_end_addr;
	uint32 bss_end_addr;
	uint32 entry_addr;
};




/* MultibootHeader values
*/

#define MULTIBOOT_HEADER_MAGIC		0x1BADB002
#define MULTIBOOT_HEADER_FLAGS		0x00010003




/* -----------------------------------------------------------------------------
** struct ElfSectionHeaderTable
** -----------------------------------------------------------------------------
*/

struct ElfSectionHeaderTable
{
	uint32 num;
	uint32 size;
	uint32 addr;
	uint32 shndx;
};




/* -----------------------------------------------------------------------------
** struct MultibootInfo
** -----------------------------------------------------------------------------
*/

struct MultibootInfo
{
	uint32 flags;
	uint32 mem_lower;
	uint32 mem_upper;
	uint32 boot_device;
	uint32 cmdline;
	uint32 mods_count;
	struct Module *mods_addr;
	struct ElfSectionHeaderTable elf_sec;
	uint32 mmap_length;
	uint32 mmap_addr;
	uint32 drives_count;
	uint32 drives_addr;
	uint32 config_table;
	uint32 boot_loader_name;
};




/* MultibootInfo.flags flags
*/

#define MI_MEM_VALID		1<<0
#define MI_BOOT_VALID		1<<1
#define MI_CMD_VALID		1<<2
#define MI_MODS_VALID		1<<3
#define MI_ELF_VALID		1<<5
#define MI_MMAP_VALID		1<<6
#define MI_DRIVES_VALID		1<<7
#define MI_CONFIG_VALID		1<<8
#define MI_NAME_VALID		1<<9




/* -----------------------------------------------------------------------------
** struct Module
** -----------------------------------------------------------------------------
*/

struct MultibootModule
{
	uint32 mod_start;
	uint32 mod_end;
	char *mod_name;
	uint32 reserved;
};




/* -----------------------------------------------------------------------------
** struct MemoryMap
** -----------------------------------------------------------------------------
*/

struct MemoryMap
{
	uint32 size;
	uint64 base_addr;
	uint64 length;
	uint32 type;
};


/* struct MemoryMap.type values
*/

#define MB_MEM_RESERVED		0
#define MB_MEM_AVAIL 		1




/*
**
*/

extern struct MultibootInfo *mbi;
extern void _stext;
extern void _etext;
extern void _sdata;
extern void _edata;
extern void _sbss;
extern void _ebss;

extern uint32 mem_ceiling;

extern uint32 boot_heap_ptr;
extern uint32 boot_heap_base;
extern uint32 boot_heap_ceiling;






#endif
