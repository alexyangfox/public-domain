
#ifndef __ACKIELDER_H__
#define __ACKIELDER_H__



#define ACPI_DEBUG 1


#define ACPI_MACHINE_WIDTH          32


#define ACPI_SYSTEM_XFACE
#define ACPI_EXTERNAL_XFACE
#define ACPI_INTERNAL_XFACE
#define ACPI_INTERNAL_VAR_XFACE



#define ACPI_MACHINE_WIDTH          32
#define COMPILER_DEPENDENT_INT64    long long
#define COMPILER_DEPENDENT_UINT64   unsigned long long
#define ACPI_USE_NATIVE_DIVIDE


#define ACPI_USE_LOCAL_CACHE

#ifndef __cdecl
#define __cdecl
#endif

#define ACPI_FLUSH_CPU_CACHE()


#include "acgcc.h"

#endif /* __ACLINUX_H__ */

