#include <kernel/types.h>
#include <kernel/vm.h>
#include <kernel/proc.h>
#include <kernel/fs.h>
#include <kernel/dbg.h>
#include <kernel/i386/i386.h>
#include <kernel/i386/multiboot.h>
#include <kernel/i386/init.h>
#include <kernel/utility.h>
#include <kernel/i386/elf.h>
#include "acpi.h"




/*
 *
 */

void InitACPI (void)
{
	KPRINTF ("** AcpiInitializeTables()");
	
	if (AcpiInitializeTables(NULL, 0, FALSE) == AE_OK)
	{
		KPRINTF ("** AcpiInitializeSubsystem()");
		
		if (AcpiInitializeSubsystem() == AE_OK)
		{
			KPRINTF ("** AcpiLoadTables()");
			
			if (AcpiLoadTables() == AE_OK)
			{
				KPRINTF ("** AcpiEnableSubsystem()");
				
				if (AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION) == AE_OK)
				{
					KPRINTF ("** AcpiEnable()");
					
					if (AcpiEnable() == AE_OK)
					{
						acpi_is_enabled = TRUE;
					}
				}
			}
		}
	}
}




/*
 * Shutdown PC.
 *
 * FIXME:  Need to handle reboot options.
 */

void FiniACPI (void)
{
	if (acpi_is_enabled == TRUE)
	{
		AcpiEnterSleepStatePrep(ACPI_STATE_S5);
		AcpiEnterSleepState(ACPI_STATE_S5);
	}
	else
	{
		KPRINTF ("It is now safe to switch off the computer.");
	}
}


