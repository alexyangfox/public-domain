#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/vm.h>
#include <kernel/sync.h>



/*
 * VM Global Variables
 */
uint32 modules_cnt;

uint32 free_pageframe_cnt;
uint32 free_pagetable_cnt;
uint32 free_memregion_cnt;
uint32 lru_pagetable_cnt;

uint32 pageframe_cnt;
uint32 pagetable_cnt;
uint32 memregion_cnt;

uint32 *pagetable;
struct MemRegion *memregion;
struct PmapDesc *pmapdesc;
struct Pageframe *pageframe;

vm_addr vm_temp_buf;



LIST_DEFINE (PageframeList) unused_pageframe_list;
LIST_DEFINE (MemRegionList) unused_memregion_list;
LIST_DEFINE (PmapDescList) lru_pmapdesc_list;

struct Mutex vm_mutex;

struct AddressSpace kernel_as;

uint32 realmode_base;
uint32 realmode_ceiling;
uint32 text_base;
uint32 text_ceiling;
uint32 data_base;
uint32 data_ceiling;

uint32 modules_base;
uint32 modules_ceiling;

uint32 heap_base;
uint32 heap_ceiling;


