#include <kernel/types.h>
#include <kernel/vm.h>
#include <kernel/utility.h>
#include <kernel/dbg.h>
#include <kernel/config.h>
#include <kernel/proc.h>
#include <kernel/error.h>




/*
 * CopyIn();
 *
 * Copys data from an addresss-pace into the kernel.
 *
 * returns 0 on success, -1 on failure
 */
 
int CopyIn (struct AddressSpace *as, void *dst, const void *src, size_t sz)
{
	struct AddressSpace *old_as;
	int rc;

	
	if (as != &kernel_as)
	{
		if ((vm_addr)src >= VM_USER_BASE && (vm_addr)src+sz >= VM_USER_BASE
			&& (vm_addr)src+sz <= VM_USER_CEILING)
		{
			old_as = SwitchAddressSpace (as);
			rc = DoCopyIn (dst, src, sz);
			SwitchAddressSpace (old_as);
		}
		else
		{
			SetError (EFAULT);
			rc = -1;
		}
	}
	else
	{
		MemCpy (dst, src, sz);
		rc = 0;
	}
	
	return rc;
}




/*
 * CopyOut();
 *
 * Copys data from the kernel to an address-space
 * Returns 0 on success, -1 on failure
 */

int CopyOut (struct AddressSpace *as, void *dst, const void *src, size_t sz)
{
	struct AddressSpace *old_as;
	int rc;
	
	
	if (as != &kernel_as)
	{
		if ((vm_addr)dst >= VM_USER_BASE && (vm_addr)dst+sz >= VM_USER_BASE
			&& (vm_addr)dst+sz <= VM_USER_CEILING)
		{
			old_as = SwitchAddressSpace (as);
			rc = DoCopyOut (dst, src, sz);
			SwitchAddressSpace (old_as);
		}
		else
		{
			SetError (EFAULT);
			rc = -1;
		}
	}
	else
	{
		MemCpy (dst, src, sz);
		rc = 0;
	}


	return rc;
}




/*
 * CopyInStr();
 *
 * Copys a string from an address-space into the kernel.
 * Returns 0 on success, -1 on failure.
 * Sets errno to ENAMETOLONG if input string is longer than buffer.
 * Sets errno to EFAULT if an unresolved page-fault occured.
 */

int CopyInStr (struct AddressSpace *as, char *dst, const char *src, size_t sz)
{
	struct AddressSpace *old_as;
	int rc;
	
	
	if (as != &kernel_as)
	{
		old_as = SwitchAddressSpace (as);
		rc = DoCopyInStr (dst, src, sz);
		SwitchAddressSpace (old_as);
	}
	else
	{
		if (StrLCpy (dst, src, sz) < sz)
		{
			rc = 0;
		}
		else
		{
			rc = -1;
			SetError (ENAMETOOLONG);
		}
	}

	return rc;
}








