/*----------------------------------------------------------------------------
USB demo code for Turbo C
Chris Giese     <geezer@execpc.com>     http://my.execpc.com/~geezer
Release date: March 28, 2007
This code is public domain (no copyright).
You can do whatever you want with it.

To do:
>- Support multiple USB devices connected to the port(s)
- Experiment with USB devices other than the camera

- In usb_display_string(), the first call to usb_control_transaction()
  returns bogus data (data is one byte -- too short?)

- uhci_control_transaction() needs a timeout (why? was it naughty? :)

- Abort if nothing connected to the USB ports
- Support hubs
- Support hotplug

- Doesn't work with 16-bit Watcom C
  (enumeration completes, but program freezes after main() returns)
- Doesn't work with 32-bit Watcom C
- Doesn't work with DJGPP

- Get rid of random delay() calls
- Support multiple USB controller chips?
- Support OHCI and EHCI
----------------------------------------------------------------------------*/
#if 1
#define	DEBUG(X)	X
#else
#define	DEBUG(X)	/* nothing */
#endif

#include <stdio.h> /* printf() */

#if defined(__TURBOC__)
#include <dos.h> /* FP_SEG(), FP_OFF(), MK_FP(), inport[b](), outport[b]() */
#define	FAR		far
#define	PTR2LINEAR(P)	(FP_SEG(P) * 16uL + FP_OFF(P))
#define	LINEAR2PTR(L)	MK_FP((unsigned)(L >> 4), (unsigned)(L & 0x0F))
#define	inportw(P)	inport(P)
#define	outportw(P,V)	outport(P,V)
/* [in|out]portl() defined above */

#elif defined(__WATCOMC__)&&!defined(__386__)
#include <dos.h> /* FP_SEG(), FP_OFF(), MK_FP() */
#include <conio.h> /* inp[w](), outp[w]() */
#define	FAR		far
#define	PTR2LINEAR(P)	(FP_SEG(P) * 16uL + FP_OFF(P))
#define	LINEAR2PTR(L)	MK_FP((unsigned)(L >> 4), (unsigned)(L & 0x0F))
#define	inportb(P)	inp(P)
#define	inportw(P)	inpw(P)
#define	outportb(P,V)	outp(P,V)
#define	outportw(P,V)	outpw(P,V)
/* [in|out]portl() defined above */

#elif defined(__WATCOMC__)&&defined(__386__)
#include <conio.h> /* inp[w](), outp[w]() */
#define	FAR		/* nothing */
#define	PTR2LINEAR(P)	((unsigned)(P)) /* CauseWay DOS extender only? */
#define	LINEAR2PTR(L)	((void *)(L)) /* CauseWay DOS extender only? */
#define	inportb(P)	inp(P)
#define	inportw(P)	inpw(P)
#define	outportb(P,V)	outp(P,V)
#define	outportw(P,V)	outpw(P,V)
#define	inportl(P)	inpd(P)
#define	outportl(P,V)	outpd(P,V)

#elif defined(__DJGPP__)
/* __djgpp_conventional_base, __djgpp_nearptr_enable() */
#include <sys/nearptr.h>
#include <crt0.h> /* _CRT0_FLAG_..., _crt0_startup_flags */
#include <dos.h> /* inport[b|w|l](), outport[b|w|l]() */
#define	FAR		/* nothing */
#define	PTR2LINEAR(P)	((unsigned)(P) - __djgpp_conventional_base)
#define	LINEAR2PTR(L)	((void *)((unsigned)(L) + __djgpp_conventional_base))

/* disable virtual memory: */
int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;

#else
#error Sorry, unsupported compiler
#endif

#define MIN(X, Y)	(((X) < (Y)) ? (X) : (Y))

static int g_debug;
/*****************************************************************************
*****************************************************************************/
#define BPERL		16	/* byte/line for dump */

void dump(void FAR *data_p, unsigned count)
{
	unsigned char FAR *data = (unsigned char FAR *)data_p;
	unsigned byte1, byte2;

	while(count != 0)
	{
		printf("    ");
		for(byte1 = 0; byte1 < BPERL; byte1++)
		{
			if(count == 0)
				break;
			printf("%02X ", data[byte1]);
			count--;
		}
		printf("\t");
		for(byte2 = 0; byte2 < byte1; byte2++)
		{
			if(data[byte2] < ' ')
				printf(".");
			else
				printf("%c", data[byte2]);
		}
		printf("\n");
		data += BPERL;
	}
}
/*****************************************************************************
*****************************************************************************/
unsigned read_le16(void *buf_ptr)
{
	unsigned char *buf = (unsigned char *)buf_ptr;

	return buf[0] + 0x100 * (unsigned)buf[1];
}
/*****************************************************************************
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

unsigned long far cdecl (*inportl)(unsigned port) =
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

void far cdecl (*outportl)(unsigned port, unsigned long val) =
	(void far (*)(unsigned, unsigned long))g_outportl;
#endif
/*----------------------------------------------------------------------------
PCI CODE

EXPORTS:
int pci_detect(void);
int pci_read_config_byte(pci_t *pci, unsigned reg, unsigned char *val);
int pci_read_config_word(pci_t *pci, unsigned reg, unsigned short *val);
int pci_read_config_dword (pci_t *pci, unsigned reg, unsigned long *val);
int pci_write_config_byte(pci_t *pci, unsigned reg, unsigned val);
int pci_write_config_word(pci_t *pci, unsigned reg, unsigned val);
int pci_write_config_dword(pci_t *pci, unsigned reg, unsigned long val);
int pci_iterate(pci_t *pci);
----------------------------------------------------------------------------*/
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
If you have PnP code available, device PNP0A03 also
indicates the presence of a PCI controller in the system.
*****************************************************************************/
int pci_detect(void)
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
int pci_read_config_byte(pci_t *pci, unsigned reg,
		unsigned char *val)
{
	outportl(PCI_ADR_REG,
		0x80000000L | /* "enable configuration space mapping" */
		((unsigned long)pci->bus << 16) |	/* b23-b16=bus */
		((unsigned)pci->dev << 11) |		/* b15-b11=dev */
		((unsigned)pci->fn << 8) |		/* b10-b8 =fn  */
		(reg & ~3));				/* b7 -b2 =reg */
	*val = inportb(PCI_DATA_REG + (reg & 3));// ### - is this legit?
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int pci_read_config_word(pci_t *pci, unsigned reg,
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
int pci_read_config_dword (pci_t *pci, unsigned reg,
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
int pci_write_config_byte(pci_t *pci, unsigned reg,
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
int pci_write_config_word(pci_t *pci, unsigned reg,
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
int pci_write_config_dword(pci_t *pci, unsigned reg,
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
int pci_iterate(pci_t *pci)
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
/*----------------------------------------------------------------------------
UHCI CODE

EXPORTS:
int uhci_detect(void);
int uhci_init(void);
void uhci_display_port_status(void);
int uhci_control_transaction(unsigned adr,
		unsigned endpoint0_max_packet_size, void FAR *setup_data_ptr,
		unsigned setup_len, void FAR *data_ptr, unsigned data_len);
----------------------------------------------------------------------------*/
#include <stdlib.h> /* atexit(), calloc(), free() */
#include <string.h> /* memset() */
#if 0
#include <stdint.h>
#else
typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
#endif

/* packet IDs. If the actual packet ID value is 'pid', these values are
	((~pid << 4) & 0xF0) | (pid & 0x0F)
The other 14 PID types are generated and managed only by the hardware. */
#define	USB_PID_OUT		0xE1
#define	USB_PID_IN		0x69
#define	USB_PID_SETUP		0x2D

/* standard USB requests */
#define	USB_REQ_GET_STATUS	0
#define	USB_REQ_CLEAR_FEATURE	1
//
#define	USB_REQ_SET_FEATURE	3
//
#define	USB_REQ_SET_ADDRESS	5
#define	USB_REQ_GET_DESCRIPTOR	6
#define	USB_REQ_SET_DESCRIPTOR	7
#define	USB_REQ_GET_CONFIG	8
#define	USB_REQ_SET_CONFIG	9
#define	USB_REQ_GET_INTERFACE	10
#define	USB_REQ_SET_INTERFACE	11
#define	USB_REQ_SYNCH_FRAME	12

#define	TD_DWORD2(PID, ADR, ENDP, D, MAX_LEN)			\
	 (((unsigned long)(PID)		 &  0xFF) <<  0) |	\
	 (((unsigned long)(ADR)		 &  0x7F) <<  8) |	\
	 (((unsigned long)(ENDP)	 &  0x0F) << 15) |	\
	 (((unsigned long)(D)		 &  0x01) << 19) |	\
	((((unsigned long)(MAX_LEN) - 1) & 0x7FF) << 21)

/* 32-byte UHCI transfer descriptor (TD) */
#pragma pack(1)
typedef struct
{
/* bytes 0-3 */
	uint32_t link;
/* bytes 4-7
b15-b11=reserved, b10-b0=(actual data length - 1) */
	uint16_t act_len;
/* error-related status bits. b7=Active.
I was using bitfields here, but Watcom C didn't work with those. */
	volatile uint8_t status;
/* various other bits. b4-b3=retry count */
	uint8_t flags;
/* bytes 8-11. Lotta stuff here:
b21-b3=max_len, b20=reserved, b19=DATA1, b18-b15=endpoint,
b14-b8=device address, b7-b0=packet ID */
	uint32_t dword2;
/* bytes 12-15 */
	uint32_t buf_ptr;	/* linear address */
/* bytes 16-31. YES, you need these -- even though they're not used. */
	char res[16];
} td_t;

/* 8-byte UHCI queue head (QH) */
typedef struct
{
/* bytes 0-3: pointer to next horizontal object in schedule
b3-b2=reserved, b1=next object is QH, b0=Terminate  */
	uint32_t h;
/* bytes 4-7: pointer to next vertical object in schedule
b3-b2=reserved, b1=next object is QH, b0=Terminate */
	uint32_t v;
} qh_t;

static unsigned g_io_adr;
static uint32_t FAR *g_queue_head;
/*****************************************************************************
*****************************************************************************/
int uhci_detect(void)
{
	uint32_t i, j;
	uint8_t k;
	pci_t pci;
	int err;

/* check for PCI BIOS */
	if(pci_detect())
		return -1;
/* find a USB controller */
	printf("USB controller...");
	memset(&pci, 0, sizeof(pci));
	do
	{
/* pci_byte[8]=revision ID, pci_byte[9]=programming interface
(registers), pci_byte[10]=sub-class, pci_byte[11]=class */
		err = pci_read_config_dword(&pci, 0x08, &j);
		if(err)
ERR:		{
			printf("Error 0x%02X reading PCI config\n", err);
			return -1;
		}
/* class=12=0x0C=serial bus controller; sub-class=3=0x03=USB */
		if((j & 0xFFFF0000L) == 0x0C030000L)
			goto OK;
	} while(!pci_iterate(&pci));
	printf("not found\n");
	return -1;
OK:
	printf("found ");
/* 00=PCI_VENDOR_ID */
err = pci_read_config_dword(&pci, 0x00, &i);
if(err)
 goto ERR;
printf(" (vendorID=%04lX, deviceID=%04lX", i & 0xFFFF, i >> 16);
/* interface: 0x00=UHCI, 0x10=OHCI, 0x20=EHCI (USB 2.x),
0x80=other HCD, 0xFE=not an HCD */
	k = (j >> 8) & 0xFF;
	printf(", interface=%02X)\n", k);
	if(k != 0)
	{
		printf("Error: this code supports only UHCI USB\n");
		return -1;
	}
/* get I/O address by reading base address register (BAR) 4 */
	err = pci_read_config_dword(&pci, 0x20, &i);
	if(err)
		goto ERR;
/* b0 indicates I/O-mapped I/O */
	i &= ~0x01L;
	printf("\tI/O adr=0x%lX\n", i);
	g_io_adr = (unsigned)i;
/* USB spec version.
### - do I need this? to tell USB 1.x from USB 2.x? */
err = pci_read_config_byte(&pci, 0x60, &k);
if(err)
 goto ERR;
printf("\tUSB version %u.%u\n", k >> 4, k & 0x0F);

/* clear Master Abort; since I keep getting it */
//err = pci_write_config_word(&pci, 0x06, 0x2000);
//if(err)
// goto ERR;

/* dump command and status registers */
//err = pci_read_config_dword(&pci, 0x04, &i);
//if(err)
// goto ERR;
/* want bus-master enabled
0x0005: b0=I/O enabled, b2=bus master enabled */
//printf("PCI command register=%04lX\n", i & 0xFFFF);
/* 0x2280: b7=fast back-to-back capable, b9=medium device select timing,
b13=received master abort */
//printf("PCI  status register=%04lX\n", i >> 16);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static void uhci_exit(void)
{
/* USBCMD register - shutdown controller */
	outportw(g_io_adr + 0, 0x0000);
}
/*****************************************************************************
*****************************************************************************/
int uhci_init(void)
{
/* 1024 Frame List (FL) entries * 4 bytes each = 4096 bytes
plus padding for memory alignment to 4K boundary */
	static char fl_buf[1024 * 4 + 4095];
/* 2 Queue Heads (QHs) * 8 bytes each = 16 bytes
plus padding for memory alignment to 16-byte boundary */
	static char qh_buf[2 * sizeof(qh_t) + 15];
/* 1 Transfer Descriptor (TD) * 32 bytes each = 32 bytes
plus padding for memory alignment to 16-byte boundary */
	static char td_buf[1 * sizeof(td_t) + 15];
/**/
	uint32_t td_linear, qh_linear, fl_linear, FAR *fl;
	td_t FAR *td;
	qh_t FAR *qh;
	unsigned i;

/* init Transfer Descriptor (TD) */
	memset(td_buf, 0, sizeof(td_buf));	/* all fields =0 by default */
	td_linear = PTR2LINEAR(td_buf);
	td_linear = (td_linear + 15) & ~15L; 	/* align to 16-byte boundary */
	td = (td_t FAR *)LINEAR2PTR(td_linear);
/* this is a dummy TD to stop the PIIX USB controller from going berserk */
	td[0].link = 0x00000001L;		/* b0=1: terminate */
/* (all other fields =0) */

/* init Queue Heads (QHs) */
	qh_linear = PTR2LINEAR(qh_buf);
	qh_linear = (qh_linear + 15) & ~15L;	/* align to 16-byte boundary */
	qh = (qh_t FAR *)LINEAR2PTR(qh_linear);
/* horizontal chain: QH[0] --> QH[1] */
	qh[0].h = (qh_linear +
		sizeof(qh_t) * 1) | 0x00000002L;/* b1=1: points to QH */
/* first vertical chain: nothing yet, but save address.
We'll put a pointer to a chain of TDs here later. */
	qh[0].v  = 0x00000001L;			/* b0=1: terminate */
	g_queue_head = &qh[0].v;
/* second vertical chain: QH[1] --> dummy TD */
	qh[1].h = 0x00000001L;			/* b0=1: terminate */
	qh[1].v  = td_linear;

/* init Frame List (FL) */
	fl_linear = PTR2LINEAR(fl_buf);
	fl_linear = (fl_linear + 4095) & ~4095L; /* align to 4K boundary */
	fl = (uint32_t FAR *)LINEAR2PTR(fl_linear);
	for(i = 0; i < 1024; i++)
		fl[i] = qh_linear | 0x00000002L;/* b1=1: points to a QH */

/* debug dumps */
printf("Linear address of Frame List (FL)=0x%lX\n", fl_linear);
printf("\tFL[n]=0x%lX\n", fl[0]);
/* FL[0] should point to QH array */
printf("Linear address of Queue Heads (QHs)=0x%lX\n", qh_linear);
printf("\tQH[0].h=0x%08lX, QH[1].h=0x%08lX\n", qh[0].h, qh[1].h);
printf("\tQH[0].v=0x%08lX, QH[1].v=0x%08lX\n", qh[0].v, qh[1].v);
/* QH[1].v should point to dummy TD */
printf("Linear address of dummy Transfer Descriptor (TD)=0x%lX\n", td_linear);

/* reset USB host controller chip */
printf("Resetting USB controller...\n");
	outportw(g_io_adr + 0, 0x0004); 	/* b2=1: RESET */
	delay(10);
	outportw(g_io_adr + 0, 0x0000);
delay(100);
/* PORT 1 & 2 STATUS/CONTROL REGISTERS - reset (xxx...what?) */
	outportw(g_io_adr + 16, (inportw(g_io_adr + 16) & 0x1DFF) | 0x000E);
	outportw(g_io_adr + 18, (inportw(g_io_adr + 18) & 0x1DFF) | 0x000E);
delay(100);
/* SOFMOD register - USB frame rate fine-tuning */
	outportb(g_io_adr + 12, 0x40);
/* FRBASEADD register - write linear address of Frame List */
	outportl(g_io_adr + 8, fl_linear);
/* FRNUM register - zero the Frame Number counter */
	outportw(g_io_adr + 6, 0x0000);
/* USBCMD register - set b0 to start controller; i.e. GO! */
	outportw(g_io_adr + 0, 0x0001);
/* stop controller when this program exits */
	atexit(uhci_exit);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
void uhci_display_port_status(void)
{
	char comma;
	int i;

//	outportw(g_io_adr + 16, 0x00FF);
	i = inportw(g_io_adr + 16) & ~0x0030;
	printf("\tUSB port 1 status=0x%02X:", i);
	comma = ' ';
	if(i & 0x1000)
	{
		printf("%c suspended", comma);
		comma = ',';
	}
	if(i & 0x0200)
	{
		printf("%c reset", comma);
		comma = ',';
	}
	if(i & 0x0100)
	{
		printf("%c low-speed device attached", comma);
		comma = ',';
	}
	if(i & 0x0004)
	{
		printf("%c enabled", comma);
		comma = ',';
	}
	if(i & 0x0001)
		printf("%c device connected", comma);

//	outportw(g_io_adr + 18, 0x00FF);
	i = inportw(g_io_adr + 18) & ~0x0030;
	printf("\n\tUSB port 2 status=0x%02X:", i);
	comma = ' ';
	if(i & 0x1000)
	{
		printf("%c suspended", comma);
		comma = ',';
	}
	if(i & 0x0200)
	{
		printf("%c reset", comma);
		comma = ',';
	}
	if(i & 0x0100)
	{
		printf("%c low-speed device attached", comma);
		comma = ',';
	}
	if(i & 0x0004)
	{
		printf("%c enabled", comma);
		comma = ',';
	}
	if(i & 0x0001)
		printf("%c device connected", comma);
	putchar('\n');
}
/*****************************************************************************
*****************************************************************************/
#include <conio.h>

int uhci_control_transaction(unsigned adr,
		unsigned endpoint0_max_packet_size, void FAR *setup_data_ptr,
		unsigned setup_len, void FAR *data_ptr, unsigned data_len)
{
/* endpoint is always #0 for control transactions */
	static const unsigned endp = 0;
/**/
	unsigned char FAR *setup_data = setup_data_ptr;
	unsigned char FAR *data = data_ptr;
	char data1, dir, *td_buf;
	unsigned num_tds, i, j;
	uint32_t td_linear;
	td_t FAR *td;
	int err;

delay(100);
/* total number of TDs required */
	num_tds = (data_len + endpoint0_max_packet_size - 1) /
		endpoint0_max_packet_size + 2;
/* allocate zeroed memory for TDs: 32 bytes each
plus padding for memory alignment to 16-byte boundary */
	td_buf = calloc(num_tds * sizeof(td_t) + 15, 1);
	if(td_buf == NULL)
	{
		printf("Error: out of memory\n");
		return -1;
	}
	td_linear = PTR2LINEAR(td_buf);
	td_linear = (td_linear + 15) & ~15L; 	/* align to 16-byte boundary */
	td = (td_t FAR *)LINEAR2PTR(td_linear);
/* build token TD
dword #0 */
	td[0].link = (td_linear +		/* b2=1: depth-first */
		sizeof(td_t) * 1) | 0x00000004L;
/* dword #1 (status) */
	td[0].act_len = setup_len - 1;
	td[0].status = 0x80;			/* b7=1: Active */
	td[0].flags = 0x18;			/* b4-b3=3: retry count */
/* dword #2 (token/destination) */
	td[0].dword2 =TD_DWORD2(USB_PID_SETUP, adr, endp, 0, setup_len);
/* dword #3 (buffer) */
	td[0].buf_ptr = PTR2LINEAR(setup_data);
	DEBUG(
		if(g_debug)
		{
			printf("Linear address of setup packet=0x%lX\n",
				PTR2LINEAR(setup_data));
			printf("    Hex dump of setup packet:\n");
			dump(setup_data, setup_len);
		}
	)
/* build data TDs */
	data1 = 1;
	dir = (setup_data[0] & 0x80) ? USB_PID_IN : USB_PID_OUT;
	for(i = 1; i < num_tds - 1; i++)
	{
		td[i].link = (td_linear +	/* b2=1: depth-first */
			sizeof(td_t) * (i + 1)) | 0x00000004L;
		j = MIN(data_len, endpoint0_max_packet_size);
		td[i].act_len = j - 1;
		td[i].status = 0x80;		/* b7=1: Active */
		td[i].flags = 0x18;		/* b4-b3=3: retry count */
		td[i].dword2 = TD_DWORD2(dir, adr, endp, data1, j);
		td[i].buf_ptr = PTR2LINEAR(data);

		data_len -= j;
// xxx - if you're going to be doing math on this pointer,
// then FAR should probably be declared as 'huge'
		data += j;
		data1 = data1 ? 0 : 1;
	}
/* build handshake TD */
	dir = (dir == USB_PID_IN) ? USB_PID_OUT : USB_PID_IN;
	td[num_tds - 1].link = 0x00000001L;	/* b0=1: terminate */
	td[num_tds - 1].act_len = 0x7FF;	/* =0 (no data) */
	td[num_tds - 1].status = 0x80;		/* b7=1: Active */
	td[num_tds - 1].flags = 0x18;		/* b4-b3=3: retry count */
	td[num_tds - 1].dword2 = TD_DWORD2(dir, adr, endp, 1, 0);
	DEBUG(
		if(g_debug)
		{
			printf("Linear address of TDs=0x%lX\n", td_linear);
			printf("    Hex dump of TDs:\n");
			for(i = 0; i < num_tds; i++)
				dump(&td[i], 16);
		}
	)
/* add our chain of TDs to the Queue Head */
	*g_queue_head = td_linear;
	err = 0;
	for(i = 0; i < num_tds; )
	{
		if(td[i].status & 0x80)
			continue;
		if(td[i].status != 0)
		{
			printf("UHCI control transfer failed (descriptor "
				"#%u, status=0x%02X)\n", i, td[i].status);
			//free(td_buf);
			//return -1;
			err = 1;
			break;
		}
		i++;
	}
	DEBUG(
		if(g_debug)
		{
			printf("    Hex dump of TDs:\n");
			for(i = 0; i < num_tds; i++)
				dump(&td[i], 16);
		}
	)
	free(td_buf);
	return err;
}
/*----------------------------------------------------------------------------
USB CODE

EXPORTS:
void usb_destroy_dev(usb_dev_t *dev);
int usb_get_descriptor(usb_dev_t *dev, unsigned desc_type,
		unsigned desc_index, unsigned desc_len, void FAR *data);
int usb_set_address(usb_dev_t *dev, unsigned adr);
void usb_dump_device(usb_dev_t *dev);
void usb_dump_config(usb_dev_t *dev, unsigned config_num);
void usb_dump_iface(usb_dev_t *dev, unsigned config_num, unsigned iface_num);
void usb_dump_endp(usb_dev_t *dev, unsigned config_num, unsigned iface_num,
		unsigned endp_num);

IMPORTS:
int uhci_control_transaction(unsigned adr,
		unsigned endpoint0_max_packet_size, void FAR *setup_data_ptr,
		unsigned setup_len, void FAR *data_ptr, unsigned data_len);
----------------------------------------------------------------------------*/
#include <stdlib.h> /* free() */
#include <string.h> /* memset() */

/* descriptor types */
#define	USB_DTYPE_DEVICE	1
#define	USB_DTYPE_CONFIG	2
#define	USB_DTYPE_STRING	3
#define	USB_DTYPE_INTERFACE	4 /* "Not directly accessible" */
#define	USB_DTYPE_ENDPOINT	5 /* "Not directly accessible" */
#define	USB_DTYPE_QUALIFIER	6 /* high-speed USB only */
#define	USB_DTYPE_SPEED_CONFIG	7 /* high-speed USB only */
#define	USB_DTYPE_IFACE_POWER	8 /* obsolete */
#define	USB_DTYPE_OTG		9 /* On-The-Go; not directly accessible */

typedef struct
{						/* descriptor offset: */
/* b7=IN/!OUT, b3-b0=endpoint number */
	unsigned char adr;				/* 2 */
	unsigned char attrib;				/* 3 */
	unsigned max_packet_size;			/* 4-5 */
	unsigned polling_interval; /* usec */		/* 6 */
} usb_endp_t;

typedef struct
{						/* descriptor offset: */
	unsigned char iface_num;			/* 2 */
	unsigned char alt;	/* ? */			/* 3 */
	unsigned char num_endp;				/* 4 */
/* "class" is a C++ reserved word */
	unsigned char _class, subclass, proto;		/* 5,6,7 */
	unsigned char iface_string;			/* 8 */
/* endpoints for this interface */
	usb_endp_t *endp;
} usb_iface_t;

typedef struct
{						/* descriptor offset: */
	unsigned char num_ifs;				/* 4 */
	unsigned char config_num;			/* 5 */
	unsigned char config_string;			/* 6 */
	unsigned char attrib;				/* 7 */
	unsigned max_current;	/* in mA */		/* 8 */
/* interfaces for this config */
	usb_iface_t *iface;
} usb_config_t;

typedef struct
{						/* descriptor offset: */
	unsigned char adr; /* assigned by this code */
	unsigned char usb_ver_major, usb_ver_minor;	/* 2,3 */
	unsigned char _class, subclass, proto;		/* 4,5,6 */
	unsigned char endpoint0_max_packet_size;	/* 7 */
	unsigned vendor_id, product_id;			/* 8-9,10-11 */
	unsigned char mfgr_string;			/* 14 */
	unsigned char prod_string;			/* 15 */
	unsigned char sernum_string; 			/* 16 */
	unsigned num_configs;				/* 17 */
/* configurations for this device */
	usb_config_t *configs;
} usb_dev_t;
/*****************************************************************************
*****************************************************************************/
void usb_destroy_dev(usb_dev_t *dev)
{
	unsigned c_num, i_num;
	usb_config_t *c;
	usb_iface_t *i;

	if(dev->configs != NULL && dev->num_configs != 0)
	{
/* for each config in device... */
		for(c_num = 0; c_num < dev->num_configs; c_num++)
		{
			c = &dev->configs[c_num];
			if(c->iface != NULL && c->num_ifs != 0)
			{
/* for each interface in config... */
				for(i_num = 0; i_num < c->num_ifs; i_num++)
				{
					i = &c->iface[i_num];
/* free endpoints */
					if(i->endp != NULL)
						free(i->endp);
				}
/* free interfaces */
				free(c->iface);
			}
		}
/* free configs */
		free(dev->configs);
	}
	memset(&dev, 0, sizeof(usb_dev_t));
}
/*****************************************************************************
*****************************************************************************/
static int usb_control_transaction(usb_dev_t *dev, void FAR *setup_data_ptr,
		unsigned setup_len, void FAR *data_ptr, unsigned data_len)
{
	return uhci_control_transaction(dev->adr,
		dev->endpoint0_max_packet_size, setup_data_ptr,
		setup_len, data_ptr, data_len);
}
/*****************************************************************************
This function will fail if desc_type==USB_DTYPE_INTERFACE or desc_type==
USB_DTYPE_ENDPOINT ("USB Made Simple" calls them "Not directly accessible").
To get those descriptors:
1. Load the config descriptor
2. Look at bytes 2-3 of the config descriptor. This is the combined
   length of the config descriptor and its subordinate interface and
   endpoint descriptors.
3. Load this entire mass of descriptors (config, interface, and
   endpoint) into memory, then pick out what you need
*****************************************************************************/
static int usb_get_descriptor(usb_dev_t *dev, unsigned desc_type,
		unsigned desc_index, unsigned desc_len, void FAR *data)
{
	char setup[8];

	setup[0] = 0x80;
	setup[1] = USB_REQ_GET_DESCRIPTOR;/* bRequest, =6 */
	setup[2] = desc_index; 		/* wValue LSB: descriptor index */
	  setup[3] = desc_type;		/* wValue MSB: descriptor type */
	setup[4] = setup[5] = 0;	/* wIndex: zero or language ID */
	setup[6] = desc_len;		/* wLength: descriptor length */
	  setup[7] = 0;
	return usb_control_transaction(dev,
		setup, sizeof(setup), data, desc_len);
}
/*****************************************************************************
*****************************************************************************/
static int usb_set_address(usb_dev_t *dev, unsigned adr)
{
	char setup[8];
	int i;

	setup[0] = 0;
	setup[1] = USB_REQ_SET_ADDRESS;	/* bRequest, =5 */
	setup[2] = adr;			/* wValue */
	 setup[3] = 0;
	setup[4] = setup[5] = 0;	/* wIndex */
	setup[6] = setup[7] = 0;	/* wLength */
	i = usb_control_transaction(dev,
		setup, sizeof(setup), 0, 0);
	if(i == 0)
		dev->adr = adr;
	return i;
}
/*****************************************************************************
*****************************************************************************/
static int usb_display_string(usb_dev_t *dev, unsigned string_num)
{
	unsigned char setup[8], data[256];
	unsigned len, j;
	int i;

// xxx - first byte returned (string length) on first call is 0xE6
// on the second call, it's 0x32, which is the correct value
	len = 1;
	setup[0] = 0x80;		/* bmRequestType */
	setup[1] = USB_REQ_GET_DESCRIPTOR;/* bRequest, =6 */
	setup[2] = string_num; 		/* wValue LSB: descriptor index */
	  setup[3] = USB_DTYPE_STRING;	/* wValue MSB: descriptor type, =1 */
	setup[4] = 0;			/* wIndex: zero or language ID */
	  setup[5] = 0;
	setup[6] = len;			/* wLength: descriptor length */
	  setup[7] = 0;
	i = usb_control_transaction(dev,
		setup, sizeof(setup), data, len);
	if(i != 0)
		return i;
	len = data[0];
	setup[6] = len;
	i = usb_control_transaction(dev,
		setup, sizeof(setup), data, len);
	if(i != 0)
		return i;
	len = data[0];
/* this assumes Latin-1 character set */
	for(j = 2; j < len; j += 2)
		putchar(data[j]);
	return 0;
}
/*****************************************************************************
Device descriptor
	Field		Field
Offset	name		size	Description
------	---------------	-----	-------------
0	bLength		1	Length of this device descriptor
1	bDescriptorType	1	USB_DTYPE_DEVICE=1
2-3	bcdUSB		2	USB version in BCD format, minor first
4	bDeviceClass	1
5	bDeviceSubClass	1
6	bDeviceProtocol	1
7	bMaxPacketSize0	1	Max. packet size for endpoint #0 (8,16,32,64)
8-9	idVendor	2
10-11	idProduct	2
12-13	bcdDevice	2
14	iManufacturer	1	String index
15	iProduct	1	String index
16	iSerialNumber	1	String index
17	bNumConfig.s	1
*****************************************************************************/
static void usb_dump_device(usb_dev_t *dev)
{
	printf("\nDEVICE INFO for address %u:\n", dev->adr);
	printf("USB ver %X.%X", dev->usb_ver_major, dev->usb_ver_minor);
	printf(", class:sub:proto=%u:%u:%u", dev->_class,
		dev->subclass, dev->proto);
	printf(", vendorID=0x%04X, deviceID=0x%04X\n",
		dev->vendor_id, dev->product_id);
	if(dev->mfgr_string != 0)
	{
		printf("Manufacturer: ");
		(void)usb_display_string(dev, dev->mfgr_string);
		putchar('\n');
	}
	if(dev->prod_string != 0)
	{
		printf("Product: ");
		(void)usb_display_string(dev, dev->prod_string);
		putchar('\n');
	}
	if(dev->sernum_string != 0)
	{
		printf("Serial number: ");
		(void)usb_display_string(dev, dev->sernum_string);
		putchar('\n');
	}
	printf("Device has %u configuration(s)\n", dev->num_configs);
}
/*****************************************************************************
Configuration descriptor
	Field		Field
Offset	name		size	Description
------	---------------	-----	-------------
0	bLength		1	Length of this configuration descriptor
1	bDescriptorType	1	USB_DTYPE_CONFIG=2
2-3	wTotalLength	2	Combined length of this descriptor and all
				subordinate descriptors (interface & endpoint)
4	bNumInterfaces	1
5	bConfig.Value	1	1-based
6	iConfiguration	1	String index
7	bmAttributes	1	b7=1, b6=self-powered,b5=remote wakeup
8	bMaxPower	1	MaxCurrent, actually, in units of 2 mA
*****************************************************************************/
static void usb_dump_config(usb_dev_t *dev, unsigned config_num)
{
	usb_config_t *c;

	if(config_num >= dev->num_configs)
		return;
	c = &dev->configs[config_num];
	printf("  CONFIG INFO for address %u, config %u:\n",
		dev->adr, c->config_num);
	printf("  Config has %u interface(s)", c->num_ifs);
	printf(", attributes=0x%02X", c->attrib);
	printf(", %umA max current\n", c->max_current);
	if(c->config_string != 0)
	{
		printf("  Config name: ");
		(void)usb_display_string(dev, c->config_string);
		putchar('\n');
	}
}
/*****************************************************************************
Interface descriptor
	Field		Field
Offset	name		size	Description
------	---------------	-----	-------------
0	bLength		1	Length of this interface descriptor
1	bDescriptorType	1	USB_DTYPE_INTERFACE=2
2	bInterfaceNumber 1	0-based
3	bAlternateSetting 1
4	bNumEndpoints	1
5	bInterfaceClass	1
6	bI.faceSubClass	1
7	bI.faceProtocol	1
8	iInterface	1	String index
*****************************************************************************/
static void usb_dump_iface(usb_dev_t *dev, unsigned config_num,
		unsigned iface_num)
{
	usb_config_t *c;
	usb_iface_t *i;

	if(config_num >= dev->num_configs)
		return;
	c = &dev->configs[config_num];
	if(iface_num >= c->num_ifs)
		return;
	i = &c->iface[iface_num];
	printf("    INTERFACE INFO for address %u, config %u, "
		"interface %u:\n", dev->adr, c->config_num, i->iface_num);
	printf("    Alternate setting=%u", i->alt);
	printf(", %u endpoint(s)", i->num_endp);
	printf(", class:sub:proto=%u:%u:%u\n",
		i->_class, i->subclass, i->proto);
	if(i->iface_string != 0)
	{
		printf("    Interface name: ");
		(void)usb_display_string(dev, i->iface_string);
		putchar('\n');
	}
}
/*****************************************************************************
Endpoint descriptor
	Field		Field
Offset	name		size	Description
------	---------------	-----	-------------
0	bLength		1	Length of this endpoint descriptor
1	bDescriptorType	1	USB_DTYPE_ENDPOINT=2
2	bEndpointAddress 1	b7=IN/!OUT, b3-b0=endpoint number
3	bmAttributes	1	b5-b4=usage (?), b3-b2=synchronization (?),
				b1-b0=transfer type (3=interrupt, 2=bulk,
				1=isochronous, 0=control)
4-5	wMaxPacketSize	2
6	bInterval	1	Polling interval in 1 ms frames (for low/full
				speed) or 125 us microframes (for high speed)
*****************************************************************************/
static void usb_dump_endp(usb_dev_t *dev, unsigned config_num,
		unsigned iface_num, unsigned endp_num)
{
	usb_config_t *c;
	usb_iface_t *i;
	usb_endp_t *e;

	if(config_num >= dev->num_configs)
		return;
	c = &dev->configs[config_num];
	if(iface_num >= c->num_ifs)
		return;
	i = &c->iface[iface_num];
	if(endp_num >= i->num_endp)
		return;
	e = &i->endp[endp_num];
	printf("      ENDPOINT INFO for address %u, config %u, interface "
		"%u, endpoint %u:\n", dev->adr, c->config_num, i->iface_num,
		e->adr & 0x0F);
	printf("      %s, attrib=0x%02X (",
		(e->adr & 0x80) ? "IN" : "OUT", e->attrib);
	switch(e->attrib & 0x03)
	{
	case 0:
		printf("control");
		break;
	case 1:
		printf("isochronous");
		break;
	case 2:
		printf("bulk");
		break;
	case 3:
		printf("interrupt");
		break;
	}
	printf("), max_packet_size=%u\n      polling interval=%u usec\n",
		e->max_packet_size, e->polling_interval);
}
/*****************************************************************************
*****************************************************************************/
int usb_enumerate(void)
{
	unsigned ddesc_len, cdesc_len, i, j, c_num, i_num, e_num = 0;
	unsigned char data[256], *data2;
	usb_iface_t *iface = NULL;
	usb_config_t *c = NULL;
	usb_endp_t *e;
	usb_dev_t dev;

/* temporary values */
	dev.adr = 0;
	dev.endpoint0_max_packet_size = 8;
/* get first 8 bytes of the device descriptor */
	if(usb_get_descriptor(&dev, USB_DTYPE_DEVICE, 0, 8, data))
	{
		printf("Error reading device descriptor (1)\n");
		return -1;
	}
	DEBUG(
		if(g_debug)
		{
			printf("Linear address of data buffer=0x%lX\n",
				PTR2LINEAR(&data[0]));
			printf("    Hex dump of received data:\n");
			dump(data, 8);
		}
	)
/* get what data we can from the first 8 bytes */
	ddesc_len = MIN(18,	data[0]); /* device descriptor length */
	dev.usb_ver_minor =	data[2];
	dev.usb_ver_major =	data[3];
	dev._class =		data[4];
	dev.subclass =		data[5];
	dev.proto =		data[6];
	dev.endpoint0_max_packet_size = data[7]; /* 8, 16, 32, or 64 */
/* set device address */
	i = 0x5A;
	if(usb_set_address(&dev, i))
	{
		printf("Error setting device address to %u\n", i);
		return -1;
	}
/* got endpoint0_max_packet_size; can now
get the rest of the device descriptor (up to 18 bytes) */
	if(usb_get_descriptor(&dev, USB_DTYPE_DEVICE, 0, ddesc_len, data))
	{
		printf("Error reading device descriptor (2)\n");
		return -1;
	}
	DEBUG(
		if(g_debug)
		{
			printf("    Hex dump of received data:\n");
			dump(data, ddesc_len);
		}
	)
	dev.vendor_id = read_le16(&data[8]);
	dev.product_id = read_le16(&data[10]);
	dev.mfgr_string =	data[14];
	dev.prod_string =	data[15];
	dev.sernum_string =	data[16];
	dev.num_configs =	data[17];
/* allocate memory for configs */
	dev.configs = malloc(dev.num_configs * sizeof(usb_config_t));
	if(dev.configs == NULL)
MEM:	{
		printf("Error: out of memory\n");
		return -1;
	}
/* load configs */
	for(c_num = 0; c_num < dev.num_configs; c_num++)
	{
		if(usb_get_descriptor(&dev, USB_DTYPE_CONFIG,
			c_num, 9, data))
		{
			printf("Error reading configuration descriptor (1)\n");
			return -1;
		}
		DEBUG(
			if(g_debug)
			{
				printf("    Hex dump of received data:\n");
				dump(data, 9);
			}
		)
		c = &dev.configs[c_num];
		c->num_ifs =		data[4];
		c->config_num =		data[5];
		c->config_string =	data[6];
		c->attrib =		data[7];
		c->max_current =	data[8] * 2;
/* data[2-3] contains combined length of this descriptor and all subordinate
descriptors (e.g. interface descriptors) for this configuration.
Allocate memory and load it all. */
		cdesc_len = read_le16(&data[2]);
		data2 = malloc(cdesc_len);
		if(data2 == NULL)
			goto MEM;
/* allocate memory for interfaces */
		c->iface = malloc(c->num_ifs * sizeof(usb_iface_t));
		if(c->iface == NULL)
		{
			free(data2);
			goto MEM;
		}
/* load config descriptor and associated interface, and endpoint descriptors */
		if(usb_get_descriptor(&dev, USB_DTYPE_CONFIG,
			c_num, cdesc_len, data2))
		{
			free(data2);
			usb_destroy_dev(&dev);
			printf("Error reading configuration descriptor (2)\n");
			return -1;
		}
/* find interface and endpoint descriptors and read them */
		i_num = 0;
		for(j = 0; j < cdesc_len; )
		{
			if(data2[j + 1] == USB_DTYPE_INTERFACE)
			{
				if(i_num >= c->num_ifs)
					break;
				iface = &c->iface[i_num];
				iface->iface_num =	data2[j + 2];
				iface->alt =		data2[j + 3];
				iface->num_endp =	data2[j + 4];
				iface->_class =		data2[j + 5];
				iface->subclass =	data2[j + 6];
				iface->proto =		data2[j + 7];
				iface->iface_string =	data2[j + 8];
				i_num++;
/* allocate memory for endpoints */
				iface->endp = malloc(iface->num_endp *
					sizeof(usb_endp_t));
				if(c->iface == NULL)
				{
					free(data2);
					goto MEM;
				}
				e_num = 0;
			}
			else if(data2[j + 1] == USB_DTYPE_ENDPOINT)
			{
				if(e_num >= iface->num_endp)
					break;
				e = &iface->endp[e_num];
				e->adr =		data2[j + 2];
				e->attrib =		data2[j + 3];
				e->max_packet_size =
					read_le16(&data2[j + 4]);
// xxx - x125 for high-speed USB
				e->polling_interval =
						1000 *	data2[j + 6];
				e_num++;
			}
			j += data2[j + 0];
		}
		free(data2);
	}
/* dump info */
	usb_dump_device(&dev);
	for(c_num = 0; c_num < dev.num_configs; c_num++)
	{
		usb_dump_config(&dev, c_num);
		for(i_num = 0; i_num < c->num_ifs; i_num++)
		{
			usb_dump_iface(&dev, c_num, i_num);
			for(e_num = 0; e_num < iface->num_endp; e_num++)
				usb_dump_endp(&dev, c_num, i_num, e_num);
		}
	}
/* done with this device */
	usb_destroy_dev(&dev);
	return 0;
}
/*----------------------------------------------------------------------------
IMPORTS:
int uhci_detect(void);
int uhci_init(void);
void uhci_display_port_status(void);
int usb_enumerate(void);
----------------------------------------------------------------------------*/
/*****************************************************************************
*****************************************************************************/
int main(void)
{

#if defined(__DJGPP__)
/* turn off data segment limit, for nearptr access */
	if(!(_crt0_startup_flags & _CRT0_FLAG_NEARPTR))
	{
		if(!__djgpp_nearptr_enable())
		{
			printf("Error: can't enable near pointer "
				"access (WinNT/2k/XP?)\n");
			return 1;
		}
	}
#endif
/* detect and set up UHCI controller */
	if(uhci_detect())
		return 1;
	if(uhci_init())
		return 1;
	uhci_display_port_status();
/* enumerate USB devices */
	(void)usb_enumerate();
	return 0;
}
