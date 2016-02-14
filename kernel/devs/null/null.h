#include <kernel/types.h>
#include <kernel/sync.h>




/*
 */

struct NullFilp
{
	struct Device *device;
};





/*
 *
 */

int null_init (void *elf);
void *null_expunge (void);
int null_opendevice (int unit, void *ioreq, uint32 flags);
int null_closedevice (void *ioreq);
void null_beginio (void *ioreq);
int null_abortio (void *ioreq);



