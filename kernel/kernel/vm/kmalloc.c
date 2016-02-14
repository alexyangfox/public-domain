#include <kernel/types.h>
#include <kernel/vm.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/config.h>





struct KMallocSlab *AllocSlab(struct KMallocCache *kcache);
void FreeSlab (struct KMallocSlab *slab);



uint32 kmalloc_object_sz[] = KMALLOC_CACHE_SIZES;
struct KMallocCache kcache[KMALLOC_CACHE_CNT];


extern bool init_proc_complete;


/*
 * InitKMalloc();
 */

void InitKMalloc(void)
{
	uint32 kc;
	
	/* Init mutex for each queue */
	
	for (kc=0; kc<KMALLOC_CACHE_CNT; kc++)
	{
		MutexInit (&kcache[kc].mutex);
		LIST_INIT(&kcache[kc].unfilled_slab_list);
		kcache[kc].object_sz = kmalloc_object_sz[kc];
	}
}




/*
 * KMalloc();
 */

void *KMalloc (int32 size)
{
	struct KMallocSlab *slab;
	struct KMallocObject *obj;
	void *mem;
	int32 kc;
	uint32 obj_offset;
	
	KASSERT (init_proc_complete == TRUE);
		
	
	if (size == 0)
		return NULL;
	
		
	if (size <= kcache[KMALLOC_CACHE_CNT-1].object_sz)
	{
		for (kc=0; kc < KMALLOC_CACHE_CNT; kc ++)
		{
			if (size <= kcache[kc].object_sz)
				break;
		}
		
		
		MutexLock (&kcache[kc].mutex);

		
		slab = LIST_HEAD(&kcache[kc].unfilled_slab_list);

		if (slab == NULL)
		{
			slab = (struct KMallocSlab *) KMap (SLAB_SZ, VM_PROT_READWRITE);
			if (slab == (void *)MAP_FAILED)
			{
				MutexUnlock (&kcache[kc].mutex);
				return NULL;
			}
			
			/* New slab,so add to unfilled list */
			
			LIST_ADD_HEAD (&kcache[kc].unfilled_slab_list, slab, unfilled_slab_entry);
			QUEUE_INIT(&slab->free_object_list);
		
			slab->cache = &kcache[kc];
			slab->slab_sz = SLAB_SZ;
			slab->object_sz = kcache[kc].object_sz; /* Needed or not ? */
			slab->max_objects = (slab->slab_sz
								- ALIGN_UP(sizeof (struct KMallocSlab), slab->object_sz))
								/ slab->object_sz;
			
			slab->init_objects_cnt = 0;
			slab->free_cnt = slab->max_objects;
		}
		
		
		if (slab->init_objects_cnt < slab->max_objects)
		{
			obj_offset = ALIGN_UP(sizeof (struct KMallocSlab), slab->object_sz)
							+ (slab->init_objects_cnt * slab->object_sz);

			obj = (struct KMallocObject *) ((vm_addr)slab + obj_offset);
			QUEUE_ADD_TAIL (&slab->free_object_list, obj, free_entry);
			slab->init_objects_cnt++;
		}
		
		
		obj = QUEUE_HEAD(&slab->free_object_list);
		QUEUE_REM_HEAD(&slab->free_object_list, free_entry);
		
		
		/* Same kcache pointer here */
		
		/* Remove it from the unfilled list if non free */
		
		slab->free_cnt--;
		if (slab->free_cnt == 0)
			LIST_REM_ENTRY (&kcache[kc].unfilled_slab_list, slab, unfilled_slab_entry);

		MutexUnlock (&kcache[kc].mutex);
				
		return (void *)obj;
	}
	else
	{		
		mem = (void *) KMap (size, VM_PROT_READWRITE);
		
		if (mem == (void *)MAP_FAILED)
			return NULL;
		else
		{
			return mem;
		}
	}
}	




/*
 * KFree();
 */
 
void KFree (void *mem)
{
	struct KMallocObject *obj;
	struct KMallocObject *obj_test;
	struct KMallocSlab *slab;
	struct KMallocCache *kcache;


	KASSERT (init_proc_complete == TRUE);
	
	
	obj = (struct KMallocObject *)mem;
	
	if (((vm_addr)mem % MMAP_DEFAULT_ALIGN) != 0)
	{
		slab = (struct KMallocSlab *) ALIGN_DOWN ((vm_addr)mem, MMAP_DEFAULT_ALIGN);
		kcache = slab->cache;

		MutexLock (&kcache->mutex);

		obj_test = LIST_HEAD(&slab->free_object_list);
		
		while (obj_test != NULL)
		{
			KASSERT (obj != obj_test);
			obj_test = LIST_NEXT (obj_test, free_entry);
		}
		

		
		QUEUE_ADD_TAIL (&slab->free_object_list, obj, free_entry);
		
		if (slab->free_cnt == 0)
		{
			LIST_ADD_HEAD (&kcache->unfilled_slab_list, slab, unfilled_slab_entry);
		}
		
		slab->free_cnt ++;
		
		if (slab->free_cnt == slab->max_objects)
		{
			LIST_REM_ENTRY (&kcache->unfilled_slab_list, slab, unfilled_slab_entry);
			KUnmap ((vm_addr)slab);
		}

		MutexUnlock (&kcache->mutex);
	}
	else
	{
		KUnmap ((vm_addr)mem);
	}
}
