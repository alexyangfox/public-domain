#ifndef KERNEL_DEVICE_H
#define KERNEL_DEVICE_H

#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/msg.h>
#include <kernel/proc.h>




/*
 *
 */

struct Device
{
	LIST_ENTRY (Device) device_entry;

	char *name;
	int version;
	int reference_cnt;
	struct Mutex mutex;					/* What was this mutex for?  the reference_cnt? */
	
	int (*init) (void *elf);
	void *(*expunge) (void);
	int (*opendevice) (int unit, void *ioreq, uint32 flags);
	int (*closedevice) (void *ioreq);
	void (*beginio) (void *ioreq);
	int (*abortio) (void *ioreq);
};




/*
 *
 */
 
struct StdIOReq
{
	struct Msg msg;
	struct Device *device;
	void *unitp;
	uint32 flags;
	int cmd;
	int result;
	int error;     /* IOERR Error state */
	struct AddressSpace *as;
	
	struct StdIOReq *abort_ioreq;
};




/* Quick IO
 */

#define IOF_QUICK			(1<<0)

/*
 * Universal commands
 */

#define CMD_ABORT			-1




/* Error codes
 */
 
#define IOERR_NOTSPECIFIED  -1
#define IOERR_OPENFAIL		-2  /* device/unit failed to open */
#define IOERR_ABORTED		-3  /* request aborted */
#define IOERR_NOCMD			-4  /* command not supported */
#define IOERR_BOUND         -5  /* Invalid sector/range of sectors */

#define IOERR_MEDIA_CHANGE  -6	/* CMD_MEDIA_PRESENT but others can return it as well */
#define IOERR_NO_MEDIA      -7  /* CMD_MEDIA_PRESENT but others can return it as well */
#define IOERR_WRITE_PROTECT -8  /* Write failed */
#define IOERR_BADMEDIA      -9  /* Write failed */
#define IOERR_TIMEOUT       -10
#define IOERR_SEEK          -11



/*
 * struct Unit;
 *
 * Generic structure for each device unit.
 */

struct Unit
{
    struct MsgPort MsgPort;
    uint32 flags;
    uint32 reference_cnt;
    void *data;
};




/*
 * Global variables
 */

extern struct RecMutex device_list_rmutex;
extern LIST_DECLARE (DeviceList, Device) device_list;






/*
 * Prototypes
 */

int AddDevice (struct Device *dev);
int RemDevice (struct Device *dev);

struct Device *FindDevice (char *name);

int OpenDevice (char *name, int unit, void *ioreq, uint32 flags);
int CloseDevice (void *ioreq);


void BeginIO (void *ioreq, struct MsgPort *reply_port);
void SendIO (void *ioreq, struct MsgPort *reply_port);
int DoIO (void *ioreq, struct MsgPort *reply_port);
int WaitIO (void *ioreq);

int DoIOAbortable (void *ioreq, struct MsgPort *reply_port, uint32 ksig_mask);
int WaitIOAbortable (void *ioreq, uint32 ksig_mask);

int CheckIO (void *ioreq);
int AbortIO (void *ioreq, struct MsgPort *reply_port);

char *CreateDeviceName (int dclass, char *prefix);






    
#endif
