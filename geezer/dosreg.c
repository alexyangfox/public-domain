/*----------------------------------------------------------------------------
DOS access to Windows registry
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: October 28, 2008
This code is public domain (no copyright).
You can do whatever you want with it.

Compile with Turbo C. Also compiles with 16-bit Watcom C,
but it doesn't run properly, and I don't know why.
----------------------------------------------------------------------------*/
#include <string.h> /* memset() */
#include <stdio.h> /* printf() */
#include <dos.h> /* union REGS, struct SREGS, MK_FP(), int86x() */

/* Turbo C++ 1.0	__TURBOC__=0x296
Turbo C++ 3.0		__TURBOC__=0x401
Borland C++ 3.1		__TURBOC__=0x410 */
#if defined(__TURBOC__)
#define	LOAD_AX(X)	__emit__(0xB8, (X) & 255, (X) / 256)

#elif defined(__WATCOMC__)
#if defined(__386__)
#error 16-bit program -- compile with WCC.EXE
#endif
/* this also works with Turbo C++ 3.0 and Borland C++ 1.0 */
#define	LOAD_AX(X)	_asm mov ax,X

#else
#error Sorry, unsupported compiler
#endif

/* VMM.VXD call ids */
#define REGOP_OPEN_KEY		0x0100
#define REGOP_CREATE_KEY	0x0101
#define REGOP_CLOSE_KEY		0x0102
#define REGOP_DELETE_KEY	0x0103
#define REGOP_SET_VALUE		0x0104
#define REGOP_QUERY_VALUE	0x0105
#define REGOP_ENUM_KEY		0x0106
#define REGOP_DELETE_VALUE	0x0107
#define REGOP_ENUM_VALUE	0x0108
#define REGOP_QUERY_VALUE_EX	0x0109

/* predefined HKEY_... values */
#define	HKEY_CLASSES_ROOT	0x80000000L
#define	HKEY_CURRENT_USER	0x80000001L
#define	HKEY_LOCAL_MACHINE	0x80000002L
#define	HKEY_USERS		0x80000003L
#define	HKEY_PERFORMANCE_DATA	0x80000004L
#define	HKEY_CURRENT_CONFIG	0x80000005L
#define	HKEY_DYN_DATA		0x80000006L

/* value types */
#define REGTYPE_NONE		0
#define REGTYPE_SZ		1 /* Unicode string */
#define REGTYPE_EXPAND_SZ	2 /* Unicode w/ environment variable expansion */
#define REGTYPE_BINARY		3 /* binary */
#define REGTYPE_DWORD		4 /* 32 bit number */
#define REGTYPE_DWORD_LE	4
#define REGTYPE_DWORD_BE	5 /* 32-bit number, big endian */

typedef unsigned long	uint32_t;

static long far cdecl (*g_vmm)(void);
/*****************************************************************************
*****************************************************************************/
#define BPERL		16	/* bytes per line for dump */

void dump(void *data_ptr, unsigned count)
{
	unsigned char *data = data_ptr;
	unsigned i, j;

	while(count != 0)
	{
		for(i = 0; i < BPERL; i++)
		{
			if(count == 0)
				break;
			printf("%02X ", data[i]);
			count--;
		}
		printf("\t");
		for(j = 0; j < i; j++)
		{
			if(data[j] < ' ')
				printf(".");
			else
				printf("%c", data[j]);
		}
		printf("\n");
		data += BPERL;
	}
}
/*****************************************************************************
(Table 02643)
Call Windows VMM 16-bit entry point with:
AX = 0100h "RegOpenKey"
STACK:	DWORD	-> DWORD for returned key handle
	DWORD	-> ASCIZ registry key name
	DWORD	HKEY (see #02644)

Note the peculiar calling convention: there is an extra stack frame
(IP and CS registers) on the stack, then the usual stack frame,
THEN the args to g_vmm(). At g_vmm(), the stack looks like this:

	hkey MSW
	hkey LSW
	key MSW
	key LSW
	handle MSW
	handle LSW
	CS		returns to main()
	IP		returns to main()
BP ->	BP
	CS		returns to reg_open_key(), reg_query_value_ex(), etc.
SP ->	IP		returns to reg_open_key(), reg_query_value_ex(), etc.

I learned this only by opening REGEDIT.EXE in a debugger, looking for
	mov ax,1684h
	int 2Fh
observing where ES and DI were stored, and searching the code for
far calls to that address.
*****************************************************************************/
#if defined(__TURBOC__)
/* "Parameter 'foo' is never used" -- actually, they are */
#pragma warn -par
#endif

static long far cdecl reg_open_key(uint32_t far *handle,
		char far *key, uint32_t hkey)
{
	LOAD_AX(REGOP_OPEN_KEY);
	return g_vmm();
}
/*****************************************************************************
(Table 02643)
Call Windows VMM 16-bit entry point with:
AX = 0105h "RegQueryValue"
STACK:	DWORD	-> DWORD for ???
	DWORD	-> ASCIZ ???
	DWORD	-> ASCIZ ???
	DWORD	HKEY (see #02644)

xxx - can't get this routine to work!
*****************************************************************************/
// doesn't work
static long far cdecl reg_query_value(uint32_t far *val_len, char far *val,
		char far *entry_name, uint32_t handle)

// doesn't work
//static long far reg_query_value(uint32_t far *val_len, char far *val,
//		char far *entry_name, uint32_t far *handle)
{
	LOAD_AX(REGOP_QUERY_VALUE);
	return g_vmm();
}
/*****************************************************************************
This routine is not documented in Ralf Brown's list
*****************************************************************************/
static long far cdecl reg_query_value_ex(uint32_t far *val_len, char far *val,
		uint32_t far *val_type, void far *reserved,
		char far *entry_name, uint32_t handle)
{
	LOAD_AX(REGOP_QUERY_VALUE_EX);
	return g_vmm();
}
/*****************************************************************************
(Table 02643)
Call Windows VMM 16-bit entry point with:
AX = function number
    0102h "RegCloseKey"
	STACK:	DWORD	key handle from RegOpenKey or RegCreateKey
*****************************************************************************/
static long far cdecl reg_close_key(uint32_t handle)
{
	LOAD_AX(REGOP_CLOSE_KEY);
	return g_vmm();
}
/*****************************************************************************
to do:
#define REGOP_CREATE_KEY	0x0101  <-- can modify registry!
#define REGOP_DELETE_KEY	0x0103  <-- can modify registry!
#define REGOP_SET_VALUE	0x0104  <-- can modify registry!
#define REGOP_ENUM_KEY	0x0106
#define REGOP_DELETE_VALUE	0x0107  <-- can modify registry!
#define REGOP_ENUM_VALUE	0x0108
*****************************************************************************/
int main(void)
{
	static const char *hkey_name[] =
	{
		"HKEY_CLASSES_ROOT",
		"HKEY_CURRENT_USER",
		"HKEY_LOCAL_MACHINE",
		"HKEY_USERS",
		"HKEY_PERFORMANCE_DATA",
		"HKEY_CURRENT_CONFIG",
		"HKEY_DYN_DATA"
	};
/**/
	uint32_t handle, data_len, data_type, hkey;
	char *key, *value, data[256];
	struct SREGS sregs;
	union REGS regs;
	long i;

	printf("\n");
/* get entry point of VMM.VXD */
	memset(&regs, 0, sizeof(regs));
	memset(&sregs, 0, sizeof(sregs));
	sregs.es = 0;
	regs.x.di = 0;
	regs.x.bx = 0x0001;	/* =INT 2Fh AX=1684h ID for VMM.VXD */
	regs.x.ax = 0x1684;
	int86x(0x2F, &regs, &regs, &sregs);
	g_vmm = MK_FP(sregs.es, regs.x.di);
	if(g_vmm == 0)
	{
		printf("Error: can't get V86 mode entry point of VMM.VXD\n");
		return 1;
	}
printf("\tV86 mode entry point for VMM.VXD=%Fp\n", g_vmm);
handle = 0xDEADBEEFL;
/* open registry key
	key = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion"; */
	key = "System\\CurrentControlSet\\Services\\VxD\\MSTCP";
	hkey = HKEY_LOCAL_MACHINE;
printf("\tcalling reg_open_key(%s)\n", key);
	i = reg_open_key((uint32_t far *)&handle,
		(char far *)key,
		hkey);
printf("\t    reg_open_key() returned 0x%lX; handle=0x%lX\n", i, handle);
/* retrieve a value
	value = "RegisteredOwner"; */
	value = "NameServer";
printf("\tcalling reg_query_value_ex(%s)\n", value);
	data_len = 256;
	data_type = REGTYPE_SZ;
	i = reg_query_value_ex((uint32_t far *)&data_len,
		(char far *)data,
		(uint32_t far *)&data_type,
		(void far *)NULL,
		(char far *)value,
		handle);
printf("\t    reg_query_value_ex() returned 0x%lX; "
 "data_len=%lu, data_type=%lu\n", i, data_len, data_type);
	if(hkey < HKEY_CLASSES_ROOT || hkey > HKEY_DYN_DATA)
		printf("<??\\");
	else
		printf("<%s\\", hkey_name[hkey - HKEY_CLASSES_ROOT]);
	printf("%s\\%s>=\n <%s>\n", key, value, data);
/* close key */
printf("\tcalling reg_close_key()\n");
	i = reg_close_key(handle);
printf("\t    reg_close_key() returned 0x%lX\n", i);
	return 0;
}
