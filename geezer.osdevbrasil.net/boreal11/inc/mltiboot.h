#ifndef __TL_MLTIBOOT_H
#define	__TL_MLTIBOOT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/* bits in mboot_info_t->flags */
#define MBF_MEMORY	0x00000001
#define MBF_ROOTDEV	0x00000002
#define MBF_CMDLINE	0x00000004
#define MBF_MODS	0x00000008
#define MBF_AOUT_SYMS	0x00000010
#define MBF_ELF_SHDR	0x00000020
#define MBF_MEM_MAP	0x00000040
#define MBF_DRIVE_INFO	0x00000080
#define MBF_CFG_TABLE	0x00000100
#define MBF_LOADER_NAME	0x00000200
#define MBF_APM_TABLE	0x00000400
#define MBF_VBE_INFO	0x00000800

/* This is the Multiboot system info struct. A pointer to this
struct is passed to the kernel in the EBX register. */
#pragma pack(1)
typedef struct
{
	uint32_t flags;			/* MBF_... bits */
/* conventional (< 1 meg) and extended (>= 1 meg) memory sizes; in Kbytes */
	uint32_t conv_mem;		/* if MBF_MEMORY */
	uint32_t ext_mem;		/* if MBF_MEMORY */
/* root device and pointer to kernel command line */
	uint32_t root_dev;		/* if MBF_ROOTDEV */
	uint32_t cmd_line;		/* if MBF_CMDLINE */
/* number of modules and pointer to mboot_mod_t array */
	uint16_t num_mods;		/* if MBF_MODS */
	uint16_t unused2;
	uint32_t mods_adr;		/* if MBF_MODS */
/* symbol table (MBF_AOUT_SYMS) or section table (MBF_ELF_SHDR) */
	uint32_t unused3[4];
/* pointer to mboot_range_t array (BIOS memory map a la INT 15h AX=E820h) */
	uint32_t map_len;		/* if MBF_MEM_MAP */
	uint32_t map_adr;		/* if MBF_MEM_MAP */
/* pointer to mboot_drive_t array (info about hard drives) */
	uint32_t drives_len;		/* if MBF_DRIVE_INFO */
	uint32_t drives_adr;		/* if MBF_DRIVE_INFO */
/* pointer to ROM config table from INT 15h AH=C0h */
	uint32_t config_table;		/* if MBF_CFG_TABLE */
/* pointer to boot loader name */
	uint32_t loader_name;		/* if MBF_LOADER_NAME */
/* pointer to APM table */
	uint32_t apm_table;		/* if MBF_APM_TABLE */
/* VBE video BIOS info */
	uint32_t vbe_ctrl_info;		/* if MBF_VBE_INFO */
	uint32_t vbe_mode_info;		/* if MBF_VBE_INFO */
	uint16_t vbe_mode;		/* if MBF_VBE_INFO */
	uint16_t vbe_iface_seg;		/* if MBF_VBE_INFO */
	uint16_t vbe_iface_off;		/* if MBF_VBE_INFO */
	uint16_t vbe_iface_len;		/* if MBF_VBE_INFO */
} mboot_info_t;

/* Multiboot modules */
#pragma pack(1)
typedef struct
{
	uint32_t start_adr;
	uint32_t end_adr;
	uint32_t cmd_line;
	uint32_t unused;
} mboot_mod_t;

/* Multiboot BIOS memory ranges */
#pragma pack(1)
typedef struct
{
	uint32_t len;		 /* =24=size of this struct */
	uint32_t adr, res_adr;
	uint32_t size, res_size;
	uint16_t type, res_type;
} mboot_range_t;

#pragma pack(1)
typedef struct
{
	uint32_t len;
	uint8_t drive_num;	 /* INT 13h drive number */
	uint8_t use_lba;
/* drive geometry values if use_lba==0 */
	uint16_t cyls;
	uint8_t heads;
	uint8_t sects;
/* ...I/O port fields can go here; adjust len accordingly... */
} mboot_drive_t;

/* Header placed in kernel file to make it compatible with Multiboot */
#pragma pack(1)
typedef struct
{
	uint32_t magic;
	uint32_t flags;
	uint32_t checksum;
/* a.out "kludge" fields: ignored in ELF kernels; required in
non-ELF kernels. These are PHYSICAL addresses: */
	uint32_t hdr_adr;
	uint32_t load_adr;
	uint32_t bss_adr;
	uint32_t end_adr;
	uint32_t entry;
} mboot_hdr_t;

#ifdef __cplusplus
}
#endif

#endif
