#ifndef KERNEL_I386_MODULE_H
#define KERNEL_I386_MODULE_H


#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/i386/elf.h>




/* Maybe move to config.h */
#define MODULE_NAME_SZ				1024




/*
 * struct KModule
 */

struct KModule
{
	Elf32_EHdr *elf_header;
	char *name;
	LIST_ENTRY(KModule) kmodule_list_entry;

	uint32 reference_cnt;
	int32 (* open_device)(void);
	int32 (* close_device)(void);
};





/*
 * Variables
 */

LIST_DECLARE (KModuleList, KModule);
extern LIST_DEFINE (KModuleList) kmodule_list;



/*
 * Prototypes
 */

void BootstrapModules(void);





#endif
