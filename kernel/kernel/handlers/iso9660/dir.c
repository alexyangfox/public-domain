#include <kernel/types.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/lists.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include <kernel/buffers.h>
#include "iso9660.h"




/* FIX:  returns 1, 0 and -1  confusing
 * 
 * OR make it call CDFileRead
 *
 * ISODirents do not cross sector boundaries.
 */

int CDDirRead (struct CDNode *node, struct Dirent *dirent, struct ISODirEntry *idir, off_t offset, off_t *new_offset)
{
	struct CDSB *cdsb;
	uint32 sector;
	uint32 sector_offset;
	struct Blk *blk;
	struct ISODirEntry *dir;
	int dir_length;
	int name_len;
	
	
	
	cdsb = node->cdsb;
		

	while (1)
	{
		sector = node->extent_start + offset / 2048;
		sector_offset = offset % 2048;

		if (offset >= node->size)
		{
			return 0;
		}
	
		if ((blk = BufGetBlock(cdsb->buf, sector)) != NULL)
		{
			dir = (struct ISODirEntry *)((uint8 *)blk->mem + sector_offset);
		
			dir_length = Iso711 (dir->length);
		
			if (dir_length == 0)
			{
				offset += (2048 - sector_offset);
				continue;
			}
			else if ((sector_offset + dir_length) > 2048)
			{
				return -1;
			}
			
			if (dir->flags[0] & ISO_EXISTENCE)
			{
				offset += dir_length;
				continue;		
			}
			
			if ((dir->name[0] == 0x00 || dir->name[0] == 0x01) && Iso711(dir->name_len) == 1)
			{
				offset += dir_length;
				continue;
			}
			
			
			
			
			*new_offset = offset + dir_length;	
			
			
			if (idir != NULL)
			{
				MemCpy (idir, dir, sizeof (struct ISODirEntry));
			}
			
			if (dirent != NULL)
			{
				name_len = Iso711 (dir->name_len);
	
				dirent->d_ino = Iso733(dir->extent);
			
				if (ISODirEntryToASCIIZ (dirent->d_name, (char*) dir->name, name_len) == 0)
					return 1;
				else
					return -1;
				
			}
			
			return 1;
		}
		else
		{
			return -1;
		}
	}
}










/*
 *
 */

int ISODirEntryToASCIIZ (char *dst, char *src, int max_len)
{
	char name[33];
	char *filename = NULL;
	char *extension = NULL;
	char *version = NULL;
	char *c;
	
	if (max_len > 32)
		return -1;
	
	MemCpy (name, src, max_len);
	name[max_len] = '\0';
	
	StrLCpy (dst, name, NAME_MAX);
	
		
	
	/* null terminate filename, extension , version */

	filename = &name[0];
	
		
	/* Find last ";" */
	
	c = name + StrLen (name);
		
	while (c > name)
	{
		if (*c == ';')
		{
			version = c + 1;
			*c = '\0';
			break;			
		}
		
		c--;	
	}
	
	
	/* Find last "." before ";" */

	while (c > name)
	{
		if (*c == '.')
		{
			extension = c + 1;
			*c = '\0';
			break;			
		}
		
		c--;	
	}
	
	
	
	StrLCpy (dst, filename, NAME_MAX);
		
	if (extension != NULL && StrLen (extension) > 0)
	{
		StrLCat (dst, ".", NAME_MAX);
		StrLCat (dst, extension, NAME_MAX);
	}
	
	if (version != NULL && StrLen (version) > 0)
	{
		if (AtoI (version) != 1)
		{
			StrLCat (dst, ";", NAME_MAX);
			StrLCat (dst, version, NAME_MAX);
		}
	}
	
	
	
	c = dst;
		
	while (*c != '\0')
	{
		if (*c >= 'A' && *c <= 'Z')
			*c += ('a' - 'A');
		
		c++;
	}
	
		
	return 0;
}



