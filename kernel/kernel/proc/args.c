#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/utility.h>
#include <kernel/kmalloc.h>
#include <kernel/dbg.h>




/*
 * static prototypes
 */

static char **ReadPointerArray (char **array, struct ArgInfo *ai);
static char *ReadArgString (char *string, struct ArgInfo *ai);




/*
 * CreateArgEnv();
 */
 
int CreateArgEnv (struct ArgInfo *ai, char **argv, char **env)
{
	void *buf;
	char **kargv, **kenv;
	char **targv, **tenv;
	
	
	KPRINTF ("CreateArgEnv()");
	
	if ((buf = (void *)KMap (ARG_MAX, VM_PROT_READWRITE)) != (void *)MAP_FAILED)
	{
		ai->buf = buf;
		ai->nbytes_free = ARG_MAX;
		ai->current_pos = buf;
		ai->argc = 0;
		ai->envc = 0;
		
		kargv = NULL;
		kenv = NULL;	
		
		if (argv != NULL)
			if ((kargv = ReadPointerArray (argv, ai)) == NULL)
				goto ErrorExit;
		
		if (env != NULL)
			if ((kenv = ReadPointerArray (env, ai)) == NULL)
				goto ErrorExit;
	
		targv = kargv;
		tenv = kenv;
		
		if (targv != NULL)
		{
			while (*targv != NULL)
			{
				if ((*targv = ReadArgString (*targv, ai)) == NULL)
					goto ErrorExit;
				targv++;
				
				ai->argc ++;
			}
		}
		
		if (tenv != NULL)
		{
			while (*tenv != NULL)
			{
				if ((*tenv = ReadArgString (*tenv, ai)) == NULL)
					goto ErrorExit;
				tenv++;

				ai->envc ++;
			}
		}	
		
		ai->argv = kargv;
		ai->env  = kenv;
		
		return 0;
		
		ErrorExit:
		KUnmap ((vm_addr)buf);
	}
	
	
	KPANIC ("CreateArgEnv FAILED");
	
	return -1;
}




/*
 * FreeArgEnv();
 */
 
void FreeArgEnv (struct ArgInfo *ai)
{
	KPRINTF ("FreeArgs()");

	KUnmap ((vm_addr)ai->buf);
}




/*
 * CopyArgEnvToUserSpace();
 */

int CopyArgEnvToUserSpace (struct ArgInfo *ai, struct AddressSpace *new_as)
{
	vm_addr ubase;
	char **argv, **env;
	
	KPRINTF ("CopyArgsToUserSpace()");
	
	if ((ubase = UMap(0, ARG_MAX ,VM_PROT_READWRITE, 0)) != MAP_FAILED)
	{
		argv = ai->argv;
		env = ai->env;

		if (argv != NULL)
		{
			while (*argv != NULL)
			{
				*argv = (char *)ubase + ((char *)*argv - (char *)ai->buf);
				argv++;
			}
		}

		if (env != NULL)
		{
			while (*env != NULL)
			{
				*env = (char *)ubase + ((char *)*env - (char *)ai->buf);
				env++;
			}
		}

	
		
		ai->ubase = ubase;
		ai->uargv = (char **)((uint8 *)ubase + ((uint8 *)ai->argv - (uint8 *)ai->buf));
		ai->uenv = (char **)((uint8 *)ubase + ((uint8 *)ai->env - (uint8 *)ai->buf));
		
		KPRINTF ("CopyOut() CopyArgsToUserSpace");
		if (CopyOut (new_as, (void *)ubase, ai->buf, ARG_MAX) == 0)
		{
			return 0;
		}

		UUnmap (ubase);
	}
	
	return -1;
}




/*
 * ReadPointerArray();
 */
 
static char **ReadPointerArray (char **src_array, struct ArgInfo *ai)
{
	char **dst_array;
	char **src_entry, **dst_entry;
	char *str;
	
	dst_array = ai->current_pos;
	src_entry = src_array;
	dst_entry = dst_array;
	
	do
	{
		if (ai->nbytes_free >= 4)
		{
			if (CopyIn (current_process->user_as, dst_entry, src_entry, 4) != 0)
				return NULL;

			str = *dst_entry;
			src_entry++;
			dst_entry++;
			
			ai->current_pos += 4;
			ai->nbytes_free -= 4;
		}
		else
			return NULL;
	}
	while (str != NULL);
	
	return dst_array;
}




/*
 * ReadArgString();
 */

static char *ReadArgString (char *src, struct ArgInfo *ai)
{
	int32 len;
	char *old_pos;
	
	if (CopyInStr (current_process->user_as, ai->current_pos, src, ai->nbytes_free) == 0)
	{
		len = StrLen (ai->current_pos) + 1;
		
		old_pos = ai->current_pos;
		ai->current_pos += len;
		ai->nbytes_free -= len;
		return old_pos;
	}
	
	return NULL;
}




