/*----------------------------------------------------------------------------
Parallel port PnP (device ID in nibble mode)
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: March 31, 2006
This code is public domain (no copyright).
You can do whatever you want with it.

Sources:
- http://www.amontec.com/ieee1284_intro.shtml
  (which may have the polarity of some signals wrong...)

- "Extended Capabilities Port: Specifications" by Microsoft
  Revision 1.06, July 14 1993 (correct signal polarities,
  but blurry and impossible-to-read timing diagrams)

- Jan Axelson's parallel port FAQ

- Craig Peacock's "Interfacing the Standard Parallel Port"

- Trial-and-error :(




Very old parallel ports (not compatible with IEEE 1284):
- do not respond to IEEE 1284 negotiation phase
- support only nibble mode for reverse data transfer (into PC)

IEEE 1284:
- can identify device and negotiate with it to see what modes it supports
- output modes: legacy (Centronics), EPP, and ECP
- ECP supports optional DMA, FIFO, and RLE
- input modes: nibble and (sometimes) byte

Device ID strings (apparently case-insensitive):
"CLASS:" or "CLS:"		Device class, e.g. "Printer"
"MANUFACTURER:" or "MFG:"
"MODEL:" or "MDL:"
"CMD:"				Command set. Win95 ignores this.
"DES:"				Description?
"COMMENT:"			?
----------------------------------------------------------------------------*/
#include <stdio.h>
#include <dos.h>

#if defined(__WATCOMC__)
#include <conio.h>
#define	inportb(P)	inp(P)
#define	outportb(P,V)	outp(P,V)
#endif

/* In the following bit definitions, a leading 'n' means the signal is
active-low. The various sources I used were conflicted on which signals
are inverted and which ones are not, but I think this is now correct: */
#define LPT_STAT_BUSY	0x80	/* BUSY			active-HIGH */
#define LPT_STAT_ACK	0x40	/* nACK			active-low */
#define LPT_STAT_EMPTY	0x20	/* PE (paper empty)	active-HIGH */
#define LPT_STAT_SEL	0x10	/* SELECT		active-HIGH */
#define LPT_STAT_ERR	0x08	/* nERROR (nFAULT)	active-low */
//efine	LPT_STAT_IRQ	0x04	/* nIRQ			active-low /
#define	LPT_STAT_MASK	0xF8

//efine	LPT_CTRL_BIDIR	0x20	/* BIDIR		active-HIGH */
#define	LPT_CTRL_IRQEN	0x10	/* IRQEN		active-HIGH */
#define	LPT_CTRL_SEL	0x08	/* nSELECTIN		active-low */
#define	LPT_CTRL_INIT	0x04	/* INIT			active-HIGH */
#define	LPT_CTRL_FEED	0x02	/* nAUTOFEED		active-low */
#define	LPT_CTRL_STB	0x01	/* nSTROBE		active-low */
#define	LPT_CTRL_MASK	0x1F

/* bit		SPP mode	Nibble/byte mode	ECP mode	Other
-------------	-----------	----------------	---------------	-----
LPT_STAT_BUSY	Busy		PeriphBusy		PeriphAck
LPT_STAT_ACK	PeriphClk	nAck
LPT_STAT_EMPTY	?
LPT_STAT_SEL	Xflag		Select
LPT_STAT_ERR	nFault		nDataAvailable		nPeriphRequest

LPT_CTRL_IRQEN
LPT_CTRL_SEL	nSelectIn	RnW			BIOSEmode	ECPMode
LPT_CTRL_INIT	nInit		(always high)		nReverseRequest
?		PError		AckDataReq		nAckReverse
LPT_CTRL_FEED	nAutoFd		HostBusy		HostAck
LPT_CTRL_STB	HostClk		nStrobe
*/

static unsigned g_ctrl;
static const unsigned g_io_adr = 0x278;
/*****************************************************************************
xxx - delay from ISA bus read is only approximately = 1 usec
*****************************************************************************/
static void udelay(unsigned long microsec)
{
	for(; microsec != 0; microsec--)
		(void)inportb(0x80);
}
/*****************************************************************************
reset returns the port to SPP (Centronics; legacy) mode
*****************************************************************************/
static void reset_lpt(void)
{
/* reset printer by sending 0x08 byte followed by 0x0C byte */
	g_ctrl = LPT_CTRL_SEL |		/* nSELECTIN=1*/
		//!LPT_CTRL_INIT |	/*     nINIT=0: reset printer */
		LPT_CTRL_INIT |		/*      INIT=1: reset printer */
		!LPT_CTRL_FEED |	/* nAUTOFEED=0 */
		!LPT_CTRL_STB;		/*   nSTROBE=0 */
	outportb(g_io_adr + 2, g_ctrl);
// xxx - where'd this delay value come from?
	udelay(50);
	//g_ctrl |= LPT_CTRL_INIT;	/*     nINIT=1: end reset */
	g_ctrl &= ~LPT_CTRL_INIT;	/*      INIT=0: end reset */
	outportb(g_io_adr + 2, g_ctrl);
}
/*****************************************************************************
*****************************************************************************/
static int await_lpt(void)
{
	unsigned timeout;

/* wait until printer not busy
xxx - how long should this delay be? */
	for(timeout = 100; timeout != 0; timeout--)
	{
		if((inportb(g_io_adr + 1) & LPT_STAT_BUSY) == 0) /* BUSY==0 */
			break;
		delay(10);
	}
	if(timeout == 0)
	{
		printf("Error: parallel port device did not become ready\n");
		return -1;
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static void display_lpt_control(unsigned mask)
{
	if(mask & LPT_CTRL_IRQEN)
		printf("IE=%u ", (g_ctrl & LPT_CTRL_IRQEN) ? 1 : 0);
	if(mask & LPT_CTRL_SEL)
		printf("SI=%u ", (g_ctrl & LPT_CTRL_SEL) ? 1 : 0);
	if(mask & LPT_CTRL_INIT)
		printf("IP=%u ", (g_ctrl & LPT_CTRL_INIT) ? 1 : 0);
	if(mask & LPT_CTRL_FEED)
		printf("AF=%u ", (g_ctrl & LPT_CTRL_FEED) ? 1 : 0);
	if(mask & LPT_CTRL_STB)
		printf("ST=%u ", (g_ctrl & LPT_CTRL_STB) ? 1 : 0);
}
/*****************************************************************************
*****************************************************************************/
static void display_lpt_status(unsigned mask, unsigned status)
{
	if(mask & LPT_STAT_BUSY)
		printf("BUSY=%u ", (status & LPT_STAT_BUSY) ? 1 : 0);
	if(mask & LPT_STAT_ACK)
		printf("nACK=%u ", (status & LPT_STAT_ACK) ? 1 : 0);
	if(mask & LPT_STAT_EMPTY)
		printf("PE=%u ", (status & LPT_STAT_EMPTY) ? 1 : 0);
	if(mask & LPT_STAT_SEL)
		printf("SELECT=%u ", (status & LPT_STAT_SEL) ? 1 : 0);
	if(mask & LPT_STAT_ERR)
		printf("nERROR=%u", (status & LPT_STAT_ERR) ? 1 : 0);
	if(mask & LPT_STAT_MASK)
		putchar('\n');
}
/*****************************************************************************
*****************************************************************************/
static int check_lpt_status(unsigned mask, unsigned want, unsigned timeout)
{
	int got = 0;

/* timeout in milliseconds */
	for(; timeout != 0; timeout--)
	{
		got = inportb(g_io_adr + 1);
		if(((got ^ want) & mask) == 0)
			break;
		delay(1);
	}
printf("check_lpt_status: got status 0x%02X (& 0x%02X = 0x%02X)\n\t",
 got, mask, got & mask);
display_lpt_status(mask, got);
	if(timeout == 0)
	{
		printf("Error: bad parallel port status.\n\tGot:    ");
		display_lpt_status(mask, got);
		printf("\tWanted: ");
		display_lpt_status(mask, want);
		return -got;
	}
	return got;
}
/*****************************************************************************
*****************************************************************************/
static int read_lpt_nibble(void)
{
	unsigned timeout, i;

/* "1. Host signals ability to take data by asserting HostBusy low" */
	g_ctrl |= LPT_CTRL_FEED;		/* nAUTOFEED=1 */
	outportb(g_io_adr + 2, g_ctrl);
/* "2. Peripheral responds by placing first nibble on status lines.
3. Peripheral signals valid nibble by asserting PtrClk low." */
// xxx - what value for this timeout? currently 1 sec
	for(timeout = 500; timeout != 0; timeout--)
	{
		i = inportb(g_io_adr + 1);
		if((i & LPT_STAT_ACK) == 0)	/* nACK==0 */
			break;
		delay(1);
	}
	if(timeout == 0)
	{
		printf("Error: parallel port device did not become ready\n");
		return -1;
	}
/* "Unfortunately, since the nACK line is generally used to provide a
peripheral interrupt, the bits used to transfer a nibble are not
conveniently packed into the byte defined by the Status register.
For this reason, the software must read the status byte and then
manipulate the bits in order to get a correct byte." */
	if(i & 0x80)
		i |= 0x40;
	else
		i &= ~0x40;
	i >>= 3;
	i &= 0x0F;
/* whoops, I need this, too. "The last line toggles two inverted bits
which were read in on the Busy line." -- Craig Peacock's
"Interfacing the Standard Parallel Port" */
	i ^= 0x08;
/* "4. Host sets HostBusy high to indicate that it has received
the nibble and is not yet ready for another nibble." */
	g_ctrl &= ~LPT_CTRL_FEED;		/* nAUTOFEED=0 */
	outportb(g_io_adr + 2, g_ctrl);
/* "5. Peripheral sets PtrClk high to acknowledge host" */
	for(timeout = 500; timeout != 0; timeout--)
	{
		if(inportb(g_io_adr + 1) & LPT_STAT_ACK)/* nACK==1 */
			break;
		delay(1);
	}
	if(timeout == 0)
	{
		printf("Error: parallel port device did not become ready\n");
		return -1;
	}
	return i;
}
/*****************************************************************************
Returns number of bytes read
xxx - error line signals end of input?
xxx - switch to "reverse idle" mode when done?
*****************************************************************************/
static int read_lpt_nibble_mode(void *buf_ptr, unsigned buf_size)
{
	unsigned char *buf;
	unsigned i, j;
	int k;

setbuf(stdout, NULL);
	buf = (unsigned char *)buf_ptr;
	for(i = 0; i < buf_size; i++)
	{
//		if(inportb(g_io_adr + 1) & LPT_STAT_ERR)
//			break;
/* read bits 0-3 (low nibble) */
putchar('*');
		k = read_lpt_nibble();
		if(k < 0)
			break;
		j = k;

//		if(inportb(g_io_adr + 1) & LPT_STAT_ERR)
//			break;
/* "6. States 1 through 5 repeat for the second nibble" */
putchar('*');
		k = read_lpt_nibble();
		if(k < 0)
			break;
		k <<= 4;
		j |= k;
/* store byte */
		buf[i] = j;
	}
putchar('\n');
	return i;
}
/*****************************************************************************
*****************************************************************************/
#define BPERL		16	/* byte/line for dump */

void dump(void *data_p, unsigned count)
{
	unsigned char *data = (unsigned char *)data_p;
	unsigned byte1, byte2;

	while(count != 0)
	{
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
#define	MAX	256

int main(void)
{
	static unsigned char buf[MAX];
/**/
	unsigned stat;
	int i;

printf("I/O address=0x%X\n", g_io_adr);
/* reset parallel port device */
printf("resetting parallel port...\n");
//	(void)await_lpt();
	reset_lpt();

/* "1284 Negotiation phases transitions:" */
	if(await_lpt())
		return 1;
/* 1. "The host places the requested extensibility byte on the data lines" */
//0x04 requests that Device ID be returned using nibble mode. THIS WORKS:
	outportb(g_io_adr + 0, 0x04);
//0x05 requests that Device ID be returned using byte mode. DOESN'T WORK:
//	outportb(g_io_adr + 0, 0x05);
//0x14 requests that Device ID be returned using ECP mode. DOESN'T WORK:
//	outportb(g_io_adr + 0, 0x14);

//0x01 requests reverse data transfer in byte mode. DOESN'T WORK:
//	outportb(g_io_adr + 0, 0x01);
//0x00 requests reverse data transfer in nibble mode. DOESN'T WORK:
//	outportb(g_io_adr + 0, 0x00);

//0x10 requests ECP mode. DOESN'T WORK (?!):
//	outportb(g_io_adr + 0, 0x10);
//0x30 requests ECP mode with RLE. DOESN'T WORK:
//	outportb(g_io_adr + 0, 0x30);
//0x40 requests EPP mode. DOESN'T WORK:
//	outportb(g_io_adr + 0, 0x40);

printf("attempting IEEE-1284 negotiation...\n");
/* "2. The host then sets nSelectIn high and nAutoFeed low
to indicate a negotiation sequence." */
#if 1
	g_ctrl = !LPT_CTRL_SEL |	/* nSELECTIN=0*/
		//LPT_CTRL_INIT |	/*     nINIT=1 */
		!LPT_CTRL_INIT |	/*      INIT=0 */
		LPT_CTRL_FEED |		/* nAUTOFEED=1 */
		!LPT_CTRL_STB;		/*   nSTROBE=0 */
#else
	g_ctrl &= ~LPT_CTRL_SEL;	/* nSELECTIN=0 */
	g_ctrl |= LPT_CTRL_FEED;	/* nAUTOFEED=1 */
#endif
	if(await_lpt())
		return 1;
	outportb(g_io_adr + 2, g_ctrl);

/* "3. A 1284 peripheral will respond by setting [nAck low, PE high,
Select high, and nError high.] A non-1284 peripheral will not respond." */
	stat = !LPT_STAT_BUSY |		/*   BUSY==0 */
		!LPT_STAT_ACK |		/*   nACK==0 */
		LPT_STAT_EMPTY |	/*     PE==1 */
		LPT_STAT_SEL | 		/* SELECT==1 */
		LPT_STAT_ERR;		/* nERROR==1 */
// xxx - what value for this timeout?
	if(check_lpt_status(LPT_STAT_MASK, stat, 1000) < 0)
	{
// xxx - port or device is not compatible with IEEE-1284
// maybe probe to see if it's an old port or device?
		printf("\tError in parallel port phase 1 negotiation\n");
		return 1;
	}
printf("IEEE-1284-compatible device detected...\n");
/* "4. The host sets nStrobe low. This is used to strobe the
Extensibility byte in to the peripheral." */
	g_ctrl |= LPT_CTRL_STB;		/* nSTROBE=1 */
	outportb(g_io_adr + 2, g_ctrl);
delay(10); // xxx - what value for this delay?
/* "5. The host then sets nStrobe and nAutoFeed high to signal
to the peripheral that it recognizes it as a 1284 device." */
	g_ctrl &= ~LPT_CTRL_STB;	/*   nSTROBE=0 */
	g_ctrl &= ~LPT_CTRL_FEED;	/* nAUTOFEED=0 */
	outportb(g_io_adr + 2, g_ctrl);
/* "6. The peripheral responds by setting PE low, Select high if the
requested mode is available, and nError low if the peripheral has
reverse channel data available"
"7. The peripheral now sets nAck high to signal that the negotiation
sequence is over and the signal lines are in a state compatible with
the request mode." */
	stat = LPT_STAT_ACK |		/*    nACK==1 */
		!LPT_STAT_EMPTY |	/*     PE==0 */
		LPT_STAT_SEL |		/* SELECT==1 */
		!LPT_STAT_ERR;		/* nERROR==0 */
// xxx - what value for this timeout?
	i = check_lpt_status(LPT_STAT_MASK, stat, 1000);
	if(i < 0)
	{
		printf("\tError in parallel port phase 2 negotiation\n");
		return 1;
	}
printf("Parallel port negotiation successful\n");

printf("Reading from parallel port device; please wait...\n");
	i = read_lpt_nibble_mode(buf, MAX);
printf("read_lpt_nibble_mode() returned %d\n", i);
	dump(buf, i);

//printf("\t");
//display_lpt_control(LPT_CTRL_MASK);
	return 0;
}
