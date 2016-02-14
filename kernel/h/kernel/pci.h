#ifndef KERNEL_PCI_H
#define KERNEL_PCI_H

#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/callback.h>



/*
 * IO Request passed to PCI Manager
 */

struct PCIReq
{
	struct Msg msg;
	struct Device *device;
	void *unitp;
	uint32 flags;
	
	int32 cmd;
	int rc;
	int error;
		
	struct AddressSpace *as;

	struct StdIOReq *abort_ioreq;
	
	
	/* PCI dependent arguments and results */
};




/*
 * PCI Commands
 */






#endif


