#include <kernel/types.h>
#include <kernel/utility.h>
#include <kernel/dbg.h>
#include <kernel/fs.h>
#include <kernel/proc.h>
#include <kernel/device.h>
#include <kernel/error.h>
#include <kernel/buffers.h>
#include <kernel/block.h>
#include "iso9660.h"

uint8 cd_toc[804];
uint8 cd_buffer[2048];




/*
 *
 */

int CDIsValid (struct CDSB *cdsb, struct FSReq *fsreq)
{
	if (cdsb->validated == TRUE)
		return 0;
	
	fsreq->filp = NULL;
	fsreq->error = ENODEV;
	fsreq->rc = -1;
	return -1;
}	




/*
 *
 */

int CDRevalidate (struct CDSB *cdsb, int skip_validation)
{
	struct BlkReq blkreq;
	struct BlockDeviceStat stat;
	
	KPRINTF ("CDRevalidate() *******************");
	
	
	cdsb->validated = FALSE;
		
	if (CDStatDevice (cdsb, &stat) == 0)
	{
		KPRINTF ("Checking Media State");

		if (stat.media_state == MEDIA_INSERTED)
		{
			KPRINTF ("Media State == INSERTED");
		
			if (CDReadTOC (cdsb, &cd_toc) == 0)
			{
				KPRINTF ("CDReadTOC() OK");
			
				if (CDIsDataTrack (&cd_toc, 1) == TRUE)
				{
					KPRINTF ("CDIsDataTrack() OK");
				
					if (BufReadBlocks (cdsb->buf, &cd_buffer, &kernel_as, 16, 0, 2048) == 0)
					{
						KPRINTF ("BufReadBlocks() OK");
						
						if (CDValidateVolDesc (cdsb, &cd_buffer) == 0)
						{
							KPRINTF("CDValidateVolDesc() OK");
							return 0;
						}
					}
				}
			}
		}
	}
	
	KPRINTF ("****************** CDRevalidate FAIL");
		
	return -1;
}




/*
 *
 */

void CDInvalidate (struct CDSB *cdsb)
{
	struct CDFilp *filp;
	struct Msg *msg;
	struct FSReq *fsreq;
	
	KPRINTF ("**** CDInvalidate()");

	
	cdsb->validated = FALSE;
	InvalidateBuf (cdsb->buf);
	
	
	filp = LIST_HEAD (&cdsb->active_filp_list);
	
	while (filp != NULL)
	{
		filp->invalid = 1;
		
		LIST_REM_HEAD (&cdsb->active_filp_list, cdsb_filp_entry);
		
		LIST_ADD_TAIL (&cdsb->invalid_filp_list, filp, cdsb_filp_entry);

		filp = LIST_HEAD (&cdsb->active_filp_list);
	}
}




/*
 * CDStatDevice();
 */

int CDStatDevice (struct CDSB *cdsb, struct BlockDeviceStat *stat)
{
	struct BlkReq blkreq;
	int rc;
	
	
	KPRINTF ("CDStatDevice()");

	blkreq.as = &kernel_as;
	blkreq.device = cdsb->device;
	blkreq.unitp = cdsb->unitp;
	blkreq.stat = stat;
	blkreq.cmd = BLK_CMD_MEDIA_PRESENT;

	rc = DoIO (&blkreq, NULL);
	SetError (0);

	if (rc != 0)
		KPRINTF ("*********** CDStatDevice() FAIL");

	return rc;
}




/*
 * CDReadToc();
 */

int CDReadTOC (struct CDSB *cdsb, void *cd_toc)
{
	struct BlkReq blkreq;
	uint8 atapi_packet[12];
	int rc;
	
	KPRINTF ("CDReadTOC()");
	
	atapi_packet[0] = 0x43;
	atapi_packet[1] = 0;
	atapi_packet[2] = 0;
	atapi_packet[3] = 0;
	atapi_packet[4] = 0;
	atapi_packet[5] = 0;
	atapi_packet[6] = 0;  /* TRACK */
	atapi_packet[7] = 804 >> 8;
	atapi_packet[8] = 804 & 0xff;
	atapi_packet[9] = 0;
	atapi_packet[10] = 0;
	atapi_packet[11] = 0;
	
	blkreq.as = &kernel_as;
	blkreq.device = cdsb->device;
	blkreq.unitp = cdsb->unitp;
	blkreq.cmd_packet_addr = atapi_packet;
	blkreq.cmd_packet_nbytes = 12;
	blkreq.data_buf_addr = cd_toc;
	blkreq.data_nbytes = 804;
	blkreq.dir = 0;
	blkreq.cmd = BLK_CMD_SCSI;

	rc = DoIO (&blkreq, NULL);

	KASSERT (rc == 0);
	
	SetError (0);

	return rc;
}




/*
 *
 */

bool CDIsDataTrack (void *cd_toc, int track)
{
	/* FIXME: */
	
	/* Determine number of tracks */
	
	/* Read first track entry */
	
	return TRUE;
}




/*
 *
 */

int CDValidateVolDesc (struct CDSB *cdsb, void *cd_buffer)
{
	struct ISOVolDesc *vd;
	
	KPRINTF ("CDValidateVolDesc()");
	
	/* FIXME:  cdsb->validated might be uninitialized */
	
	vd = cd_buffer;
	
	KPRINTF ("%02x %02x %02x %02x %02x",
		vd->id[0], vd->id[1], vd->id[2], vd->id[3], vd->id[4]);
	
	if (vd->type[0] != ISO_VD_PRIMARY)
		return -1;
	
	if (vd->id[0] != 'C' || vd->id[1] != 'D' || vd->id[2] != '0' || vd->id[3] != '0' || vd->id[4] != '1')
		return -1;
	
	/* Valid, now copy either this structure or fields to bpb */
	
	MemCpy (&cdsb->pvd, vd, sizeof (struct ISOVolDesc));
	
	
	/* FIXME:  Init root node here */
	
	CDInitRootNode (cdsb, (struct ISODirEntry *)cdsb->pvd.root_directory_entry);


	KPRINTF ("************* CD VALIDATED");	
	
	cdsb->validated = TRUE;

	return 0;	
}











