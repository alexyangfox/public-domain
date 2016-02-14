#ifndef KERNEL_VM_H
#define KERNEL_VM_H

#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/sync.h>
#include <kernel/arch.h>




/*
 *
 */
 
#define PF_DIR_READ		0
#define PF_DIR_WRITE	1




/*
 * MMap, MemRegion types, modes and return values
 */
 

#define MR_TYPE_UNALLOCATED			0
#define MR_TYPE_FREE				1
#define MR_TYPE_ANON				2
#define MR_TYPE_PHYS				3
#define MR_TYPE_RESERVED			4

#define MR_FLAGS_NONE		0
#define MR_FLAGS_LAZY		(1<<0)
#define MR_FLAGS_FIXED		(1<<1)

#define VM_PROT_NONE		0
#define VM_PROT_READ		(1<<0)
#define VM_PROT_WRITE		(1<<1)
#define VM_PROT_EXEC		(1<<2)
#define VM_PROT_ALL			(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC)
#define VM_PROT_READWRITE 	(VM_PROT_READ | VM_PROT_WRITE)
#define VM_PROT_READEXEC	(VM_PROT_READ | VM_PROT_EXEC)

#define VM_PROT_NOUNMAP		(1<<4)




/*
 * UMap() return value and flags
 */

#define MAP_FAILED		((uint32)-1)
#define MAP_FIXED			(1<<0)




/*
 *
 */

#define PF_RESERVED			0
#define PF_FREE				1
#define PF_ACTIVE			2




/*
 *
 */
 
#define PMAP_THROWAWAY		0 
#define PMAP_WIRED			1



/*
 * struct MemRegion
 */

struct MemRegion
{
	vm_addr base_addr;
	vm_addr ceiling_addr;

	LIST_ENTRY (MemRegion) sorted_entry;
	LIST_ENTRY (MemRegion) free_entry;
	LIST_ENTRY (MemRegion) unused_entry;
		
	struct AddressSpace *as;

	uint32 type;
	uint32 prot;
	uint32 flags;
	vm_addr phys_base_addr;
	
	struct Pageframe *pageframe_hint;
	LIST (Pageframe) pageframe_list;	
};




/*
 * struct Pageframe
 */
 
struct Pageframe
{
	vm_addr physical_addr;
	vm_addr virtual_addr;

	struct MemRegion *mr;
	
	LIST_ENTRY (Pageframe) unused_entry;
	LIST_ENTRY (Pageframe) memregion_entry;
	uint32 state;
};




/*
 * struct PmapDescriptor
 */

struct PmapDesc
{
	LIST_ENTRY (PmapDesc) pmap_entry;
	LIST_ENTRY (PmapDesc) lru_entry;
	
	struct Pmap *pmap;
	uint32 *addr;
	struct PmapDesc *parent_pdesc;
	uint32 pde_idx;
	uint32 reference_cnt;
	bool wired;
};





/*
 * struct AddressSpace
 */

struct AddressSpace
{
	LIST (MemRegion) sorted_memregion_list;
	LIST (MemRegion) free_memregion_list;
	struct MemRegion *hint;
	struct Pmap pmap;
	int page_cnt;
	bool active;
};






/*
 * VM variables
 */

#define VM_TEMP_BUF_SZ 0x10000
extern vm_addr vm_temp_buf;
 
extern uint32 modules_cnt;

extern uint32 free_pageframe_cnt;
extern uint32 free_pagetable_cnt;
extern uint32 free_memregion_cnt;
extern uint32 lru_pagetable_cnt;

extern uint32 pageframe_cnt;
extern uint32 pagetable_cnt;
extern uint32 memregion_cnt;

extern uint32 *pagetable;
extern struct MemRegion *memregion;
extern struct Pageframe *pageframe;
extern struct PmapDesc *pmapdesc;

extern LIST_DECLARE (MemRegionList, MemRegion) unused_memregion_list;
extern LIST_DECLARE (PageframeList, Pageframe) unused_pageframe_list;
extern LIST_DECLARE (PmapDescList, PmapDesc) lru_pmapdesc_list;

extern struct Mutex vm_mutex;



extern struct AddressSpace kernel_as;

extern uint32 realmode_base;
extern uint32 realmode_ceiling;
extern uint32 text_base;
extern uint32 text_ceiling;
extern uint32 data_base;
extern uint32 data_ceiling;
extern uint32 modules_base;
extern uint32 modules_ceiling;
extern uint32 heap_base;
extern uint32 heap_ceiling;





/*
 * VM Functions
 */

struct AddressSpace *SwitchAddressSpace (struct AddressSpace *as);
int CreateAddressSpace (struct AddressSpace *as);
void FreeAddressSpace (struct AddressSpace *as);
int CreateNullAddressSpace (struct AddressSpace *as);
int DuplicateAddressSpace (struct AddressSpace *as, struct AddressSpace *new_as);
int AllocDupAddressSpace (struct AddressSpace *src_as, struct AddressSpace *dst_as);


struct Pageframe *ObtainPageframe (uint32 va);
void ReleasePageframe (struct Pageframe *pf);
int AllocPageframes (struct MemRegion *mr);
int AllocDupPageframes (struct MemRegion *mr_dst, struct MemRegion *mr_src);
void FreePageframes (struct MemRegion *mr);

struct Pageframe *FindPageframe (struct MemRegion *mr, vm_addr addr);
void InsertPageframe (struct MemRegion *mr, struct Pageframe *pf);

struct MemRegion *MemRegionFindFree (struct AddressSpace *as, vm_addr addr);
struct MemRegion *MemRegionFindSorted (struct AddressSpace *as, vm_addr addr);
struct MemRegion *MemRegionCreate (struct AddressSpace *as, vm_offset addr,
								vm_size size, uint32 flags, uint32 prot, uint32 type);
void MemRegionDelete (struct AddressSpace *as, struct MemRegion *mr);


bool PmapInit (struct AddressSpace *as);
void PmapDestroy (struct AddressSpace *as);
void PmapKEnter (vm_offset va, vm_offset pa, uint32 prot);
void PmapKProtect (vm_offset va, uint32 prot);
void PmapKRemove (vm_offset va);
bool PmapEnter (struct Pmap *pmap, vm_offset va, vm_offset pa, uint32 prot);
bool PmapProtect (struct Pmap *pmap, vm_offset va, uint32 prot);
bool PmapRemove (struct Pmap *pmap, vm_offset va);
uint32 *PmapAllocPagetable (struct Pmap *pmap, bool wired);
void PmapFreePagetable (struct Pmap *pmap, uint32 *pt);
void PmapFlushTLBs (void);
void PmapFlushKernelTLBs (void);
void PmapSwitch (struct Pmap *pmap);
struct AddressSpace *PmapSwitchAddressSpace(struct AddressSpace *as);





void ClearPages (struct MemRegion *mr);
void PmapKEnterRegion (struct MemRegion *mr);
void PmapKRemoveRegion (struct MemRegion *mr);
int PmapEnterRegion (struct MemRegion *mr);
void PmapRemoveRegion (struct MemRegion *mr);

void WriteProtectEnable (void);
void WriteProtectDisable (void);








vm_offset KMap (vm_size len, uint32 prot);
void KUnmap (vm_offset addr);
vm_offset KMapPhys (vm_addr pa, vm_size len, uint32 prot);
vm_offset KMapProtect (vm_addr addr, uint32 prot);


vm_offset KMapISADMA (uint32 size, uint32 align);
void KUnmapISADMA (vm_addr addr);


vm_offset UMap (vm_addr addr, vm_size len, uint32 prot, uint32 flags);
int UUnmap (vm_offset addr);
int UProtect (vm_addr addr, uint32 prot);
vm_size USizeOfMap (vm_addr addr);


int PageFault (vm_addr pf_addr, int direction, int privilege);





int CopyIn (struct AddressSpace *as, void *dst, const void *src, size_t sz);
int CopyInStr (struct AddressSpace *as, char *dst, const char *src, size_t sz);
int CopyOut (struct AddressSpace *as, void *dst, const void *src, size_t sz);

int DoCopyIn (void *dst, const void *src, size_t sz);
int DoCopyInStr (char *dst, const char *src, size_t sz);
int DoCopyOut (void *dst, const void *src, size_t sz);


void MemSet (void *mem, int c, size_t sz);
void MemCpy (void *dst, const void *src, size_t sz);






/*
 * VM Macros
 */

#define ALIGN_UP(val, alignment)								\
	(((val + alignment - 1)/alignment)*alignment)
#define ALIGN_DOWN(val, alignment)								\
			((val) - ((val) % (alignment)))



#endif
