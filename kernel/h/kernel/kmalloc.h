#ifndef KERNEL_KMALLOC_H
#define KERNEL_KMALLOC_H


#include <kernel/types.h>
#include <kernel/vm.h>
#include <kernel/config.h>
#include <kernel/sync.h>



struct KMallocObject
{
	QUEUE_ENTRY (KMallocObject) free_entry;
};


struct KMallocSlab
{
	LIST_ENTRY (KMallocSlab) unfilled_slab_entry;
	QUEUE (KMallocObject) free_object_list;
	struct KMallocCache *cache;
	uint32 slab_sz;
	uint32 object_sz;
	uint32 max_objects;
	uint32 init_objects_cnt;
	uint32 free_cnt;
};


struct KMallocCache
{
	struct Mutex mutex;
	LIST (KMallocSlab) unfilled_slab_list;
	uint32 object_sz;
};



#define KMALLOC_SLAB_SZ		(ALIGN_UP(sizeof (struct KMallocSlab), KMAL_ALIGN))


void KMallocInit(void);
void *KMalloc (int32 size);
void KFree (void *addr);



#endif


