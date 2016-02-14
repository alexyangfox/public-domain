/*----------------------------------------------------------------------------
PCI demo for DJGPP, Turbo C, or Watcom C, using I/O ports
(configuration mechanism 1)

Chris Giese     <geezer@execpc.com>     http://my.execpc.com/~geezer
Release date: April 25, 2007
This code is public domain (no copyright).
You can do whatever you want with it.
----------------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>

#if defined(__DJGPP__)
#include <dos.h> /* inport[b|w|l](), outport[b|w|l]() */

#elif defined(__WATCOMC__)
#include <conio.h>

#define	inportb(P)	inp(P)
#define	inportw(P)	inpw(P)
#define	outportb(P,V)	outp(P,V)
#define	outportw(P,V)	outpw(P,V)
#if defined(__386__)
#define	inportl(P)	inpd(P)
#define	outportl(P,V)	outpd(P,V)
/* else [in|out]portl() defined below */
#endif

#elif defined(__TURBOC__)
#include <dos.h> /* inport[b](), outport[b]() */

#define	inportw(P)	inport(P)
#define	outportw(P,V)	outport(P,V)
/* [in|out]portl() defined below */

#else
#error Sorry, unsupported compiler
#endif

#define	PCI_ADR_REG		0xCF8
#define	PCI_DATA_REG		0xCFC

typedef struct
{
	unsigned char bus, dev, fn;
} pci_t;
/*****************************************************************************
These functions are _code_, but are stored in the _data_ segment.
Declare them 'far' and end them with 'retf' instead of 'ret'.

For 16-bit Watcom C, use 'cdecl' to force usage of normal, stack
calling convention instead of Watcom register calling convention.
*****************************************************************************/
#if defined(__TURBOC__) || (defined(__WATCOMC__)&&!defined(__386__))
static const unsigned char g_inportl[] =
{
	0x55,			/* push bp */
	0x8B, 0xEC,		/*  mov bp,sp */
	0x8B, 0x56, 0x06,	/*  mov dx,[bp + 6] */
	0x66, 0xED,		/*  in eax,dx */
	0x8B, 0xD0,		/*  mov dx,ax */
	0x66, 0xC1, 0xE8, 0x10,	/*  shr eax,16 */
	0x92,			/*  xchg dx,ax */
	0x5D,			/* pop bp */
	0xCB			/* retf */
};

static unsigned long far cdecl (*inportl)(unsigned port) =
	(unsigned long far (*)(unsigned))g_inportl;
/*****************************************************************************
*****************************************************************************/
static const unsigned char g_outportl[] =
{
	0x55,			/* push bp */
	0x8B, 0xEC,		/*  mov bp,sp */
	0x8B, 0x56, 0x06,	/*  mov dx,[bp + 6] */
	0x66, 0x8B, 0x46, 0x08,	/*  mov eax,[bp + 8] */
	0x66, 0xEF,		/*  out dx,eax */
	0x5D,			/* pop bp */
	0xCB			/* retf */
};

static void far cdecl (*outportl)(unsigned port, unsigned long val) =
	(void far (*)(unsigned, unsigned long))g_outportl;
#endif
/*****************************************************************************
*****************************************************************************/
static int pci_detect(void)
{
	printf("PCI controller...");
/* poke 32-bit I/O register at 0xCF8 to see if
there's a PCI controller there */
	outportl(PCI_ADR_REG, 0x80000000L); /* bus 0, dev 0, fn 0, reg 0 */
	if(inportl(PCI_ADR_REG) != 0x80000000L)
	{
		printf("not found\n");
		return -1;
	}
	printf("found\n");
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int pci_read_config_byte(pci_t *pci, unsigned reg,
		unsigned char *val)
{
	outportl(PCI_ADR_REG,
		0x80000000L | /* "enable configuration space mapping" */
		((unsigned long)pci->bus << 16) |	/* b23-b16=bus */
		((unsigned)pci->dev << 11) |		/* b15-b11=dev */
		((unsigned)pci->fn << 8) |		/* b10-b8 =fn  */
		(reg & ~3));				/* b7 -b2 =reg */
	*val = inportb(PCI_DATA_REG + (reg & 3));
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int pci_read_config_word(pci_t *pci, unsigned reg,
		unsigned short *val)
{
	outportl(PCI_ADR_REG, 0x80000000L |
		((unsigned long)pci->bus << 16) |
		((unsigned)pci->dev << 11) |
		((unsigned)pci->fn << 8) |
		(reg & ~3));
	*val = inportw(PCI_DATA_REG + (reg & 2));
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int pci_read_config_dword (pci_t *pci, unsigned reg,
		unsigned long *val)
{
	outportl(PCI_ADR_REG, 0x80000000L |
		((unsigned long)pci->bus << 16) |
		((unsigned)pci->dev << 11) |
		((unsigned)pci->fn << 8) |
		(reg & ~3));
	*val = inportl(PCI_DATA_REG + 0);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int pci_write_config_byte(pci_t *pci, unsigned reg,
		unsigned val)
{
	outportl(PCI_ADR_REG, 0x80000000L |
		((unsigned long)pci->bus << 16) |
		((unsigned)pci->dev << 11) |
		((unsigned)pci->fn << 8) |
		(reg & ~3));
	outportb(PCI_DATA_REG + (reg & 3), val);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int pci_write_config_word(pci_t *pci, unsigned reg,
		unsigned val)
{
	outportl(PCI_ADR_REG, 0x80000000L |
		((unsigned long)pci->bus << 16) |
		((unsigned)pci->dev << 11) |
		((unsigned)pci->fn << 8) |
		(reg & ~3));
	outportw(PCI_DATA_REG + (reg & 2), val);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int pci_write_config_dword(pci_t *pci, unsigned reg,
		unsigned long val)
{
	outportl(PCI_ADR_REG, 0x80000000L |
		((unsigned long)pci->bus << 16) |
		((unsigned)pci->dev << 11) |
		((unsigned)pci->fn << 8) |
		(reg & ~3));
	outportl(PCI_DATA_REG + 0, val);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int pci_iterate(pci_t *pci)
{
	unsigned char hdr_type = 0x80;

/* if first function of this device, check if multi-function device
(otherwise fn==0 is the _only_ function of this device) */
	if(pci->fn == 0)
	{
/* 0x0E=PCI_HEADER_TYPE (byte) */
		if(pci_read_config_byte(pci, 0x0E, &hdr_type))
			return -1;	/* error */
	}
/* increment iterators
fn (function) is the least significant, bus is the most significant */
	pci->fn++;
	if(pci->fn >= 8 || (hdr_type & 0x80) == 0)
	{
		pci->fn = 0;
		pci->dev++;
		if(pci->dev >= 32)
		{
			pci->dev = 0;
			pci->bus++;
//			if(pci->bus > g_last_pci_bus)
			if(pci->bus > 7)
				return 1; /* done */
		}
	}
	return 0;
}
/*****************************************************************************
"class" is a C++ reserved word
*****************************************************************************/
static void display_device_class(unsigned _class, unsigned subclass)
{
	static const char *name1[] =
	{
		"SCSI", "IDE", "floppy", "IPI", "RAID"
	};
	static const char *name2[] =
	{
		"Ethernet", "Token Ring", "FDDI", "ATM"
	};
	static const char *name3[] =
	{
		"VGA", "SuperVGA", "XGA"
	};
	static const char *name4[] =
	{
		"video", "audio"
	};
	static const char *name5[] =
	{
		"RAM", "Flash"
	};
	static const char *name6[] =
	{
		"CPU", "ISA", "EISA", "MicroChannel", "PCI", "PCMCIA",
		"NuBus", "CardBus"
	};
	static const char *name7[] =
	{
		"PC serial", "PC parallel"
	};
	static const char *name8[] =
	{
		"8259 PIC", "8237 DMAC", "8254 PTC", "MC146818 RTC"
	};
	static const char *name9[] =
	{
		"keybaord", "digitizer/pen", "mouse"
	};
	static const char *name10[] =
	{
		"generic"
	};
	static const char *name11[] =
	{
		"386", "486", "Pentium", "Pentium Pro (P6)"
/* 0x10=DEC Alpha; 0x40=coprocessor */
	};
	static const char *name12[] =
	{
		"Firewire (IEEE 1394)", "ACCESS.bus", "SSA", "USB",
		"Fiber Channel"
	};
/**/

	switch(_class)
	{
	case 1:
		printf("disk controller");
		if(subclass >= sizeof(name1) / sizeof(name1[0]))
ERR:		{
			printf(":unknown (subclass=%u)", subclass);
			return;
		}
		printf(":%s", name1[subclass]);
		break;
	case 2:
		printf("network controller");
		if(subclass >= sizeof(name2) / sizeof(name2[0]))
			goto ERR;
		printf(":%s", name2[subclass]);
		break;
	case 3:
		printf("display controller");
		if(subclass >= sizeof(name3) / sizeof(name3[0]))
			goto ERR;
		printf(":%s", name3[subclass]);
		break;
	case 4:
		printf("multimedia controller");
		if(subclass >= sizeof(name4) / sizeof(name4[0]))
			goto ERR;
		printf(":%s", name4[subclass]);
		break;
	case 5:
		printf("memory");
		if(subclass >= sizeof(name5) / sizeof(name5[0]))
			goto ERR;
		printf(":%s", name5[subclass]);
		break;
	case 6:
		printf("bridge");
		if(subclass >= sizeof(name6) / sizeof(name6[0]))
			goto ERR;
		printf(":%s", name6[subclass]);
		break;
	case 7:
		printf("communications device");
		if(subclass >= sizeof(name7) / sizeof(name7[0]))
			goto ERR;
		printf(":%s", name7[subclass]);
		break;
	case 8:
		printf("system device");
		if(subclass >= sizeof(name8) / sizeof(name8[0]))
			goto ERR;
		printf(":%s", name8[subclass]);
		break;
	case 9:
		printf("HID");
		if(subclass >= sizeof(name9) / sizeof(name9[0]))
			goto ERR;
		printf(":%s", name9[subclass]);
		break;
	case 10:
		printf("dock");
		if(subclass >= sizeof(name10) / sizeof(name10[0]))
			goto ERR;
		printf(":%s", name10[subclass]);
		break;
	case 11:
		printf("CPU");
		if(subclass >= sizeof(name11) / sizeof(name11[0]))
			goto ERR;
		printf(":%s", name11[subclass]);
		break;
	case 12:
		printf("serial bus controller");
		if(subclass >= sizeof(name12) / sizeof(name12[0]))
			goto ERR;
		printf(":%s", name12[subclass]);
		break;
	default:
		printf("unknown (class=%u)", _class);
		break;
	}
}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	unsigned long i;
	pci_t pci;
	int err;

/* check for PCI BIOS */
	if(pci_detect())
		return 1;
	printf(	"Bus Device Function VendorID DeviceID Class:Subclass\n"
		"--- ------ -------- -------- -------- ------------------\n");
/* display numeric ID of all PCI devices detected */
	memset(&pci, 0, sizeof(pci));
	do
	{
		unsigned _class, subclass;

/* 00=PCI_VENDOR_ID (word), 02=PCI_DEVICE_ID (word) */
		err = pci_read_config_dword(&pci, 0x00, &i);
		if(err)
ERR:		{
			printf("Error 0x%02X reading PCI config\n", err);
			return 1;
		}
/* anything there? */
		if(i != 0xFFFFFFFFL)
		{
			printf("%3u %6u %8u %8lX %-8lX ", pci.bus,
				pci.dev, pci.fn, i & 0xFFFF, i >> 16);
/* 09=PCI_PROGRAMMING_INTERFACE, 0A=PCI_SUBCLASS, 0B=PCI_CLASS */
			err = pci_read_config_dword(&pci, 0x08, &i);
			if(err)
				goto ERR;
			_class = (unsigned)((i >> 24) & 0xFF);
			subclass = (unsigned)((i >> 16) & 0xFF);
			display_device_class(_class, subclass);
			putchar('\n');

if(_class == 12 && subclass == 3)
{
 unsigned pi;

 printf("USB controller detected; programming interface=");
 pi = (unsigned)((i >> 8) & 0xFF);
 if(pi == 0)
  printf("UHCI\n");
 else if(pi == 0x10)
  printf("OHCI\n");
 else if(pi == 0x20)
  printf("EHCI\n");
 else
  printf("unknown (0x%02X)\n", pi);
}
		}
	} while(!pci_iterate(&pci));
	return 0;
}
