#include <kernel/types.h>
#include <kernel/utility.h>
#include <kernel/dbg.h>
#include <kernel/fs.h>
#include <kernel/proc.h>
#include <kernel/device.h>
#include <kernel/error.h>
#include <kernel/buffers.h>
#include <kernel/block.h>
#include "fat.h"




/*
 * Buffers for reading BPB and FS-INFO sectors.
 */

static uint8 bpb_sector[512];
static uint8 fsinfo_sector[512];





int FatIsValid (struct FatSB *fsb, struct FSReq *fsreq)
{
	if (fsb->validated == TRUE)
		return 0;
	
	fsreq->filp = NULL;
	fsreq->error = ENODEV;
	fsreq->rc = -1;
	return -1;
}	



/*
 *
 */

int FatRevalidate (struct FatSB *fsb, int skip_validation)
{
	KPRINTF ("**** FatRevalidate();");
	
	return FatValidateBPB (fsb, 0);
}


/*
 * FatValidateBPB();
 *
 * The correct validation from w2k's FASTFAT is: 
 * - jump 
 * - BPB::SectorSize is a power of 2 from 128 to 4069 
 * - BPB::SectorsPerCluster is a power of 2 from 1 to 128 
 * - BPB::ReservedSectors is nonzero 
 * - BPB::Fats is nonzero 
 * - either BPB::Sectors or BPB::LargeSectors is nonzero 
 * - BPB::MediaDescriptor is one of 
 *      0 1 0xf0 0xf8 0xf9 0xfa 0xfb 0xfc 0xfd 0xfe 0xff 
 * - either BPB::SectorsPerFat or BPB::LargeSectorsPerFat is nonzero 
 * - for FAT32 ( BPB::SectorsPerFat == 0 ), FsVersion must be zero 
 * - for FAT12/16 ( BPB::SectorsPerFat != 0  ), RootEntries must be 
 *      nonzero 
 * - the sector size reported by the block device must match the BPB's 
 *      one 
 * - the device size reported by the block device must be <= then BPB's 
 *      one 
 *
 * Note that 8char OEM ID is absolutely irrelevant for FASTFAT - it is 
 * never written and never checked. Contrary to FASTFAT which will 
 * tolerate any value there, NTFS requires the FAT OEM ID to be "NTFS 
 */

int FatValidateBPB (struct FatSB *fsb, uint32 total_sector_cnt)
{
	struct BlockDeviceStat stat;
	

	/* FIXME:  fsb->validated might be uninitialized? */

	
	if (fsb->validated == TRUE && fsb->removable == FALSE)
	{
		KPRINTF ("FatRevalidate() already validated, not removable");
		return 0;
	}
	
	
	if (FatStatDevice (fsb, &stat) == 0)
	{
		if (stat.media_state == MEDIA_INSERTED)
		{
			fsb->total_sectors = stat.total_sectors;
			fsb->write_protect = stat.write_protect;
		}
		else
		{
			fsb->validated = FALSE;
			SetError (ENODEV);
			return -1;
		}
	}
	else
	{
		KPRINTF ("FatRevalidate() unable to stat device");

		fsb->validated = FALSE;
		FatInvalidateCache(fsb);
		SetError (ENODEV);
		return -1;
	}
	

	
	if (ReadBlocks (fsb, &kernel_as, bpb_sector, 0, 0, 512) != 0)
		return -1;
	
	
	MemCpy (&fsb->bpb, bpb_sector, sizeof (struct FatBPB));
	MemCpy (&fsb->bpb16, bpb_sector + BPB_EXT_OFFSET, sizeof (struct FatBPB_16Ext));
	MemCpy (&fsb->bpb32, bpb_sector + BPB_EXT_OFFSET, sizeof (struct FatBPB_32Ext));
	
	
	/* FIXME:  Not sure why this is commented out
	
	if (!(fsb->bpb.jump[0] == 0xeb && fsb->bpb.jump[2] == 0x00) || !(fsb->bpb,jmp[2] == 0x00))
		return -1;
	*/
	
	KPRINTF ("bpb.bytes_per_sector = %d", fsb->bpb.bytes_per_sector);
	
	if (fsb->bpb.bytes_per_sector != 512)        /* Fails here */
		return -1;
	
	if (!	(fsb->bpb.sectors_per_cluster >= 1
			&& fsb->bpb.sectors_per_cluster <= 128
			&& (fsb->bpb.sectors_per_cluster & (fsb->bpb.sectors_per_cluster - 1)) == 0))
		return -1;
	
	if (fsb->bpb.reserved_sectors_cnt == 0)
		return -1;
	
	if (fsb->bpb.fat_cnt == 0)
		return -1;
	
	if (!	(fsb->bpb.media_type == 0
			|| fsb->bpb.media_type == 1
			|| fsb->bpb.media_type >= 0xf0))
		return -1;
	
	if (!	( (fsb->bpb.total_sectors_cnt16 != 0)
			|| (fsb->bpb.total_sectors_cnt16 == 0 && fsb->bpb.total_sectors_cnt32 != 0)))
		return -1;
	
	if (!	((fsb->bpb.sectors_per_fat16 != 0)
			|| (fsb->bpb.sectors_per_fat16 == 0 && fsb->bpb32.sectors_per_fat32 != 0)))
		return -1;
	
	
	FatPrecalculateFSBValues (fsb);
		
	
	if (fsb->fat_type == TYPE_FAT32 && fsb->bpb32.fs_version != 0)
		return -1;
	
	if ((fsb->fat_type == TYPE_FAT12 || fsb->fat_type == TYPE_FAT16) && fsb->bpb.root_entries_cnt == 0)
		return -1;
	
	
	/*
	if (fsb->total_sectors_cnt > total_sector_cnt)
		return -1;
	*/
	
	
	
	fsb->start_search_cluster = 2;
	fsb->fsinfo_valid = FALSE;
	
	if (fsb->fat_type == TYPE_FAT32 && fsb->bpb32.fs_info > 0 && fsb->bpb32.fs_info < fsb->bpb.reserved_sectors_cnt)
	{
		if (ReadBlocks (fsb, &kernel_as, fsinfo_sector, fsb->bpb32.fs_info, 0, 512) == 0)
		{
			MemCpy (&fsb->fsi, fsinfo_sector + 0, sizeof (struct FatFSInfo));
			
			if (fsb->fsi.lead_sig == FSINFO_LEAD_SIG && fsb->fsi.struc_sig == FSINFO_STRUC_SIG
				&& fsb->fsi.trail_sig == FSINFO_TRAIL_SIG)
			{
				fsb->fsinfo_valid = TRUE;
				fsb->start_search_cluster = fsb->fsi.next_free;
			}
		}
	}
	
	
	fsb->validated = TRUE;

		
	KPRINTF ("FatRevalidate SUCCESS");
	
	return 0;
}





/*
 * FatStatDevice();
 */

int FatStatDevice (struct FatSB *fsb, struct BlockDeviceStat *stat)
{
	struct BlkReq blkreq;

	KPRINTF ("FatStatDevice()");

	blkreq.as = &kernel_as;
	blkreq.device = fsb->device;
	blkreq.unitp = fsb->unitp;
	blkreq.stat = stat;
	blkreq.cmd = BLK_CMD_MEDIA_PRESENT;

	DoIO (&blkreq, NULL);
	SetError (0);

	return 0;
	
}




/*
 * FatInvalidate();
 *
 * Invalidate cache and mark all filps as invalidated,  should free
 * nodes or at least put them on an invalidated list as well.
 */

void FatInvalidate (struct FatSB *fsb)
{
	struct FatFilp *filp;

	
	KPRINTF ("**** FATInvalidate()");
	
	fsb->validated = FALSE;
	FatInvalidateCache(fsb);

	
	filp = LIST_HEAD (&fsb->active_filp_list);
	
	while (filp != NULL)
	{
		filp->invalid = 1;
		
		LIST_REM_HEAD (&fsb->active_filp_list, fsb_filp_entry);
		
		LIST_ADD_TAIL (&fsb->invalid_filp_list, filp, fsb_filp_entry);

		filp = LIST_HEAD (&fsb->active_filp_list);
	}
}




