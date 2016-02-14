#include <kernel/types.h>
#include <kernel/fs.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/dbg.h>
#include <kernel/proc.h>
#include <kernel/error.h>






/*
 * CreatePathInfo (&pi, pathname, keep_canon);
 */
 
int CreatePathInfo (struct PathInfo *pi, char *pathname, bool keep_canon)
{
	pi->buffer = NULL;
	pi->canon = NULL;


	if ((pi->canon = CanonPathname(pathname)) != NULL)
	{
		if ((pi->buffer = TranslatePathnameAliases (pi->canon)) != NULL)
		{
			if (InitPathInfo (pi) == 0)
			{
				KPRINTF ("pi->canon = %s", pi->canon);
			
				if (keep_canon != KEEP_CANON)
				{
					KFree (pi->canon);
					pi->canon = NULL;
				}
	
				return 0;
			}
			
			KFree (pi->buffer);	
		}
				
		KFree (pi->canon);
	}
	
	SetError (ENOENT);
	return -1;
}




/*
 * FreePathInfo();
 */

void FreePathInfo (struct PathInfo *pi)
{
	if (pi->canon != NULL)
		KFree (pi->canon);
	
	KFree (pi->buffer);
}




/*
 * InitPathInfo();
 *
 */
 
int32 InitPathInfo (struct PathInfo *pi)
{
	char *c;

	c = pi->buffer;
	
	if (*pi->buffer == '/' && *(pi->buffer+1) == '\0')
	{
		pi->mountname = pi->buffer + 1;
		pi->pathname = pi->buffer + 1;
		return 0;
	}
	
	
	/* Find mountname,  doesn't null terminate it */
	
	while (*c == '/')
	{
		if (*c == '\0')
		{
			/* Path is of the form "/", no mountname */
			SetError(ENOENT);
			return -1;
		}
		
		c++;
	}
	
	pi->mountname = c;
	
			
	while (*c != '\0')
	{
		if (*c == '/')
		{
			/* Path is of the form "/fd0/ */
			
			*c = '\0';
			c++;
			pi->pathname = c;
			return 0;
		}
		else if (!((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9')))
		{
			SetError(ENOENT);
			return -1;
		}
		
		c++;
	}
	
	
	/* Path is of the form "/fd0" */
	
	pi->pathname = c;
	return 0;
}





/*
 * CanonPathname();
 *
 * Converts pathname to a canonical pathname, appending the current directory
 * if necessary.  pathname is help in a 'temp' buffer and components are
 * processed one at a time and copied to the resulting 'canon' buffer.
 * 
 * Source pathname "/sys/bin/../home/./zing" becomes "/sys/home/zing" in the
 * 'canon' buffer.
 */

char *CanonPathname (char *pathname)
{
	char *temp;
	char *canon = NULL;
	char *src;
	int error = 0;
	
	
	if ((temp = KMalloc (MAX_PATHNAME_SZ)) != NULL)
	{
		if (CopyInStr (current_process->user_as, temp, pathname, MAX_PATHNAME_SZ) == 0)
		{
			if ((canon = KMalloc (MAX_PATHNAME_SZ)) != NULL)
			{	
				*canon = '\0';
							
				if (*temp != '/')
				{
					StrLCpy (canon, current_process->current_dir, MAX_PATHNAME_SZ);
					src = temp;
				}
				else
				{
					src = Advance (temp);
				}
				
								
				while (*src != '\0')
				{
					if (CompareComponent("..", src) == 0)
					{
						if (DeleteLastComponent(canon) != 0)
						{
							SetError(ENOENT);
							error = -1;
							break;
						}
					}
					else if (CompareComponent(".", src) != 0)
					{
						if (AppendComponent (canon, src) != 0)
						{
							SetError(ENAMETOOLONG);
							error = -1;
							break;
						}
					}
					
					src = Advance (src);
				}
				
				if (error == 0)
				{
					if (*canon == '\0')
						StrLCpy (canon, "/", MAX_PATHNAME_SZ);
				}
				else
				{
					KFree (canon);
				}
			}
			else
				error = -1;
		}
		else
			error = -1;
		
		
		KFree (temp);
	}
	
	
	if (error == -1)
		return NULL;
	else
		return canon;
}




/*
 * DeleteLastComponent();
 *
 * Find last '/' seperator and replaces it with '\0' thereby
 * removing the last component from the 'canon' buffer.
 */

int DeleteLastComponent (char *canon)
{
	char *c;
	char *last_slash = NULL;
	
	/* Really need FindLastComponent(), then find slash before it */
	
	for (c=canon; *c != '\0'; c++)
		if (*c == '/')
			last_slash = c;
	
	if (last_slash != NULL)
	{
		*last_slash = '\0';
		return 0;
	}
	else
		return -1;
}




/*
 * AppendComponent();
 *
 * Add '/' followed by component and '\0' terminator
 * Check bounds.
 *
 * Assumes canon buffer size equals MAX_PATHNAME_SZ
 */

int AppendComponent (char *canon, char *component)
{
	char *c;
	char *src;
	
	
	for (c = canon; *c != '\0'; c++);
	
	if ((c - canon) < MAX_PATHNAME_SZ)
		*c++ = '/';
	else
		return -1;
	
	for (src = component; *src != '/' && *src != '\0'; src++, c++)
	{
		if ((c - canon) < MAX_PATHNAME_SZ)
			*c = *src;
		else
			return -1;
	}
	
	if ((c - canon) < MAX_PATHNAME_SZ)
	{
		*c++ = '\0';
		return 0;
	}
	else
		return -1;
		
}




/*
 *
 */

int AppendPath (char *buf, char *remaining)
{
	char *c;
	char *src;
	
	
	for (c = buf; *c != '\0'; c++);
	
		if ((c - buf) < MAX_PATHNAME_SZ)
		*c++ = '/';
	else
		return -1;
	
	for (src = remaining; *src != '\0'; src++, c++)
	{
		if ((c - buf) < MAX_PATHNAME_SZ)
			*c = *src;
		else
			return -1;
	}
	
	if ((c - buf) < MAX_PATHNAME_SZ)
	{
		*c++ = '\0';
		return 0;
	}
	else
		return -1;
}



/*
 * Advance();
 *
 * Can be used within filesystems to parse a pathname.
 */

char *Advance (char *component)
{
	while (*component != '/' && *component != '\0')
		component++;
		
	if (*component == '\0')
		return component;
	
	while (*component == '/')
		component++;

	return component;
}




/*
 * CompareComponent (a, b)
 */

int CompareComponent (char *s, char *t)
{
	while (*s == *t  || (*s == '\0' && *t == '/') || (*s == '/' && *t == '\0'))
	{
		if (*s == '\0' || *s == '/')
		{
			return 0;
		}
		
		 s++;
		 t++;
	}
	
	return *s - *t;
}










/*
 * AllocAssign();
 */

struct Mount *AllocAssign (char *alias, char *path)
{
	struct Mount *assign;
	int alias_len;


	alias_len = StrLen (alias) + 1;
	
	if ((assign = KMalloc (sizeof (struct Mount))) != NULL)
	{
		if ((assign->name = KMalloc (alias_len)) != NULL)
		{
			StrLCpy (assign->name, alias, alias_len);
			
			if (ValidAlias (assign->name) == 0)
			{
				if ((assign->pathname = TranslatePathnameAliases (path)) != NULL)
				{
					assign->type = MOUNT_ASSIGN;

					AddMount (assign);
						
					return assign;
				}
			}
			
			KFree (assign->name);
		}

		KFree (assign);
	}
	
	
	return NULL;
}




/*
 * FreeAssign();
 */

void FreeAssign (struct Mount *assign)
{
	RemMount (assign);
	KFree (assign->name);
	KFree (assign->pathname);
	KFree (assign);
}




/*
 * AllocPathname();
 */

char *AllocPathname (char *src)
{
	char *buf;
	
	if ((buf = KMalloc (MAX_PATHNAME_SZ)) != NULL)
	{
		if (CopyInStr (current_process->user_as, buf, src, MAX_PATHNAME_SZ) == 0)
			return buf;
		else
			KFree (buf);
	}
	
	return NULL;
}




/*
 * ValidAlias();
 */

int ValidAlias (char *alias)
{
	while (*alias != '\0')
	{
		if (!((*alias >= 'a' && *alias <= 'z') || (*alias >= 'A' && *alias <= 'Z')
			|| (*alias >= '0' && *alias <= '9')))
		{
			return -1;
		}
		
		alias++;
	}
	
	return 0;
}




/*
 * TranslatePathnameAliases();
 */

char *TranslatePathnameAliases (char *canon)
{
	char *pathname;
	char *path;
	struct Mount *assign;
	
		
	if ((pathname = KMalloc (MAX_PATHNAME_SZ)) != NULL)
	{
		canon = Advance (canon);
		assign = FindAssign (canon);
		
		if (assign != NULL && assign->type == MOUNT_ASSIGN)
		{
			StrLCpy (pathname, "/", MAX_PATHNAME_SZ);
			StrLCpy (pathname, assign->pathname, MAX_PATHNAME_SZ);
		
			canon = Advance (canon);

			if (*canon != '\0')
			{
				StrLCat (pathname, "/", MAX_PATHNAME_SZ);
				StrLCat (pathname, canon, MAX_PATHNAME_SZ);
			}
		}
		else
		{
			StrLCpy (pathname, canon, MAX_PATHNAME_SZ);
		}
	}
	
		
	return pathname;
}




/*
 * Find matching Assign()
 *
 * Needs to skip to first component of "alias",  can't use strcmp
 * as may not be null terminated.
 *
 * The canon is before the pathinfo structure is initialised.
 */

struct Mount *FindAssign (char *alias)
{
	struct Mount *assign;
	char *alias_end;
	
	
	alias_end = alias;
	
	while (*alias_end != '\0' && *alias_end != '/')
		alias_end++;
	
	if (*alias_end == '/')
		*alias_end = '\0';
	else
		alias_end = NULL;
	
	
	if (alias != NULL)
	{	
		assign = LIST_HEAD (&mount_list);
		
		while (assign != NULL)
		{
			if (assign->type == MOUNT_ASSIGN)
			{
				if (StrCmp (alias, assign->name) == 0)
				{
					if (alias_end != NULL)
						*alias_end = '/';
						
					return assign;
				}
			}
			
			assign = LIST_NEXT (assign, mount_list_entry);
		}
	}

	if (alias_end != NULL)
		*alias_end = '/';

	return NULL;
}

