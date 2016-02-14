#include <kernel/types.h>
#include <kernel/fs.h>
#include <kernel/lists.h>
#include <kernel/sync.h>
#include <kernel/utility.h>
#include <kernel/kmalloc.h>
#include <kernel/dbg.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/device.h>
#include <kernel/resident.h>
#include "io.h"




/*
 * IOManagerTask()
 *
 * Parent process of all device drivers, including nested drivers.
 *
 * Handles opening and closing of devices.  Processes calling OpenDevice()
 * or CloseDevice() send a message to this task which then does the actual
 * work of opening a device.
 *
 * Nested devices are a special case.  If a drivers's opendevice() function
 * calls OpenDevice() the IOManagerTask calls the nested driver's opendevice()
 * function instead of sending a message to itself.
 *
 * This places a restriction on device drivers in that only the opendevice()
 * function can call OpenDevice(), no device driver threads can do so.
 */

int32 IOManagerTask (void *arg)
{
	struct IOMReq *iomreq;
	struct Msg *msg;
	uint32 signals;
	
	
	InitIOManagerTask();
		
	
	while (1)
	{
		signals = KWait ((1 << iomanager_msgport->signal) | SIGF_TERM);
		
		if (signals & (1 << iomanager_msgport->signal))
		{
			while ((msg = GetMsg (iomanager_msgport)) != NULL)
			{
				SetError (0);
			
				iomreq = (struct IOMReq *)msg;
				
				switch (iomreq->cmd)
				{
					case IOM_CMD_OPENDEVICE:
						iomreq->rc = OpenDevice (iomreq->od_name, iomreq->od_unit, iomreq->od_ioreq, iomreq->od_flags);
						iomreq->error = GetError();
						ReplyMsg (msg);
						
						break;
						
					case IOM_CMD_CLOSEDEVICE:
						iomreq->rc = CloseDevice (iomreq->cd_ioreq);
						iomreq->error = GetError();
						ReplyMsg (msg);
						break;
						
					default:
						iomreq->rc = -1;
						iomreq->error = ENOSYS;
						ReplyMsg (msg);
						break;
				}
			}
		}
		
		if (signals & SIGF_TERM)
		{
			FiniIOManagerTask();
		}
	}
}




/*
 * FIXME:  Move most of this code out of the IO Manager
 */

void InitIOManagerTask (void)
{
	LIST_INIT(&resident_list);
	LIST_INIT(&device_list);


	iomanager_pid = GetPID();
	
	if ((iomanager_msgport = CreateMsgPort()) != NULL)
	{
		InitResidentDevices();
	
		iomanager_init_error = 0;
		KSignal (GetPPID(), SIG_INIT);
		return;
	}
		
	iomanager_init_error = -1;
	KSignal (GetPPID(), SIG_INIT);
	Exit(-1);
}




/*
 * 
 */
 
void FiniIOManagerTask (void)
{
	KPRINTF ("FiniResidentDevices()");
	
	FiniResidentDevices();
	
	KPRINTF ("IOManager exiting");
	Exit(0);
}




