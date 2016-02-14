#include <kernel/types.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/lists.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include "iso9660.h"




/*
 * Think new layout of code is much clearer
 */

int CDLookup (struct CDLookup *lookup)
{
	struct CDSB *cdsb;
	struct CDNode *parent;
	struct CDNode *node;
	char *component, *first_component, *final_component;
	
		
	cdsb = lookup->cdsb;
	node = &cdsb->root_node;
	
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
		lookup->parent = NULL;
		lookup->node = &cdsb->root_node;
		return 0;
	}
	else
	{	
		/* Find directories */
		
		while (component != final_component)
		{		
			parent = node;
			node = CDSearchDir (cdsb, node, component);
			CDFreeNode (cdsb, parent);
						
			if (node != NULL)
			{
				if (node->flags & ISO_DIRECTORY)
					component = Advance (component);
				else
				{
					CDFreeNode (cdsb, node);
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
		
		
		
		
		node = CDSearchDir (cdsb, parent, final_component);
		
		CDFreeNode (cdsb, parent);
		lookup->node = node;
		lookup->parent = NULL;
						
		if (node != NULL)
			return 0;
		else
			return -1;
	}
	
	return -1;
}




/*
 * CDSearchDir();
 */

struct CDNode *CDSearchDir (struct CDSB *cdsb, struct CDNode *dirnode, char *component)
{
	struct Dirent dirent;
	struct ISODirEntry idir;
	struct CDNode *node;
	off_t offset, new_offset;
	int32 rc;
	
			
	offset = 0;
	
	do
	{
		rc = CDDirRead (dirnode, &dirent, &idir, offset, &new_offset);
	
			
		if (rc == 1)
		{
		 	if (CDCompareDirEntry (&dirent, component) == TRUE)
			{
				if ((node = CDFindNode (cdsb, &idir)) == NULL)
				{
					node = CDAllocNode (cdsb, &idir);
				}
				
				return node;
			}
			
			offset = new_offset;
		}
		
	} while (rc == 1);
	
	return NULL;	
}




/*
 * CDCompareDirEntry()
 *
 * Comparison can be simplified, shouldn't need len. (old?)
 */
 
bool CDCompareDirEntry (struct Dirent *dirent, char *component)
{
	int32 len, i;
	char *comp_p, *dirent_p;
	
		
	len = StrLen (dirent->d_name);
		
	if (len == 0)
	{
		return FALSE;
	}

	comp_p = component;
	dirent_p = dirent->d_name;
		
	for (i=0; i<len; i++)
	{
		if (*comp_p != *dirent_p)
		{
			return FALSE;
			break;
		}
		
		comp_p++;
		dirent_p++;
	}
		
	if (*comp_p != '\0' && *comp_p != '/')
		return FALSE;
	else
		return TRUE;
}



