#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/fs.h>
#include <kernel/device.h>
#include <kernel/lists.h>
#include <kernel/dbg.h>
#include <kernel/vm.h>
#include <kernel/resident.h>
#include <kernel/i386/multiboot.h>



/*
 * InitResidentDevices();
 */

void InitResidentDevices (void)
{
	vm_addr addr;
	struct Resident *resident, *r;
	
		
	KPRINTF ("InitResidentDevices()");
		
	addr = (vm_addr)&_sdata;

	while (addr < (vm_addr)&_edata)
	{
		resident = (struct Resident *)addr;
						
		if (resident->magic1 == RESIDENT_MAGIC1 && resident->magic2 == RESIDENT_MAGIC2 &&
			resident->magic3 == RESIDENT_MAGIC3 && resident->magic4 == RESIDENT_MAGIC4 &&
			resident->self == resident)
		{
			r = LIST_HEAD(&resident_list);
			
			while (r != NULL)
			{
				if (resident->priority > r->priority)
					break;
			
				r = LIST_NEXT (r, resident_entry);
			}
			
			
			if (r == NULL)
			{
				LIST_ADD_TAIL (&resident_list, resident, resident_entry);
			}
			else
			{
				LIST_INSERT_BEFORE (&resident_list, r, resident, resident_entry);
			}
		}
	
		addr++;
	}
	
	
	

	r = LIST_HEAD(&resident_list);
	
	while (r != NULL)
	{
		switch (r->type)
		{
			case RTYPE_DEVICE:
			
				if (r->flags & RFLG_AUTOINIT)
					r->init(NULL);

				break;

			default:
				if (r->flags & RFLG_AUTOINIT)
					r->init(NULL);
					
				break;
		}
		
		r = LIST_NEXT (r, resident_entry);
	}
}




/*
 *
 */

void FiniResidentDevices (void)
{
	struct Device *device, *next;
	
	
	KPRINTF ("FiniResidentDevices()");
	
	device = LIST_HEAD (&device_list);
	
	while (device != NULL)
	{
		next = LIST_NEXT(device, device_entry);
		device->expunge();
		device = next;
	}
}

