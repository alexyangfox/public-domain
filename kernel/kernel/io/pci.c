#include <kernel/types.h>
#include <kernel/fs.h>
#include <kernel/lists.h>
#include <kernel/sync.h>
#include <kernel/utility.h>
#include <kernel/kmalloc.h>
#include <kernel/dbg.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/device.h>
#include <kernel/resident.h>
#include <kernel/pci.h>
#include "io.h"


uint8 PciReadConfigByte (uint32 bus, uint32 slot, uint32 func, uint32 offset);
uint16 PciReadConfigWord (uint32 bus, uint32 slot, uint32 func, uint32 offset);
uint32 PciReadConfigLong (uint32 bus, uint32 slot, uint32 func, uint32 offset);


#define	CONFIG_ADDRESS  0xCF8
#define	CONFIG_DATA	0xCFC




/*
 *
 */

uint8 PciReadConfigByte (uint32 bus, uint32 slot, uint32 func, uint32 offset)
{
	uint32 address;
	uint32 tmp = 0;
 
	address = (unsigned long)((bus << 16) | (slot << 11) |
				(func << 8) | (offset & 0xfc) | 0x80000000);
  
	OutLong (0xCF8, address);
	
	tmp = InLong (0xCFC);
	
	tmp = (tmp >> ((offset & 0x11) * 8)) & 0x000000ff;
	
	return (uint8)tmp;
}




/*
 *
 */

uint16 PciReadConfigWord (uint32 bus, uint32 slot, uint32 func, uint32 offset)
{
	uint32 address;
	uint32 tmp = 0;
 
	address = (unsigned long)((bus << 16) | (slot << 11) |
				(func << 8) | (offset & 0xfc) | 0x80000000);
  
	OutLong (0xCF8, address);
	
	tmp = InLong (0xCFC);
	
	tmp = (tmp >> ((offset & 0x10) * 8)) & 0x0000ffff;
	
	return (uint16) tmp;
}




/*
 *
 */

uint32 PciReadConfigLong (uint32 bus, uint32 slot, uint32 func, uint32 offset)
{
	uint32 address;
	uint32 tmp = 0;
 
	address = (unsigned long)((bus << 16) | (slot << 11) |
				(func << 8) | (offset & 0xfc) | 0x80000000);
  
	OutLong (0xCF8, address);
	
	tmp = InLong (0xCFC);
	
	return tmp;
}
