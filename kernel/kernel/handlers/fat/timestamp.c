#include <kernel/types.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/lists.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include <kernel/timer.h>
#include "fat.h"




/*
 *
 */
 
void FatToEpochTime (struct TimeVal *tv, uint16 date, uint16 time, uint8 milliten)
{
	struct Tm tm;
	
	tm.sec = ((time & 0x001f)*2)  + (milliten / 100);
	tm.min = (time >> 5) & 0x003f;
	tm.hour = (time >> 11) & 0x001f;
	tm.mday = (date & 0x001f);
	tm.mon = ((date >> 5) & 0x000f) - 1;
	tm.year = ((date >> 9) & 0x007f) + 80;

	tv->seconds = MakeTime (&tm);
	tv->microseconds = (milliten % 100) * 10000;
}




/*
 *
 */

void EpochToFatTime (struct TimeVal *tv, uint16 *date, uint16 *time, uint8 *milliten)
{
	struct Tm tm;
		
	LocalTime (&tm, tv);
	
	*time = ((tm.sec/2) & 0x001f)  | ((tm.min & 0x003f) << 5) | ((tm.hour & 0x001f) << 11);
	*date = (tm.mday & 0x001f) | (((tm.mon+1) & 0x000f) << 5) | (((tm.year-80) & 0x007f) << 9);
	*milliten = ((tm.sec % 2) * 100) + (tv->microseconds / 10000);
}




/*
 *
 */

void FatSetTime (struct FatSB *fsb, struct FatDirEntry *dirent, uint32 update)
{
	struct TimeVal tv;
	uint16 date;
	uint16 time;
	uint8 tenths;
	
	
	if (fsb != NULL && fsb->write_protect == TRUE)
		return;
	
	
	KGetTimeOfDay (&tv);
	EpochToFatTime (&tv, &date, &time, &tenths);
	
	
	/* FIXME: Not really a creation time,  also can avoid flushing dirent
		if not modified. */
			
	if (update & ST_CTIME)
	{
		dirent->creation_time_sec_tenths = tenths;
		dirent->creation_time_2secs = time;
		dirent->creation_date = date;
	}
	
	if (update & ST_ATIME)
	{
		dirent->last_access_date = date;
	}
	
	if (update & ST_MTIME)
	{
		dirent->last_write_time = time;
		dirent->last_write_date = date;
	}
	
	
}

