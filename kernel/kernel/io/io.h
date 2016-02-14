#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/msg.h>
#include <kernel/proc.h>
#include <kernel/fs.h>
#include <kernel/i386/elf.h>




/*
 * IO Manager IO-Request
 */

struct IOMReq
{
	struct Msg msg;
	struct Device *device;
	void *unitp;
	uint32 flags;
	int cmd;
	int result;		/* Should be rc */
	int error;     /* IOERR Error state */
	struct AddressSpace *as;
	
	struct StdIOReq *abort_ioreq;
	
	/* ---- Unique IO Manager data ---- */
	
	int rc;
	
	/* OpenDevice() */
	char *od_name;
	int od_unit;
	void *od_ioreq;
	uint32 od_flags;
	
	/* CloseDevice() */
	struct IOReq *cd_ioreq;
};




/*
 * IO Manager commands
 */

#define IOM_CMD_OPENDEVICE	200
#define IOM_CMD_CLOSEDEVICE	201




/*
 * Variables
 */
 
extern int iomanager_init_error;
extern int iomanager_pid;

extern struct MsgPort *iomanager_msgport;
extern struct Device iomanager_device;




/*
 * Prototypes
 */

int iomanager_init (void);
int iomanager_expunge (void);


int iomanager_opendevice (int unit, void *ioreq, uint32 flags);
int iomanager_closedevice (void *ioreq);
void iomanager_beginio (void *ioreq);
int iomanager_abortio (void *ioreq);

int32 IOManagerTask (void *arg);
void InitIOManagerTask (void);
void FiniIOManagerTask (void);

void InitResidentDevices (void);
void FiniResidentDevices (void);

void MountBootDevices (void);
void UnmountAll (void);

void *LoadDevice (char *name);
void UnloadDevice (void *elf);


struct Resident *FindElfResident (Elf32_EHdr *elf);


