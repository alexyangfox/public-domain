#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H




/*
 * Kernel - User VM Boundaries
 */

#define VM_KERNEL_BASE				0x00000000
#define VM_KERNEL_CEILING			0x3ffff000
#define VM_USER_BASE				0x40000000
#define VM_USER_CEILING				0xfffff000  /* Maybe 0-64k ? */



/* The size of some of the tables on the kernel heap are calculated as
 * a fraction of the total number of pageframes.  Lower bounds
 * prevent the arrays being to small on low memory systems.
 */
 
#define MEMREGION_MIN_CNT		1024
#define MEMREGION_MAX_CNT		32768
#define MEMREGION_DIVISOR_CNT	4

#define PROCESS_MIN_CNT			64
#define PROCESS_MAX_CNT			2048
#define PROCESS_DIVISOR_CNT		32

#define PAGETABLE_MIN_CNT		512  	/* 2MB */
#define PAGETABLE_MAX_CNT		8192	/* 32MB */
#define PAGETABLE_DIVISOR_CNT	8

#define PMAPDESC_MIN_CNT		512     /* Needs more than that? */
#define PMAPDESC_MAX_CNT		8192
#define PMAPDESC_DIVISOR_CNT	8

#define CACHEDESC_MIN_CNT		128
#define CACHEDESC_MAX_CNT		262144
#define CACHEDESC_DIVISOR_CNT	8
#define CACHEDESC_HASH_CNT		1024

#define MIN_MEM_FOR_16MB_BASE	0x2000000
#define MIN_MEM_FOR_PAENX		0x2000000

#define FLOPPY_DMA_SZ			0x10000



/*
 */

#define INTERRUPT_STACK_SZ		8192
#define INTERRUPT_STACK_CNT		1
#define KERNEL_STACK_SZ			8192
#define USER_STACK_SZ			1048576

/*
 */

#define MMAP_DEFAULT_ALIGN			0x10000

/* Add boot heap align and DMA align? */
/* Add alignment for individual arrays within heap separate from
	underlying boot heap align */


/*
 * KMalloc
 */

#define SLAB_SZ				0x10000
#define MIN_LARGE_ALLOC_SZ	8192
#define KMAL_ALIGN			16

#define KMALLOC_CACHE_CNT 9
#define KMALLOC_CACHE_SIZES {32,64,128,256,512,1024,2048, 4096, 8192}











#endif
