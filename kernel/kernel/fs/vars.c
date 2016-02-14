#include <kernel/device.h>
#include <kernel/fs.h>
#include <kernel/lists.h>
#include <kernel/sync.h>


LIST_DEFINE (BootEntryList) bootentry_list;
LIST_DEFINE (MountList) mount_list;


struct Mutex mount_mutex;
struct RWLock mountlist_rwlock;

struct Mount root_mount;
struct Mutex root_mutex;         /* Needed or not? */
LIST_DEFINE (RootFilpList) root_filp_list;


