#include <kernel/types.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/lists.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include "fat.h"




/*
 * Would be nice to make it generic across filesystems, then supply pointers to
 * name comparison/conversion functions.
 */

int FatLookup (struct FatLookup *lookup)
{
	struct FatSB *fsb;
	struct FatNode *parent;
	struct FatNode *node;
	char *component, *first_component, *final_component;
	
	
	fsb = lookup->fsb;
	node = &fsb->root_node;
	
	/*
	 * why incrememting reference_cnt of root_node???????
	 */
	 
	node->reference_cnt ++;


	first_component = lookup->pathname;

	component = first_component;
	final_component = component;
	
	while (*component != '\0')
	{
		final_component = component;
		component = Advance (component);
	}

	component = first_component;


	if (*first_component == '\0')
	{	
		if (lookup->cmd == CMD_LOOKUP)
		{
			/* Check permissions */
			lookup->parent = NULL;
			lookup->node = &fsb->root_node;
			return 0;
		}
		else
			return -1;
	}
	else
	{	
		/* Find directories */
		
		while (component != final_component)
		{		
			parent = node;
			node = FatSearchDir (fsb, node, component);
			FreeNode (fsb, parent);
						
			if (node != NULL)
			{
				if (node->dirent.attributes & ATTR_DIRECTORY)
					component = Advance (component);
				else
				{
					FreeNode (fsb, node);
					SetError (ENOTDIR);
					lookup->parent = NULL;
					lookup->node = NULL;
					return -1;
				}
			}
			else
			{
				SetError (ENOENT);
				lookup->parent = NULL;
				lookup->node = NULL;
				return -1;
			}
		}
		
		
		/* Find final node */
		
		/* At this point node->ref_cnt should be incremented by one */
		
		parent = node;
		node = NULL;
		
		KASSERT (parent != NULL);
		
		
		
		
		node = FatSearchDir (fsb, parent, final_component);
		

		if (lookup->cmd == CMD_LOOKUP)
		{
			FreeNode (fsb, parent);
			lookup->node = node;
			lookup->parent = NULL;
						
			if (node != NULL)
				return 0;
			else
				return -1;
		}
		else if (lookup->cmd == CMD_CREATE)
		{
			lookup->parent = parent;
			lookup->node = node;
			lookup->last_component = final_component;
			return 0;
		}
		else if (lookup->cmd == CMD_DELETE)
		{
			lookup->parent = parent;
			lookup->node = node;
			lookup->last_component = final_component;
			return 0;
		}
		else if (lookup->cmd == CMD_RENAME)
		{
			lookup->parent = parent;
			lookup->node = node;
			lookup->last_component = final_component;
			return 0;
		}
		else
		{
			SetError (EINVAL);
			FreeNode (fsb, parent);
			FreeNode (fsb, node);
			return -1;
		}
	}
	
	return -1;
}




/*
 * FatSearchDir();
 */

struct FatNode *FatSearchDir (struct FatSB *fsb, struct FatNode *dirnode, char *component)
{
	struct FatDirEntry dirent;
	int32 offset;
	struct FatNode *node;
	uint32 dirent_sector, dirent_offset;
	int32 rc;
	
		
	offset = 0;
	
	do
	{
		rc = FatDirRead (dirnode, &dirent, offset, &dirent_sector, &dirent_offset);
				
		if (rc == 1)
		{
			if (dirent.name[0] == DIRENTRY_FREE)
			{
				return NULL;
			}
			
			if (dirent.name[0] != DIRENTRY_DELETED)	
			{
				if (CompareDirEntry (&dirent, component) == TRUE)
				{
					if ((node = FindNode (fsb, dirent_sector, dirent_offset)) == NULL)
					{
						node = AllocNode (fsb, &dirent, dirent_sector, dirent_offset);
					}
					
					return node;
				}
			}
						
			offset ++;
		}
		
	} while (rc == 1);
	
	return NULL;	
}




/*
 *
 */

int CalcDirentLocation (struct FatNode *node, off_t offset,
							int32 *node_sector, int32 *node_offset)
{
	struct FatSB *fsb;
	uint32 cluster;
	uint32 sector_in_cluster;
	uint32 cluster_offset;
	uint32 base_sector;
	
	fsb = node->fsb;
		
	if (node == &fsb->root_node && (fsb->fat_type == TYPE_FAT12 || fsb->fat_type == TYPE_FAT16))
	{
		*node_sector = fsb->bpb.reserved_sectors_cnt +
			(fsb->bpb.fat_cnt * fsb->sectors_per_fat) + (offset/512);
		*node_offset = offset % 512;
		
		return 0;
	}
	else if (FindCluster (fsb, node, offset, &cluster) == 0)
	{
		base_sector = ClusterToSector(fsb, cluster);
		
		cluster_offset = offset % (fsb->bpb.sectors_per_cluster * 512);
		sector_in_cluster = cluster_offset / 512;
			
		*node_sector = base_sector + sector_in_cluster;
		*node_offset = offset % 512;
		
		return 0;
	}
	
	return -1;
}




/*
 * CompareDirEntry()
 *
 * Comparison can be simplified, shouldn't need len. (old?)
 */
 
bool CompareDirEntry (struct FatDirEntry *dirent, char *comp)
{
	int32 len, i;
	char *comp_p, *dirent_p;
	char packed_dirent_name[12];
	bool match = FALSE;
	

	if (dirent->name[0] != DIRENTRY_FREE && (uint8)dirent->name[0] != DIRENTRY_DELETED
			&& (dirent->attributes & ATTR_LONG_FILENAME) != ATTR_LONG_FILENAME)
	{
		if (FatIsDosName (comp) == 0)
		{
			len = FatDirEntryToASCIIZ (packed_dirent_name, dirent);
			
			if (len == 0)
				return FALSE;

			packed_dirent_name[11] = '\0';
			
			comp_p = comp;
			dirent_p = packed_dirent_name;
			match = TRUE;
			
			for (i=0; i<len; i++)
			{
				if (*comp_p != *dirent_p)
				{
					match = FALSE;
					break;
				}
				
				comp_p++;
				dirent_p++;
			}
			
			if (*comp_p != '\0' && *comp_p != '/')
				match = FALSE;
		}
		else
			match = FALSE;
	}

	return match;
}




/*
 * DirEntryToASCIIZ();
 *
 */

int FatDirEntryToASCIIZ (char *pathbuf, struct FatDirEntry *dirent)
{
	char *p = pathbuf;
	int i;
	int len = 0;
		
	*pathbuf = 0;
	
	len = 0;
	
	for (i=0; i<8; i++)
	{
		if (dirent->name[i] != ' ')
		{
			*p++ = dirent->name[i];
			len++;
		}
		else
			break;
	}
			
	if (dirent->extension[0] != ' ')
	{
		*p++ = '.';
		len ++;
	}
		
	for (i=0; i<3; i++)
	{
		if (dirent->extension[i] != ' ')
		{
			*p++ = dirent->extension[i];
			len ++;
		}
		else
			break;
	}
	
	*p = 0;
	
	return len;
}




/*
 *
 */

int FatASCIIZToDirEntry (struct FatDirEntry *dirent, char *filename)
{
	char *p = filename;
	int i;

	if (FatIsDosName (filename) == 0)
	{
		for (i=0; i<8; i++)
		{
			if (*p == '\0' || *p =='.')
				dirent->name[i] = ' ';
			else
				dirent->name[i] = *p++;
		}
			
		
		if (*p == '.')
		{
			p++;
					
			for (i=0; i<3; i++)
			{
				if (*p == '\0')
					dirent->extension[i] = ' ';
				else
					dirent->extension[i] = *p++;
			}
		}
		else
		{
			for (i=0; i<3; i++)
				dirent->extension[i] = ' ';
		}
	
		return 0;
	}

	SetError(ENOENT);
	return -1;
}




/*
 * FatIsDosName();
 *
 * Checks that component is a valid DOS filename
 *
 */

int FatIsDosName (char *s)
{
	int32 name_cnt = 0;
	int32 extension_cnt = 0;
	int dot_cnt = 0;

	while (*s != '\0' && *s != '/')
	{
		/* Add support for special Japanese Kanji character 0x05 0xe5 ?? */
		
		if (*s == '.')
		{
			dot_cnt++;
			
			if (dot_cnt > 1)
				return -1;
			
			s++;
			continue;
		}
		else if (*s >= 'a' && *s <= 'z')
		{
			*s -= 'a' - 'A';
		}
		else if (! ((*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9') || (*s < 0)))
		{
			switch (*s)
			{
				case '$':
				case '%':
				case '\'':
				case '-':
				case '_':
				case '@':
				case '~':
				case '`':
				case '!':
				case '(':
				case ')':
				case '{':
				case '}':
				case '^':
				case '#':
				case '&':
				case ' ':
					break;
				default:
					return -1;
			}
		}
		
		
		if (dot_cnt == 0)
			name_cnt++;
		else if (dot_cnt == 1)
			extension_cnt++;
		else
		{
			return -1;
		}
		
		if (name_cnt == 0 && dot_cnt >= 1)
		{
			return -1;
		}
		
		if (name_cnt > 8 || dot_cnt > 1 || extension_cnt > 3)
		{
			return -1;
		}
			
		s++;
	}

	return 0;		
}

