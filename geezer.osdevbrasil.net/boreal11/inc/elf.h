#ifndef __TL_ELF_H
#define	__TL_ELF_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h> /* uint8_t, uint16_t, uint32_t */

#pragma pack(1)
typedef struct
{
	uint32_t magic;
	uint8_t bitness;
	uint8_t endian;
	uint8_t elf_ver_1;
	uint8_t res[9];
	uint16_t file_type;
	uint16_t machine;
	uint32_t elf_ver_2;
	uint32_t entry;
	uint32_t phtab_offset;
	uint32_t shtab_offset;
	uint32_t flags;
	uint16_t file_hdr_size;
	uint16_t ph_size;
	uint16_t num_ph;
	uint16_t sh_size;
	uint16_t num_sects;
	uint16_t shstrtab_index;
} elf_file_t;

#pragma pack(1)
typedef struct
{
	uint32_t sect_name;
	uint32_t type;
	uint32_t flags;
	uint32_t virt_adr;
	uint32_t offset;
	uint32_t size;
	uint32_t link;
	uint32_t info;
	uint32_t align;
	uint32_t ent_size;
} elf_sect_t;

#pragma pack(1)
typedef struct
{
	uint32_t adr;
	uint8_t type;
/* 16-bit Turbo C sez: "Bitfields must be signed or unsigned int" */
//	uint32_t symtab_index : 24;
	unsigned symtab_index : 24;
	uint32_t addend;
} elf_reloc_t;

#pragma pack(1)
typedef struct
{
	uint32_t name;
	uint32_t value;
	uint32_t size;
	unsigned type : 4;
	unsigned binding : 4;
	uint8_t zero;
	uint16_t section;
} elf_sym_t;

#pragma pack(1)
typedef struct
{
	uint32_t type;
	uint32_t offset;
	uint32_t virt_adr;
	uint32_t phys_adr;
	uint32_t disk_size;
	uint32_t mem_size;
	uint32_t flags;
	uint32_t align;
} elf_seg_t; /* 32 bytes */

#ifdef __cplusplus
}
#endif

#endif
