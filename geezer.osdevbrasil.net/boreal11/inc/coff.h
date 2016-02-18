#ifndef __TL_COFF_H
#define	__TL_COFF_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h> /* uint8_t, uint16_t, uint32_t */

#pragma pack(1)
typedef struct
{
	uint16_t magic;
	uint16_t num_sects;
	uint32_t time_date;
	uint32_t symtab_offset;
	uint32_t num_syms;
	uint16_t aout_hdr_size;
	uint16_t flags;
/* for executable COFF file, a.out header starts here */
} coff_file_t; /* 20 bytes */

/* Win32 PE COFF a.out header is longer than this */
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
} coff_aout_t; /* 28 bytes */

/* Win32 PE COFF: phys_adr=virtual section size (size-in-memory),
size=raw section size (size-on-disk)

from OUTCOFF.C of the NASM source code:

(3) Win32 uses some extra flags into the section header table:
it defines flags 0x80000000 (writable), 0x40000000 (readable)
and 0x20000000 (executable), and uses them in the expected
combinations. It also defines 0x00100000 through 0x00700000 for
section alignments of 1 through 64 bytes.
*/
#pragma pack(1)
typedef struct
{
	char name[8];
	uint32_t phys_adr;
	uint32_t virt_adr;
	uint32_t size;
	uint32_t offset;
	uint32_t relocs_offset;
	uint32_t line_nums_offset;
	uint16_t num_relocs;
	uint16_t num_line_nums;
	uint32_t flags;
} coff_sect_t; /* 40 bytes */

#pragma pack(1)
typedef struct
{
	uint32_t adr;
	uint32_t symtab_index;
	uint16_t type;
} coff_reloc_t; /* 10 bytes */

#pragma pack(1)
typedef struct
{
	union
	{
		char name[8];
		struct
		{
			uint32_t zero;
			uint32_t strtab_index;
		} x;
	} x;
	uint32_t val;
	uint16_t sect_num;
	uint16_t type;
	uint8_t sym_class;
	uint8_t num_aux;
} coff_sym_t; /* 18 bytes */

#ifdef __cplusplus
}
#endif

#endif
