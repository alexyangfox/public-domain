/******************************************************************************
 *
 * Module Name: oskielderxf - KielderOS OSL interfaces
 *
 *****************************************************************************/


#include <kernel/i386/i386.h>
#include <kernel/proc.h>
#include <kernel/vm.h>
#include <kernel/kmalloc.h>
#include <kernel/sync.h>
#include <kernel/pci.h>
#include <kernel/dbg.h>
#include <kernel/error.h>
#include <kernel/timer.h>
#include "acpi.h"
#include "amlcode.h"
#include "acparser.h"
#include "acdebug.h"


int32 AcpiOsInterruptHandlerStub (int32 irq_idx, void *arg);



#define _COMPONENT          ACPI_OS_SERVICES
        ACPI_MODULE_NAME    ("oskielderxf")



static ACPI_OSD_HANDLER kacpi_isr_routine = NULL;
static void *kacpi_isr_context = NULL;
static int kacpi_isr_number = 0;
static struct ISRHandler *kacpi_isr_handler;




struct ACPISemaphore
{
	struct Mutex mutex;
	struct Cond cond;
	uint32 max_units;
	uint32 current_units;
};



uint8 PciReadByte (int busno, int devno, int funcno, int addr);
uint16 PciReadWord (int busno, int devno, int funcno, int addr);
uint32 PciReadLong (int busno, int devno, int funcno, int addr);
void PciWriteByte (int busno, int devno, int funcno, int addr, uint8 value);
void PciWriteWord (int busno, int devno, int funcno, int addr, uint16 value);
void PciWriteLong (int busno, int devno, int funcno, int addr, uint32 value);








uint8 PciReadByte (int busno, int devno, int funcno, int addr)
{
  OutLong (PCI_CONFIG_ADDRESS, ((unsigned long) 0x80000000 | (busno << 16) | (devno << 11) | (funcno << 8) | addr));
  return InByte (PCI_CONFIG_DATA);
}

uint16 PciReadWord (int busno, int devno, int funcno, int addr)
{
  OutLong (PCI_CONFIG_ADDRESS, ((unsigned long) 0x80000000 | (busno << 16) | (devno << 11) | (funcno << 8) | addr));
  return InWord (PCI_CONFIG_DATA);
}

uint32 PciReadLong (int busno, int devno, int funcno, int addr)
{
  OutLong (PCI_CONFIG_ADDRESS, ((unsigned long) 0x80000000 | (busno << 16) | (devno << 11) | (funcno << 8) | addr));
  return InLong (PCI_CONFIG_DATA);
}




void PciWriteByte (int busno, int devno, int funcno, int addr, uint8 value)
{
  OutLong (PCI_CONFIG_ADDRESS, ((unsigned long) 0x80000000 | (busno << 16) | (devno << 11) | (funcno << 8) | addr));
  OutByte (PCI_CONFIG_DATA, value);
}

void PciWriteWord (int busno, int devno, int funcno, int addr, uint16 value)
{
  OutLong (PCI_CONFIG_ADDRESS, ((unsigned long) 0x80000000 | (busno << 16) | (devno << 11) | (funcno << 8) | addr));
  OutWord (PCI_CONFIG_DATA, value);
}

void PciWriteLong (int busno, int devno, int funcno, int addr, uint32 value)
{
  OutLong (PCI_CONFIG_ADDRESS, ((unsigned long) 0x80000000 | (busno << 16) | (devno << 11) | (funcno << 8) | addr));
  OutLong (PCI_CONFIG_DATA, value);
}




















/******************************************************************************
 *
 * FUNCTION:    AcpiOsInitialize, AcpiOsTerminate
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Init and terminate.  Nothing to do.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsInitialize (void)
{
	KPRINTF ("* AcpiOsInitialize()");
    return AE_OK;
}


ACPI_STATUS
AcpiOsTerminate (void)
{
	KPRINTF ("* AcpiOsTerminate()");
    return AE_OK;
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetRootPointer
 *
 * PARAMETERS:  None
 *
 * RETURN:      RSDP physical address
 *
 * DESCRIPTION: Gets the root pointer (RSDP)
 *
 *****************************************************************************/

ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer (
    void)
{
	ACPI_PHYSICAL_ADDRESS table_address = 0;

	AcpiFindRootPointer(&table_address);
	
    return table_address;
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsPredefinedOverride
 *
 * PARAMETERS:  InitVal     - Initial value of the predefined object
 *              NewVal      - The new value for the object
 *
 * RETURN:      Status, pointer to value.  Null pointer returned if not
 *              overriding.
 *
 * DESCRIPTION: Allow the OS to override predefined names
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsPredefinedOverride (
    const ACPI_PREDEFINED_NAMES *InitVal,
    ACPI_STRING                 *NewVal)
{
	KPRINTF ("AcpiOsPredefinedOverride()");

 	if (!InitVal || !NewVal)
		return AE_BAD_PARAMETER;

	*NewVal = NULL;
	return AE_OK;
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsTableOverride
 *
 * PARAMETERS:  ExistingTable   - Header of current table (probably firmware)
 *              NewTable        - Where an entire new table is returned.
 *
 * RETURN:      Status, pointer to new table.  Null pointer returned if no
 *              table is available to override
 *
 * DESCRIPTION: Return a different version of a table if one is available
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_TABLE_HEADER       **NewTable)
{
	KPRINTF ("AcpiOsTableOverride()");
	
	*NewTable = NULL;
    return (AE_OK);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadable
 *
 * PARAMETERS:  Pointer             - Area to be verified
 *              Length              - Size of area
 *
 * RETURN:      TRUE if readable for entire length
 *
 * DESCRIPTION: Verify that a pointer is valid for reading
 *
 *****************************************************************************/

BOOLEAN
AcpiOsReadable (
    void                    *Pointer,
    ACPI_SIZE               Length)
{
    return (TRUE);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsWritable
 *
 * PARAMETERS:  Pointer             - Area to be verified
 *              Length              - Size of area
 *
 * RETURN:      TRUE if writable for entire length
 *
 * DESCRIPTION: Verify that a pointer is valid for writing
 *
 *****************************************************************************/

BOOLEAN
AcpiOsWritable (
    void                    *Pointer,
    ACPI_SIZE               Length)
{
    return (TRUE);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsRedirectOutput
 *
 * PARAMETERS:  Destination         - An open file handle/pointer
 *
 * RETURN:      None
 *
 * DESCRIPTION: Causes redirect of AcpiOsPrintf and AcpiOsVprintf
 *
 *****************************************************************************/

void
AcpiOsRedirectOutput (
    void                    *Destination)
{
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsPrintf
 *
 * PARAMETERS:  fmt, ...            Standard printf format
 *
 * RETURN:      None
 *
 * DESCRIPTION: Formatted output
 *
 *****************************************************************************/

void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf (
    const char              *format,
    ...)
{
	va_list ap;
	
	va_start (ap, format);
	
	KLog2 (format, ap);

	va_end (ap);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsVprintf
 *
 * PARAMETERS:  fmt                 Standard printf format
 *              args                Argument list
 *
 * RETURN:      None
 *
 * DESCRIPTION: Formatted output with argument list pointer
 *
 *****************************************************************************/

void
AcpiOsVprintf (
    const char              *format,
    va_list                 ap)
{
	KLog2 (format, ap);
	
    return;
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetLine
 *
 * PARAMETERS:  fmt                 Standard printf format
 *              args                Argument list
 *
 * RETURN:      Actual bytes read
 *
 * DESCRIPTION: Formatted input with argument list pointer
 *
 *****************************************************************************/

UINT32
AcpiOsGetLine (
    char                    *Buffer)
{
    return 0;
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsMapMemory
 *
 * PARAMETERS:  where               Physical address of memory to be mapped
 *              length              How much memory to map
 *
 * RETURN:      Pointer to mapped memory.  Null on error.
 *
 * DESCRIPTION: Map physical memory into caller's address space
 *
 *****************************************************************************/

void *
AcpiOsMapMemory (
    ACPI_PHYSICAL_ADDRESS   where,
    ACPI_SIZE               length)
{
	vm_addr va;
	vm_addr map_pbase;
	vm_size map_size;
	vm_addr ret_addr;
	
	map_pbase = ALIGN_DOWN((vm_addr)where, 4096);
	map_size = (where % 4096) + length;


	
	if ((va = KMapPhys (map_pbase, map_size, VM_PROT_READWRITE)) != MAP_FAILED)
	{
		ret_addr = (where % 4096) + va;

		KPRINTF ("** %#010x = AcpiOsMapMemory (where = %#010x, len = %d", ret_addr, where, length);

		return (void *)ret_addr;
	}
	else
	{
		KPRINTF ("** NULL = AcpiOsMapMemory (where = %#010x, len = %d", where, length);

		return NULL;
	}
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsUnmapMemory
 *
 * PARAMETERS:  where               Logical address of memory to be unmapped
 *              length              How much memory to unmap
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete a previously created mapping.  Where and Length must
 *              correspond to a previous mapping exactly.
 *
 *****************************************************************************/

void
AcpiOsUnmapMemory (
    void                    *where,
    ACPI_SIZE               length)
{
	vm_addr map_pbase;
	
	
	KPRINTF ("* AcpiOsUnmapMemory(where = %#010x, len = %d)", where, length);

	map_pbase = ALIGN_DOWN((vm_addr)where, 4096);

	
	KUnmap ((vm_offset) map_pbase);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsAllocate
 *
 * PARAMETERS:  Size                Amount to allocate, in bytes
 *
 * RETURN:      Pointer to the new allocation.  Null on error.
 *
 * DESCRIPTION: Allocate memory.  Algorithm is dependent on the OS.
 *
 *****************************************************************************/

void *
AcpiOsAllocate (
    ACPI_SIZE               size)
{
    return KMalloc (size);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsFree
 *
 * PARAMETERS:  mem                 Pointer to previously allocated memory
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Free memory allocated via AcpiOsAllocate
 *
 *****************************************************************************/

void
AcpiOsFree (
    void                    *mem)
{
	KFree (mem);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsCreateSemaphore
 *
 * PARAMETERS:  InitialUnits        - Units to be assigned to the new semaphore
 *              OutHandle           - Where a handle will be returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create an OS semaphore
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsCreateSemaphore (
    UINT32                  MaxUnits,
    UINT32                  InitialUnits,
    ACPI_HANDLE             *OutHandle)
{
	struct ACPISemaphore *sem;
	
	if ((sem = KMalloc (sizeof (struct ACPISemaphore))) != NULL)
	{
		MutexInit (&sem->mutex);
		CondInit (&sem->cond);
		
		sem->max_units = MaxUnits;
		sem->current_units = InitialUnits;
		
		*OutHandle = (ACPI_HANDLE)sem;
		
	    return AE_OK;
	}

	*OutHandle = NULL;	
    return AE_NO_MEMORY;
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsDeleteSemaphore
 *
 * PARAMETERS:  Handle              - Handle returned by AcpiOsCreateSemaphore
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Delete an OS semaphore
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsDeleteSemaphore (
    ACPI_HANDLE             Handle)
{
	struct ACPISemaphore *sem = (struct ACPISemaphore *) Handle;
	
	KFree (sem);
	return AE_OK;
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsWaitSemaphore
 *
 * PARAMETERS:  Handle              - Handle returned by AcpiOsCreateSemaphore
 *              Units               - How many units to wait for
 *              Timeout             - How long to wait
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Wait for units
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsWaitSemaphore (
    ACPI_HANDLE             handle,
    UINT32                  units,
    UINT16                  timeout)
{
	ACPI_STATUS rc;
	struct ACPISemaphore *sem = (struct ACPISemaphore *)handle;
	struct TimeVal tv, current_tv, timeout_tv;
	int timeout_status;
	
	
	if (units < 1 && units > sem->max_units)
		return AE_BAD_PARAMETER;
	
	MutexLock (&sem->mutex);
	
	if (sem->current_units < units)
	{
		if (units == 0)
		{
			if (sem->current_units >= units)
			{
				rc = AE_OK;
				sem->current_units -= units;
			}
			else
				rc = AE_TIME;
		}
		else if (units == 0xffff)
		{
			while (sem->current_units < units)
				CondWait (&sem->cond, &sem->mutex);
		
			sem->current_units -= units;
			rc = AE_OK;
		}
		else
		{
			KGetTimeOfDay (&current_tv);
			timeout_tv.seconds = timeout / 1000;
			timeout_tv.microseconds = (timeout % 1000) * 1000;
						
			AddTime (&current_tv, &timeout_tv, &tv);
			
			timeout_status = 0;
			
			while (sem->current_units < units)
			{
				if (CondTimedWait (&sem->cond, &sem->mutex, &tv) == ETIMEDOUT)
				{
					timeout_status = 1;
					break;
				}
			}
		
			if (timeout_status == 0)
			{
				sem->current_units -= units;
				rc = AE_OK;
			}
			else
				rc = AE_TIME;
		}
	}
	else
	{
		sem->current_units -= units;
		rc = AE_OK;
	}

	MutexUnlock (&sem->mutex);
	
    return rc;
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsSignalSemaphore
 *
 * PARAMETERS:  Handle              - Handle returned by AcpiOsCreateSemaphore
 *              Units               - Number of units to send
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Send units
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsSignalSemaphore (
    ACPI_HANDLE             handle,
    UINT32                  units)
{
	ACPI_STATUS rc;
	struct ACPISemaphore *sem = (struct ACPISemaphore *)handle;
	
	
	if (units < 1 && units > sem->max_units)
		return AE_BAD_PARAMETER;
		
	MutexLock (&sem->mutex);

	sem->current_units += units;
	
	if (sem->current_units > sem->max_units)
	{
		sem->current_units = sem->max_units;
		rc = AE_LIMIT;
	}
	else
		rc = AE_OK;	
	
	CondBroadcast (&sem->cond);
	
	MutexUnlock (&sem->mutex);
	
    return rc;
}




/******************************************************************************
 *
 * ACPI lock functions
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsCreateLock (
    ACPI_SPINLOCK           *OutHandle)
{

    return (AcpiOsCreateSemaphore (1, 1, OutHandle));
}


void
AcpiOsDeleteLock (
    ACPI_SPINLOCK           Handle)
{
    AcpiOsDeleteSemaphore (Handle);
}


ACPI_CPU_FLAGS
AcpiOsAcquireLock (
    ACPI_HANDLE             Handle)
{
    AcpiOsWaitSemaphore (Handle, 1, 0xFFFF);
    return (0);
}


void
AcpiOsReleaseLock (
    ACPI_SPINLOCK           Handle,
    ACPI_CPU_FLAGS          Flags)
{
    AcpiOsSignalSemaphore (Handle, 1);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsInstallInterruptHandler
 *
 * PARAMETERS:  InterruptNumber     Level handler should respond to.
 *              Isr                 Address of the ACPI interrupt handler
 *              ExceptPtr           Where status is returned
 *
 * RETURN:      Handle to the newly installed handler.
 *
 * DESCRIPTION: Install an interrupt handler.  Used to install the ACPI
 *              OS-independent handler.
 *
 *****************************************************************************/

UINT32
AcpiOsInstallInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine,
    void                    *Context)
{
	KPRINTF ("* AcpiOsInstallInterruptHandler()");

	kacpi_isr_number = InterruptNumber;
	kacpi_isr_routine = ServiceRoutine;
	kacpi_isr_context = Context;

	if ((kacpi_isr_handler = ISRHandlerInsert (InterruptNumber, &AcpiOsInterruptHandlerStub, NULL)) != NULL)
		return AE_OK;
	else
	    return AE_ERROR;
}



int32 AcpiOsInterruptHandlerStub (int32 irq_idx, void *arg)
{
	INT32 status;
	
	KPANIC ("**** AcpiOsInterruptHandlerStub() ****");	
	
	status = (*kacpi_isr_routine)(kacpi_isr_context);

	if (status == ACPI_INTERRUPT_HANDLED)
		return TRUE;
	else
		return FALSE;
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsRemoveInterruptHandler
 *
 * PARAMETERS:  Handle              Returned when handler was installed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Uninstalls an interrupt handler.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsRemoveInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine)
{
	KPRINTF ("* AcpiOsRemoveInterruptHandler()");
	
	if (kacpi_isr_handler != NULL)
		ISRHandlerRemove (kacpi_isr_handler);
	
    return AE_OK;
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsExecute
 *
 * PARAMETERS:  Type            - Type of execution
 *              Function        - Address of the function to execute
 *              Context         - Passed as a parameter to the function
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Execute a new thread
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsExecute (
    ACPI_EXECUTE_TYPE       Type,
    ACPI_OSD_EXEC_CALLBACK  Function,
    void                    *Context)
{
	KPANIC ("* AcpiOsExecute() - FIXME, dummy");

/* FIXME: 
	AllocACPIDPCMessage()
	acpi_dpc->type = type;
	acpi_dpc->function = Function;
	acpi_dpc->context = Context;
	PutMsg ();
*/
	
	
/*	KeInsertQueueDpc(&AcpiDpc, (PVOID)Function, (PVOID)Context); */


//    _beginthread (Function, (unsigned) 0, Context);
    return 0;
}







/******************************************************************************
 *
 * FUNCTION:    AcpiOsBreakpoint
 *
 * PARAMETERS:  Msg                 Message to print
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Print a message and break to the debugger.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsBreakpoint (
    char                    *Msg)
{
    return AE_OK;
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsStall
 *
 * PARAMETERS:  microseconds        To sleep
 *
 * RETURN:      Blocks until sleep is completed.
 *
 * DESCRIPTION: Sleep at microsecond granularity
 *
 *****************************************************************************/

void
AcpiOsStall (
    UINT32                  microseconds)
{
    UDelay (microseconds);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsSleep
 *
 * PARAMETERS:  milliseconds        To sleep
 *
 * RETURN:      Blocks until sleep is completed.
 *
 * DESCRIPTION: Sleep at millisecond granularity
 *
 *****************************************************************************/

void
AcpiOsSleep (
    ACPI_INTEGER            milliseconds)
{
	KSleep2 (milliseconds/1000, milliseconds%1000);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetTimer
 *
 * PARAMETERS:  None
 *
 * RETURN:      Current time in 100 nanosecond units
 *
 * DESCRIPTION: Get the current system time
 *
 *****************************************************************************/

UINT64
AcpiOsGetTimer (void)
{
	struct TimeVal time;
	
	KGetTimeOfDay (&time);
	
    return (((UINT64) time.seconds * 10000000) + ((UINT64) time.microseconds * 10));
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsValidateInterface
 *
 * PARAMETERS:  Interface           - Requested interface to be validated
 *
 * RETURN:      AE_OK if interface is supported, AE_SUPPORT otherwise
 *
 * DESCRIPTION: Match an interface string to the interfaces supported by the
 *              host. Strings originate from an AML call to the _OSI method.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsValidateInterface (
    char                    *Interface)
{
	KPRINTF ("AcpiOsValidateInterface()");
    return (AE_SUPPORT);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsValidateAddress
 *
 * PARAMETERS:  SpaceId             - ACPI space ID
 *              Address             - Physical address
 *              Length              - Address length
 *
 * RETURN:      AE_OK if Address/Length is valid for the SpaceId. Otherwise,
 *              should return AE_AML_ILLEGAL_ADDRESS.
 *
 * DESCRIPTION: Validate a system address via the host OS. Used to validate
 *              the addresses accessed by AML operation regions.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsValidateAddress (
    UINT8                   SpaceId,
    ACPI_PHYSICAL_ADDRESS   Address,
    ACPI_SIZE               Length)
{
	KPRINTF ("* AcpiOsValidateAddress()");
    return (AE_OK);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadPciConfiguration
 *
 * PARAMETERS:  PciId               Seg/Bus/Dev
 *              Register            Device Register
 *              Value               Buffer where value is placed
 *              Width               Number of bits
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Read data from PCI configuration space
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsReadPciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Register,
    void                    *Value,
    UINT32                  Width)
{
	KPRINTF ("AcpiOsReadPciConfiguration()");

	/* FIXME:

	switch (Width)
	{		
	    case 8:
	        *(uint8 *)Value = PciReadByte (PciId->Bus, PciId->Device, PciId->Function, Register);
	        break;
	
	    case 16:
	        *(uint16 *)Value = PciReadWord (PciId->Bus, PciId->Device, PciId->Function, Register);
	        break;
	
	    case 32:
	        *(uint32 *)Value = PciReadLong (PciId->Bus, PciId->Device, PciId->Function, Register);
	        break;
		default:
			return (AE_BAD_PARAMETER);
			break;
	}
	
	*/
	
	
    return (AE_OK);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsWritePciConfiguration
 *
 * PARAMETERS:  PciId               Seg/Bus/Dev
 *              Register            Device Register
 *              Value               Value to be written
 *              Width               Number of bits
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Write data to PCI configuration space
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsWritePciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Register,
    ACPI_INTEGER            Value,
    UINT32                  Width)
{
	KPRINTF ("AcpiOsWritePciConfiguration()");
	
	/* FIXME:
	
	switch (Width)
	{		
	    case 8:
	        PciWriteByte (PciId->Bus, PciId->Device, PciId->Function, Register, Value);
	        break;
	
	    case 16:
	        PciWriteWord (PciId->Bus, PciId->Device, PciId->Function, Register, Value);
	        break;
	
	    case 32:
	        PciWriteLong (PciId->Bus, PciId->Device, PciId->Function, Register, Value);
	        break;
		default:
			return (AE_BAD_PARAMETER);
			break;
	}
	*/

    return (AE_OK);
}




/* TEMPORARY STUB FUNCTION */
void
AcpiOsDerivePciId(
    ACPI_HANDLE             rhandle,
    ACPI_HANDLE             chandle,
    ACPI_PCI_ID             **PciId)
{
	return;
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadPort
 *
 * PARAMETERS:  Address             Address of I/O port/register to read
 *              Value               Where value is placed
 *              Width               Number of bits
 *
 * RETURN:      Value read from port
 *
 * DESCRIPTION: Read data from an I/O port or register
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsReadPort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  *Value,
    UINT32                  Width)
{
	KPRINTF ("AcpiOsReadPort()");
	
    switch (Width)
    {
	    case 8:
	        *Value = InByte (Address);
	        break;
	
	    case 16:
	        *Value = InWord(Address);
	        break;
	
	    case 32:
	        *Value = InLong(Address);
	        break;
	        
		default:
			return (AE_BAD_PARAMETER);
			break;
	    }

    return (AE_OK);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsWritePort
 *
 * PARAMETERS:  Address             Address of I/O port/register to write
 *              Value               Value to write
 *              Width               Number of bits
 *
 * RETURN:      None
 *
 * DESCRIPTION: Write data to an I/O port or register
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsWritePort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  Value,
    UINT32                  Width)
{
	KPRINTF ("AcpiOsWritePort()");

    switch (Width)
    {
	    case 8:
	        OutByte (Address, Value);
	        break;
	
	    case 16:
	        OutWord (Address, Value);
	        break;
	
	    case 32:
	        OutLong (Address, Value);
	        break;
	    
		default:
			return (AE_BAD_PARAMETER);
			break;
	}
    return (AE_OK);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadMemory
 *
 * PARAMETERS:  Address             Physical Memory Address to read
 *              Value               Where value is placed
 *              Width               Number of bits
 *
 * RETURN:      Value read from physical memory address
 *
 * DESCRIPTION: Read data from a physical memory address
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsReadMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  *Value,
    UINT32                  Width)
{
   switch (Width)
   {
		case 8:
			*Value = (*(uint8 *)Address);
			break;
		case 16:
			*Value = (*(uint16 *)Address);
			break;
		case 32:
			*Value = (*(uint32 *)Address);
			break;
		default:
			return (AE_BAD_PARAMETER);
			break;
	}

    return (AE_OK);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsWriteMemory
 *
 * PARAMETERS:  Address             Physical Memory Address to write
 *              Value               Value to write
 *              Width               Number of bits
 *
 * RETURN:      None
 *
 * DESCRIPTION: Write data to a physical memory address
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsWriteMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  Value,
    UINT32                  Width)
{
   switch (Width)
   {
		case 8:
			*(uint8 *)Address = Value;
			break;
		case 16:
			*(uint16 *)Address = Value;
			break;
		case 32:
			*(uint32 *)Address = Value;
			break;
		default:
			return (AE_BAD_PARAMETER);
			break;
	}

    return (AE_OK);
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetThreadId
 *
 * PARAMETERS:  None
 *
 * RETURN:      Thread ID
 *
 * DESCRIPTION: Miscellaneous functions
 *
 *****************************************************************************/

ACPI_THREAD_ID
AcpiOsGetThreadId(void)
{
	return GetPID();
}




/******************************************************************************
 *
 * FUNCTION:    AcpiOsSignal
 *
 * PARAMETERS:  Function            ACPI CA signal function code
 *              Info                Pointer to function-dependent structure
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Miscellaneous functions
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsSignal (
    UINT32                  Function,
    void                    *Info)
{
    switch (Function)
    {
	    case ACPI_SIGNAL_FATAL:
	        if (Info)
	        {
	            AcpiOsPrintf ("AcpiOsBreakpoint: %s ****\n", Info);
	        }
	        else
	        {
	            AcpiOsPrintf ("At AcpiOsBreakpoint ****\n");
	        }
			break;
	    case ACPI_SIGNAL_BREAKPOINT:
	
	        if (Info)
	        {
	            AcpiOsPrintf ("AcpiOsBreakpoint: %s ****\n", Info);
	        }
	        else
	        {
	            AcpiOsPrintf ("At AcpiOsBreakpoint ****\n");
	        }
	
	        break;
	    }

    return (AE_OK);
}






