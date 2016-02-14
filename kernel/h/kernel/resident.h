#ifndef KERNEL_RESIDENT_H
#define KERNEL_RESIDENT_H

#include <kernel/types.h>
#include <kernel/lists.h>


/*
 * Kernel searches for Resident structures inbetween the data section of the kernel.
 * Sorts the Resident structures by priority and then calls init().
 *
 * Loadable Kernel Devices and Libraries also have a Resident structure in order to
 * find the initialization routine.
 */

struct Resident
{
    uint32 magic1;
    uint32 magic2;
    uint32 magic3;
    uint32 magic4;
    struct Resident *self;  /* Pointer to this structure */
    uint32 flags;
    uint32 version;
    uint32 type;
    int32  priority;
    LIST_ENTRY (Resident) resident_entry;
    char *name;
    char *id;
    int (*init)(void *elf);
    void *data;					/* Device/Library structure etc */
    void (*boot_alloc)(void);   /* So drivers/modules can allocate physical/dma memory */
};



#define RESIDENT_MAGIC1		(('R' << 24) | ('E' << 16) | ('S' <<8) | ('I'))
#define RESIDENT_MAGIC2		(('D' << 24) | ('E' << 16) | ('N' <<8) | ('T'))
#define RESIDENT_MAGIC3		(('K' << 24) | ('I' << 16) | ('E' <<8) | ('L'))
#define RESIDENT_MAGIC4		(('D' << 24) | ('E' << 16) | ('R' <<8) | ('X'))

#define RFLG_AUTOINIT		(1<<0)
#define RFLG_AFTERDOS		(1<<1)

#define RTYPE_DEVICE		1
#define RTYPE_LIBRARY		2


/*
 *
 */

extern LIST_DECLARE (ResidentList, Resident) resident_list;


    
#endif
