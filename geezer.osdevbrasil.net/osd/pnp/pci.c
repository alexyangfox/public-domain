/*****************************************************************************
PCI demo for DJGPP, Turbo C, or Watcom C, using I/O ports
(configuration mechanism 1)

Chris Giese     <geezer@execpc.com>     http://my.execpc.com/~geezer
Release date: Feb 23, 2005
This code is public domain (no copyright).
You can do whatever you want with it.
*****************************************************************************/
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
#endif
/* else [in|out]portl() defined below */

#elif defined(__TURBOC__)
#include <dos.h> /* inport[b](), outport[b]() */

#define	inportw(P)	inport(P)
#define	outportw(P,V)	outport(P,V)
/* [in|out]portl() defined below */

#else
#error Unsupported compiler. Ni!
#endif

#define PCI_READ_CONFIG_BYTE	0xB108
#define PCI_READ_CONFIG_WORD	0xB109
#define PCI_READ_CONFIG_DWORD	0xB10A
#define PCI_WRITE_CONFIG_BYTE	0xB10B
#define PCI_WRITE_CONFIG_WORD	0xB10C
#define PCI_WRITE_CONFIG_DWORD	0xB10D

#define	PCI_ADR_REG		0xCF8
#define	PCI_DATA_REG		0xCFC

typedef struct
{
	unsigned char bus, dev, fn;
} pci_t;
/*****************************************************************************
ugh...

These functions are _code_, but are stored in the _data_ segment.
Declare them 'far' and end them with 'retf' instead of 'ret'

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
If you have PnP code available, device PNP0A03 also
indicates the presence of a PCI controller in the system.
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
	*val = inportb(PCI_DATA_REG + (reg & 3));// xxx - is this legit?
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
		((unsigned)pci->fn << 8) | (reg & ~3));
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
		((unsigned)pci->fn << 8) | (reg & ~3));
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
		((unsigned)pci->fn << 8) | (reg & ~3));
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
		((unsigned)pci->fn << 8) | (reg & ~3));
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
		((unsigned)pci->fn << 8) | (reg & ~3));
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
*****************************************************************************/
int main(void)
{
	pci_t pci;
	int err;

/* check for PCI BIOS */
	if(pci_detect())
		return 1;
/* display numeric ID of all PCI devices detected */
	memset(&pci, 0, sizeof(pci));
	do
	{
		unsigned long id;

/* 00=PCI_VENDOR_ID */
		err = pci_read_config_dword(&pci, 0x00, &id);
		if(err)
ERR:		{
			printf("Error 0x%02X reading PCI config\n", err);
			return 1;
		}
/* anything there? */
		if(id != 0xFFFFFFFFL)
		{
			printf("bus %u, device %2u, function %u: "
				"device=%04lX:%04lX\n", pci.bus,
				pci.dev, pci.fn, id & 0xFFFF, id >> 16);
		}
	} while(!pci_iterate(&pci));
/* find a USB controller */
	memset(&pci, 0, sizeof(pci));
	do
	{
		unsigned char major, minor;

/* 0B=class */
		err = pci_read_config_byte(&pci, 0x0B, &major);
		if(err)
			goto ERR;
/* 0A=sub-class */
		err = pci_read_config_byte(&pci, 0x0A, &minor);
		if(err)
			goto ERR;
/* anything there? */
		if(major != 0xFF || minor != 0xFF)
		{
printf("detected device of class %u.%u\n", major, minor);
			if(major == 12 && minor == 3)
			{
				printf("USB controller detected\n");
				break;
			}
		}
	} while(!pci_iterate(&pci));
	return 0;
}

