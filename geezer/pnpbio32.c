/*----------------------------------------------------------------------------
PnP demo for DJGPP or 32-bit Watcom C, using 16-bit PnP BIOS (thunking)
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: Feb 23, 2005
This code is public domain (no copyright).
You can do whatever you want with it.
----------------------------------------------------------------------------*/
#include <stdio.h>
#include <dos.h>
#if 0
/* C99 fixed-width types */
#include <stdint.h>
#else
typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
#endif

#if defined(__DJGPP__)
#include <sys/nearptr.h>
#include <dpmi.h>
#include <crt0.h>

#elif defined(__WATCOMC__)
#if !defined(__386__)
#error This is a 32-bit program. Compile with WCC386
#endif

/* using CauseWay DOS extender: */
#define	__djgpp_conventional_base	0

#else
#error Sorry, unsupported compiler
#endif

/* 16:16 far address of the 16-bit pmode code we want to execute
(e.g. PnP BIOS, APM BIOS, VBE 3.x video BIOS, etc.) */
#pragma pack(1)
static struct
{
	uint16_t ip;
	uint16_t cs;
} g_target;

/* 16:32 far address of thunk_help(), which is also 16-bit code */
#pragma pack(1)
static struct
{
	uint32_t eip;
	uint16_t cs;
} g_thunk;

static unsigned g_pnp_ds, g_ptr1_ds, g_ptr2_ds;
/*****************************************************************************
This is actually a 16-bit function (NASM-syntax assembly in comments).
Call with EAX pointing to 16:16 far target address,
and arguments (left to right, if any) in registers EBX, ECX, EDX, and EDI.
*****************************************************************************/
static const unsigned char thunk_help[] =
{				/* BITS 16 */
/* push up to eight 16-bit arguments. If 16-bit callee takes no args,
these will be ignored. Note that the stack is still 32-bit here. */
	0x66, 0x57,		/* push edi */
	0x66, 0x52,		/* push edx */
	0x66, 0x51,		/* push ecx */
	0x66, 0x53,		/* push ebx */
/* call 16-bit code with 16:16 far return address, suitable for 16-bit RETF */
	    0x67, 0xFF, 0x18,	/* call far [eax] */
/* clean up stack */
	0x66, 0x83, 0xC4, 0x10,	/* add esp,byte 16 */
/* pop EIP:CS to return to 32-bit code. */
	0x66, 0xCB		/* o32 retf */
};
/*****************************************************************************
*****************************************************************************/
#if defined(__WATCOMC__)
static int __dpmi_allocate_ldt_descriptors(unsigned num_descriptors)
{
	union REGS regs;

	regs.w.ax = 0x0000;
	regs.w.cx = num_descriptors;
	int386(0x31, &regs, &regs);
	return regs.w.cflag ? -1 : regs.w.ax;
}
/*****************************************************************************
*****************************************************************************/
static int __dpmi_set_segment_base_address(unsigned sel, unsigned base_adr)
{
	union REGS regs;

	regs.w.ax = 0x0007;
	regs.w.bx = sel;
	regs.w.cx = base_adr >> 16;
	regs.w.dx = base_adr & 0xFFFF;
	int386(0x31, &regs, &regs);
	return regs.w.cflag ? -1 : 0;
}
/*****************************************************************************
*****************************************************************************/
static int __dpmi_set_segment_limit(unsigned sel, unsigned limit)
{
	union REGS regs;

	regs.w.ax = 0x0008;
	regs.w.bx = sel;
	regs.w.cx = limit >> 16;
	regs.w.dx = limit & 0xFFFF;
	int386(0x31, &regs, &regs);
	return regs.w.cflag ? -1 : 0;
}
/*****************************************************************************
*****************************************************************************/
static int __dpmi_set_descriptor_access_rights(unsigned sel, unsigned access)
{
	union REGS regs;

	regs.w.ax = 0x0009;
	regs.w.bx = sel;
	regs.w.cx = access;
	int386(0x31, &regs, &regs);
	return regs.w.cflag ? -1 : 0;
}
#endif
/*****************************************************************************
create descriptor and selector for 16-bit ring 3 code segment
with base address base_adr and limit 0xFFFF
*****************************************************************************/
static int create_cs16(unsigned long base_adr)
{
	int sel;

	sel = __dpmi_allocate_ldt_descriptors(1);
	if(sel == -1)
		return -1;
/* 0x00FB = byte-granular, 16-bit, present, ring 3, code, non-conforming,
readable, accessed */
	if(__dpmi_set_descriptor_access_rights(sel, 0x00FB))
		return -1;
	if(__dpmi_set_segment_base_address(sel, base_adr))
		return -1;
	if(__dpmi_set_segment_limit(sel, 0xFFFF))
		return -1;
	return sel;
}
/*****************************************************************************
create descriptor and selector for 16-bit ring 3 data segment
with base address base_adr and limit 0xFFFF
*****************************************************************************/
static int create_ds16(unsigned long base_adr)
{
	int sel;

	sel = __dpmi_allocate_ldt_descriptors(1);
	if(sel == -1)
		return -1;
/* 0x00F3 = byte-granular, 16-bit, present, ring 3, data, expand-up,
writable, accessed */
	if(__dpmi_set_descriptor_access_rights(sel, 0x00F3))
		return -1;
	if(__dpmi_set_segment_base_address(sel, base_adr))
		return -1;
	if(__dpmi_set_segment_limit(sel, 0xFFFF))
		return -1;
	return sel;
}
/*****************************************************************************
*****************************************************************************/
static int init_thunk(unsigned long ds_base,
		unsigned long cs_base, unsigned entry)
{
	int i;

/* 32-bit code can't call 16-bit code directly, even with a far call.
This is because the 32-bit code may have an EIP value > 0xFFFF,
and the 16-bit RETF will pop only the bottom 16 bits (IP).

We create a small stub that makes a far call to the 16-bit code
(thunk_help()), end the stub with a 32-bit RETF, and encapsulate
the stub in a 16-bit code segment to restrict EIP to <= 0xFFFF */
	i = create_cs16((unsigned long)thunk_help -
		__djgpp_conventional_base);
	if(i == -1)
		return -1;
printf("thunk_help: virtual adr=0x%p, physical adr (base)=0x%p, CS=0x%X\n",
 thunk_help, thunk_help - __djgpp_conventional_base, i);
	g_thunk.cs = i;
	g_thunk.eip = 0;
/* now create a 16-bit code segment for the target code
(e.g. PnP BIOS, APM BIOS, VBE 3.x video BIOS, etc.) */
	i = create_cs16(cs_base);
	if(i == -1)
		return -1;
printf("target code: physical adr (base)=0x%lX, entry (IP)=0x%X, CS=0x%X\n",
 cs_base, entry, i);
	g_target.cs = i;
	g_target.ip = entry;
/* PnP needs three more 16-bit data segment descriptors/selectors:
PnP BIOS data segment */
	i = create_ds16(ds_base);
	if(i == -1)
		return -1;
	g_pnp_ds = i;
/* ...and two 16-bit data segments to encapsulate pointer arguments */
	i = create_ds16(0);
	if(i == -1)
		return -1;
	g_ptr1_ds = i;

	i = create_ds16(0);
	if(i == -1)
		return -1;
	g_ptr2_ds = i;
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int pnp_detect(unsigned long *ds_base,
		unsigned long *cs_base, unsigned *entry)
{
	unsigned char *adr, csum;
	unsigned long offset;
	unsigned len, i;

	printf("16-bit PnP BIOS...");
/* the spec says it's in this range, aligned on a 16-byte boundary */
	for(offset = 0; offset < 0xFFF0; offset += 16)
	{
		adr = (unsigned char *)(0xF0000L +
			offset + __djgpp_conventional_base);
/* bytes 0-3: signature */
		if(adr[0] != '$' || adr[1] != 'P' ||
			adr[2] != 'n' || adr[3] != 'P')
				continue;
/* byte 5: structure len */
		len = adr[5];
		if(len == 0)
			continue;
/* byte 8: checksum (checksum of entire structure must be 0) */
		csum = 0;
		for(i = 0; i < len; i++)
			csum += adr[i];
		if(csum)
			continue;
/* found it! */
		goto FOUND;
	}
	printf("not found\n");
	return -1;
FOUND:
/* byte 4: PnP version */
	i = adr[4];
	printf("version %u.%u, ", i >> 4, i & 0x0F);
/* get CS base... */
	offset = *(uint32_t *)(adr + 19);
	printf("entry point at %05lX:", offset);
	*cs_base = offset;
/* ...and entry point... */
	i = *(uint16_t *)(adr + 17);
	printf("%04X, ", i);
	*entry = i;
/* ...and DS base */
	offset = *(uint32_t *)(adr + 29);
	printf("DS=0x%lX\n", offset);
	*ds_base = offset;
	return 0;
}
/*****************************************************************************
int (*entry)(uint16_t fn, uint8_t far *num_nodes,
  uint16_t far *node_size, uint16_t pnp_ds);
=
int (*entry)(uint16_t fn, uint16_t num_nodes_off, uint16_t num_nodes_seg,
  uint16_t node_size_off, uint16_t node_size_seg, uint16_t pnp_ds);

call init_thunk() before calling this function,
to set g_thunk and g_target
*****************************************************************************/
static int pnp_count_nodes(uint8_t *num_nodes, uint16_t *max_node_size)
{
	static const uint16_t fn = 0;
/**/
	unsigned long arg12, arg34, arg56;
	int rv;

/* pointer argument is an address, and its offset could be > 0xFFFF,
which is not compatible with 16-bit code. Encapsulate each pointer
argument in a 16-bit data segment. */
	if(__dpmi_set_segment_base_address(g_ptr1_ds,
		(unsigned long)num_nodes -
		__djgpp_conventional_base) == -1)
			return -1;
	if(__dpmi_set_segment_base_address(g_ptr2_ds,
		(unsigned long)max_node_size -
		__djgpp_conventional_base) == -1)
			return -1;
/* put args in registers. g_thunk[]/thunk_help() will push them
onto the stack before calling the target 16-bit code */
	arg12 = fn |		/* arg1: fn=0 */
		(0 << 16);	/* arg2: FP_OFF(&num_nodes)=0 */
	arg34 = g_ptr1_ds |	/* arg3: FP_SEG(&num_nodes)=g_ptr1_ds */
		(0 << 16);	/* arg4: FP_OFF(&max_node_size)=0 */
	arg56 = g_ptr2_ds |	/* arg5: FP_SEG(&max_node_size)=g_ptr2_ds */
		(g_pnp_ds << 16);/* arg6: g_pnp_ds */
#if defined(__DJGPP__)
	__asm__("lcall *(%1)\n"
/* outputs:
return value from 16-bit code in (E)AX */
		: "=a"(rv)
/* inputs:
16:32 far pointer to thunk_help() in ESI */
		: "S"(&g_thunk),
/* 16:16 far pointer to target code in EAX */
		  "0"(&g_target),
/* args */
		  "b"(arg12),
		  "c"(arg34),
		  "d"(arg56));
#elif defined(__WATCOMC__)
	_asm
	{
/* grrrrr.....
		mov esi,&g_thunk
		mov eax,&g_target */
		lea esi,g_thunk
		lea eax,g_target
		mov ebx,arg12
		mov ecx,arg34
		mov edx,arg56
/* GRRRRRR....
		call far [esi] */
		db 0xFF, 0x1E

		mov rv,eax
	}
#endif
	return rv;
}
/*****************************************************************************
int (*entry)(uint16_t fn, uint8_t far *node_num,
  uint8_t far *buf, uint16_t boot, uint16_t pnp_ds);
=
int (*entry)(uint16_t fn, uint16_t node_num_off, uint16_t node_num_seg,
  uint16_t buf_off, uint16_t buf_seg, uint16_t boot, uint16_t pnp_ds);

call init_thunk() before calling this function,
to set g_thunk and g_target
*****************************************************************************/
static int pnp_get_node(uint8_t *node_num, uint8_t *buf)
{
/* b0=get dynamic (current) node value,
b1=get static node value (value for next boot) */
	static const uint16_t boot = 0x01;
	static const uint16_t fn = 1;
/**/
	unsigned long arg12, arg34, arg56, arg78;
	int rv;

	if(__dpmi_set_segment_base_address(g_ptr1_ds,
		(unsigned long)node_num -
		__djgpp_conventional_base) == -1)
			return -1;
	if(__dpmi_set_segment_base_address(g_ptr2_ds,
		(unsigned long)buf -
		__djgpp_conventional_base) == -1)
			return -1;
	arg12 = fn |		/* arg1: fn=1 */
		(0 << 16);	/* arg2: FP_OFF(&node_num)=0 */
	arg34 = g_ptr1_ds |	/* arg3: FP_SEG(&node_num)=g_ptr1_ds */
		(0 << 16);	/* arg4: FP_OFF(&buf)=0 */
	arg56 = g_ptr2_ds |	/* arg5: FP_SEG(&buf)=g_ptr2_ds */
		(boot << 16);	/* arg6: boot=0x01 */
	arg78 = g_pnp_ds |	/* arg7: g_pnp_ds */
		(0 << 16);	/* arg8: (not used) */
#if defined(__DJGPP__)
	__asm__("lcall *(%1)\n"
/* outputs:
return value from 16-bit code in (E)AX */
		: "=a"(rv)
/* inputs:
16:32 far pointer to thunk_help() in ESI */
		: "S"(&g_thunk),
/* 16:16 far pointer to target code in EAX */
		  "0"(&g_target),
/* args */
		  "b"(arg12),
		  "c"(arg34),
		  "d"(arg56),
		  "D"(arg78));
#elif defined(__WATCOMC__)
	_asm
	{
		lea esi,g_thunk
		lea eax,g_target
		mov ebx,arg12
		mov ecx,arg34
		mov edx,arg56
		mov edi,arg78
		db 0xFF, 0x1E /* call far [esi] */
		mov rv,eax
	}
#endif
	return rv;
}
/*****************************************************************************
converts 4-character "compressed ASCII" PnP ID at src
to 7-character normal ASCII at dst
*****************************************************************************/
static void decode_id(char *dst, unsigned char *src)
{
	unsigned i;

/* why are these values Big Endian??? */
	i = src[2];
	i <<= 8;
	i |= src[3];
	sprintf(dst + 3, "%04X", i);

	i = src[0];
	i <<= 8;
	i |= src[1];
	dst[2] = '@' + (i & 0x1F);
	i >>= 5;
	dst[1] = '@' + (i & 0x1F);
	i >>= 5;
	dst[0] = '@' + (i & 0x1F);
}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	static unsigned char buf[256];
/**/
	unsigned long ds_base, cs_base;
	uint8_t num_nodes, node;
	uint16_t max_node_size;
	unsigned entry;
	int i;

#if defined(__DJGPP__)
/* turn off data segment limit, for nearptr access */
	if(!(_crt0_startup_flags & _CRT0_FLAG_NEARPTR))
	{
		if(!__djgpp_nearptr_enable())
		{
			printf("Can't enable near pointer "
				"access (WinNT/2k/XP?)\n");
			return -1;
		}
	}
#endif
/* detect 16-bit PnP BIOS */
	if(pnp_detect(&ds_base, &cs_base, &entry))
		return 1;
/* create 16-bit pmode interface */
	if(init_thunk(ds_base, cs_base, entry))
PM_ERR:	{
		printf("Error initializing 16-bit protected mode interface\n");
		return 1;
	}
/* call PnP function #0 to get buf count and maximum size */
	i = pnp_count_nodes(&num_nodes, &max_node_size);
	if(i == -1)
		goto PM_ERR;
	i &= 0xFFFF;
	if(i)
PNP_ERR:{
		printf("PnP BIOS call 0 returned 0x%X\n", i);
		return 1;
	}
	printf("%u nodes, max %u bytes each\n", num_nodes, max_node_size);
/* read nodes and dump PnP ID */
	for(node = 0; node != 0xFF; )
	{
		i = pnp_get_node(&node, buf);
		if(i == -1)
			goto PM_ERR;
		i &= 0xFFFF;
		if(i)
			goto PNP_ERR;
		decode_id(buf + 128, buf + 3);
		printf("%.7s ", buf + 128);
	}
	putchar('\n');
	return 0;
}

