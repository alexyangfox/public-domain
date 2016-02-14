#include <kernel/types.h>
#include <kernel/config.h>
#include <kernel/lists.h>
#include <kernel/proc.h>
#include <kernel/i386/i386.h>
#include <kernel/i386/multiboot.h>
#include <kernel/i386/module.h>
#include <kernel/fs.h>
#include <kernel/resident.h>
#include <kernel/reboot.h>



struct MultibootInfo *mbi;

uint32 mem_ceiling;

uint32 boot_heap_ptr;
uint32 boot_heap_base;
uint32 boot_heap_ceiling;

LIST_DEFINE (KModuleList) kmodule_list;
LIST_DEFINE (ResidentList) resident_list;

uint32 floppy_dma_base;
uint32 floppy_dma_ceiling;


struct SegmentDescriptor gdt[7];
struct SegmentDescriptor idt[49];
struct TablePointer gdtptr;
struct TablePointer idtptr;
struct TSS tss;

bool cpu_is_i386 = TRUE;
bool cpu_cpuid = FALSE;
char cpu_vendorid[13] = "CAVEATEMPTOR";
uint32 cpu_largest_std_function = 0;
uint32 cpu_largest_ext_function = 0;
uint64 cpu_freq = 0;
uint32 cpu_stepping;
uint32 cpu_model;
uint32 cpu_family;
uint32 cpu_mxcsr_mask;

union I387_State root_fpu_state __attribute__ ((aligned (16)));
uint32 kernel_cr3;

char root_current_dir[MAX_PATHNAME_SZ];
char root_process_name[5] = "root";
struct Process *root_process;
int root_pid;
int idle_pid;

struct Process *fpu_state_owner;

/*
 * CPU feature determination, see the Intel & AMD CPUID documentation.
 */

bool cpu_fpu      = FALSE;  /* Floating point unit on-board          */
bool cpu_vme      = FALSE;
bool cpu_debug    = FALSE;
bool cpu_pse      = FALSE;  /* Page Size Extension                   */
bool cpu_tsc      = FALSE;  /* Timestamp Counter                     */
bool cpu_msr      = FALSE;  
bool cpu_pae      = FALSE;  /* Physical Address Extension            */
bool cpu_mce      = FALSE;
bool cpu_cmpxchg  = FALSE;
bool cpu_apic     = FALSE;
bool cpu_sysenter = FALSE;  /* Intel sysenter/sysexit calls          */
bool cpu_mtrr     = FALSE;
bool cpu_gpe      = FALSE;
bool cpu_mca      = FALSE;
bool cpu_cmov     = FALSE;
bool cpu_pat      = FALSE;
bool cpu_pse36    = FALSE;  /* Page Size Extension - 36              */
bool cpu_pn       = FALSE;
bool cpu_mmx      = FALSE;
bool cpu_fxsr     = FALSE;
bool cpu_xmm      = FALSE;
bool cpu_syscall  = FALSE;  /* AMD syscall/sysret calls              */
bool cpu_mmxext   = FALSE;
bool cpu_3dnow    = FALSE;
bool cpu_3dnowext = FALSE;




char *cfg_boot_prefix="hd0";
int cfg_boot_verbose = 0;


struct TSS df_tss __attribute__ ((aligned (256)));
uint8 df_stack[KERNEL_STACK_SZ] __attribute__ ((aligned (16)));

uint8 root_stack[KERNEL_STACK_SZ] __attribute__ ((aligned (16)));


int reboot_how = KOS_REBOOT_REBOOT;
bool reboot_requested = FALSE;

bool acpi_is_enabled = FALSE;
