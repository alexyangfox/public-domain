/*----------------------------------------------------------------------------
Interrupt-driven serial port code
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer/
This code is public domain (no copyright).
You can do whatever you want with it.
Compile with: Turbo C, Borland C for DOS, or Watcom C for DOS

September 9, 2009
- Initial release

To do:
- make it work with DJGPP
- this code does not currently support more than one COM port per IRQ
- maybe detect serial chip by probing instead of reading I/O address from BIOS
- maybe probe IRQ instead of assuming IRQ 3/4
- allow user control of modem handshake lines (RTS, CTS, etc.)
- do something when errors (framing, parity, etc.) are detected
----------------------------------------------------------------------------*/
#if 1
#define	_DEBUG		1
#define	DEBUG(X)	X
#else
#define	_DEBUG		0
#define	DEBUG(X)	/* nothing */
#endif
/*----------------------------------------------------------------------------
QUEUES
----------------------------------------------------------------------------*/
typedef struct
{
	unsigned char *buf;
	unsigned size, in_ptr, out_ptr;
} queue_t;
/*****************************************************************************
*****************************************************************************/
static int inq(queue_t *q, unsigned char data)
{
	unsigned temp;

	temp = q->in_ptr + 1;
	if(temp >= q->size)
		temp = 0;
/* if in_ptr reaches out_ptr, the queue is full */
	if(temp == q->out_ptr)
		return -1;
	q->buf[q->in_ptr] = data;
	q->in_ptr = temp;
	return 0;
}
/*****************************************************************************
this function used to look like this:
	static int deq(queue_t *q, unsigned char *data)
but Watcom C produces broken code for such a function. I don't know why.
*****************************************************************************/
static int deq(queue_t *q)
{
	int rv;

/* if out_ptr reaches in_ptr, the queue is empty */
	if(q->out_ptr == q->in_ptr)
		return -1;
	rv = q->buf[q->out_ptr];
	q->out_ptr++;
	if(q->out_ptr >= q->size)
		q->out_ptr = 0;
	return rv;
}
/*----------------------------------------------------------------------------
SERIAL PORT I/O

EXPORTS:
serial_t (can be re-defined as an opaque type)

extern serial_t *g_port;

void serial_destroy(serial_t *port, int do_stats);
serial_t *serial_create(unsigned io_adr);
int serial_setup(serial_t *port, unsigned long baud,
		unsigned bits, int enable_fifo);
void serial_tx(serial_t *port, char *s);
char *serial_rx(serial_t *port, unsigned timeout0);
----------------------------------------------------------------------------*/
#include <stdlib.h> /* free(), calloc(), malloc(), realloc() */
#include <string.h> /* memset() */
#include <stdio.h> /* printf() */
#include <dos.h> /* inportb(), outportb(), [_dos_]getvect(), [_dos_]setvect() */

#if defined(__WATCOMC__)
#define	inportb(P)	inp(P)
#define	outportb(P,V)	outp(P,V)
#define	getvect(N)	_dos_getvect(N)
#define	setvect(N,H)	_dos_setvect(N,H)
#endif

#if defined(__TURBOC__)
#if __TURBOC__<=0x300 /* Turbo C++ 1.0 */
#define	_interrupt	interrupt
#endif
#endif

typedef struct
{
	queue_t rx, tx;
	char tx_busy;
/* number of interrupts: spurious, total, MSR events, transmit, receive... */
	unsigned spur_count, int_count, msr_count, tx_count, rx_count;
/* ...overrun/parity/framing error, other (the latter should =0) */
	unsigned err_count, unknown_count;
/* number of: framing errors, parity errors, overrun errors */
	unsigned ferr_count, perr_count, oerr_count;
	unsigned fifo_size;
/* hardware resources */
	unsigned io_adr, irq;
	void _interrupt (*old_handler)(void);
} serial_t;

// xxx - any way to get rid of this global variable?
static serial_t *g_port;
/*****************************************************************************
*****************************************************************************/
static void _interrupt serial_irq(void)
{
	unsigned iir, reason, lsr, i;
	serial_t *port;
	int c;

	port = g_port;
/* verify interrupt is from serial chip */
	iir = inportb(port->io_adr + 2);
	if(iir & 0x01)
		port->spur_count++;
	else
		port->int_count++;
/* loop while chip signals interrupt */
	for(; (iir & 0x01) == 0; )
	{
/* determine reason for interrupt */
		reason = iir;
		reason >>= 1;
		reason &= 0x07;
		switch(reason)
		{
/* MSR event */
		case 0:
			port->msr_count++;
/* ...clear MSR interrupt by reading MSR */
			(void)inportb(port->io_adr + 6);
			break;
/* transmitter ready for more data */
		case 1:
			port->tx_count++;
			c = deq(&port->tx);
/* nothing more to transmit.
Reading from IIR cleared the transmit interrupt. */
			if(c < 0)
				port->tx_busy = 0;
/* fill the transmit FIFO to minimize interrupts */
			else
			{
				for(i = port->fifo_size; i != 0; i--)
				{
					outportb(port->io_adr + 0, c);
					c = deq(&port->tx);
					if(c < 0)
						break;
				}
			}
			break;
/* received data is available */
		case 2:
/* stale data in receive FIFO */
		case 6:
			port->rx_count++;
/* ...clear receive interrupt by reading RBR */
			lsr = inportb(port->io_adr + 5);
			for(; lsr & 0x01; )
			{
				c = inportb(port->io_adr + 0);
// xxx - keep track of queue overflow?
				(void)inq(&port->rx, c);
				lsr = inportb(port->io_adr + 5);
			}
			break;
/* overrun/parity/framing error or break received */
		case 3:
			port->err_count++;
/* ...clear error interrupt by reading LSR */
			lsr = inportb(port->io_adr + 5);
			if(lsr & 0x02)
				port->oerr_count++;
			if(lsr & 0x04)
				port->perr_count++;
			if(lsr & 0x08)
				port->ferr_count++;
			break;
/* WTF? */
		default:
			port->unknown_count++;
			break;
		}
		iir = inportb(port->io_adr + 2);
	}
// xxx - also need outportb(0xA0, 0x20); if IRQ >= 8
	outportb(0x20, 0x20);
}
/*****************************************************************************
Identifies serial chip type (8250, 16550, etc.)
Returns FIFO size or 1 if no/defective FIFO. 16650+ detection is UNTESTED.
*****************************************************************************/
static unsigned serial_id(unsigned io_adr)
{
	unsigned i, j;

/* set EFR = 0 (16650+ chips only)
"The EFR can only be accessed after writing [0xBF] to the LCR..."
For 16550/A, this code zeroes the FCR instead */
	outportb(io_adr + 3, 0xBF);
	outportb(io_adr + 2, 0);
/* set FCR = 1 to enable FIFOs (if any) */
	outportb(io_adr + 3, 0);
	outportb(io_adr + 2, 0x01);
/* enabling FIFOs should set bits b7 and b6 in Interrupt ID register */
	i = inportb(io_adr + 2) & 0xC0;
	DEBUG(printf("Serial chip type: ");)
/* no FIFO -- check if scratch register exists */
	if(i == 0)
	{
		outportb(io_adr + 7, 0xA5);
		outportb(0x80, 0xFF); /* prevent bus float returning 0xA5 */
		i = inportb(io_adr + 7);
		outportb(io_adr + 7, 0x5A);
		outportb(0x80, 0xFF); /* prevent bus float returning 0x5A */
		j = inportb(io_adr + 7);
/* scratch register 7 exists */
	DEBUG(
		if(i == 0xA5 && j == 0x5A)
			printf("8250A/16450 (no FIFO)\n");
		else
/* all 8250s (including 8250A) are said to have serious problems... */
			 printf("ewww, 8250/8250B (no FIFO)\n");
		)
	}
	DEBUG(
/* FIFO exists... */
	else if(i == 0x40)
		printf("UNKNOWN; assuming no FIFO\n");
	else if(i == 0x80)
		printf("16550; defective FIFO disabled\n");
	)
	else if(i == 0xC0)
	{
/* for 16650+, should be able to read 0 from EFR
else will read 1 from FCR */
		outportb(io_adr + 3, 0xBF);
		if(inportb(io_adr + 2) == 0)
		{
			DEBUG(printf("16650+ (32-byte FIFO)\n");)
			return 32;
		}
		else
		{
			DEBUG(printf("16550A (16-byte FIFO)\n");)
			return 16;
		}
	}
	return 1;
}
/*****************************************************************************
*****************************************************************************/
void serial_destroy(serial_t *port, int do_stats)
{
	unsigned vect_num;

	if(port == NULL)
		return;
/* disable all interrupts */
	outportb(port->io_adr + 1, 0);
/* restore old interrupt handler */
	vect_num = (port->irq < 8) ? (port->irq + 8)
		: (port->irq - 8 + 112);
	setvect(vect_num, port->old_handler);
/* free buffer memory */
	if(port->rx.buf != NULL)
		free(port->rx.buf);
	if(port->tx.buf != NULL)
		free(port->tx.buf);
	if(do_stats)
	{
/* interrupts not caused by the serial chip: */
		printf("Spurious interrupts: %5u\t", port->spur_count);
/* interrupts caused by the serial chip: */
		printf("Valid interrupts   : %5u\n", port->int_count);
/* the next five values should sum to the previous value, I think */
		printf("MSR interrupts     : %5u\t", port->msr_count);
		printf("Transmit interrupts: %5u\n", port->tx_count);
		printf("Receive interrupts : %5u\t", port->rx_count);
		printf("Error interrupts   : %5u\n", port->err_count);
/* this value should be zero: */
		printf("Unknown interrupts : %5u\t", port->unknown_count);
		printf("Framing errors     : %5u\n", port->ferr_count);
		printf("Parity errors      : %5u\t", port->perr_count);
		printf("Overrun errors     : %5u\n", port->oerr_count);
	}
/* zero the serial_t object to cause a segfault
if we attempt to use it after this function */
	memset(port, 0, sizeof(serial_t));
	free(port);
}
/*****************************************************************************
*****************************************************************************/
#define	BUF_SIZE	256

serial_t *serial_create(unsigned io_adr)
{
	unsigned i, vect_num, mask;
	serial_t *port;

/* create serial port object */
	port = calloc(1, sizeof(serial_t));
	if(port == NULL)
MEM:	{
		serial_destroy(port, 0);
printf("serial_create: out of memory\n");
		return NULL;
	}
/* allocate buffers */
	port->rx.buf = malloc(BUF_SIZE);
	port->tx.buf = malloc(BUF_SIZE);
	if(port->rx.buf == NULL || port->tx.buf == NULL)
		goto MEM;
	port->rx.size = port->tx.size = BUF_SIZE;
/* probe for serial chip at given io_adr */
	for(i = 0; i < 4; i++)
	{
// xxx - probe for chip; don't use the BIOS like this:
#undef peekw
#define peekw(S,O)	*(unsigned short far *)MK_FP(S,O)
		if(peekw(0x40, 0 + 2 * i) != io_adr)
			continue;
		port->io_adr = io_adr;
// xxx - probe IRQ; don't assume these values:
		port->irq = (i & 0x01) ? 3 : 4;
		port->fifo_size = serial_id(io_adr);
		DEBUG(printf("I/O=0x%03X, IRQ=%u, BUF_SIZE=%u\n",
			port->io_adr, port->irq, BUF_SIZE);)
/* install interrupt handler */
		vect_num = (port->irq < 8) ? (port->irq + 8)
			: (port->irq - 8 + 112);
		port->old_handler = getvect(vect_num);
		setvect(vect_num, serial_irq);
/* enable serial IRQ at 8259 interrupt controller chip(s) */
		if(port->irq < 8)
		{
			mask = 1 << port->irq;
			outportb(0x21, inportb(0x21) & ~mask);
		}
		else
		{
			mask = 1 << (port->irq - 8);
			outportb(0xA1, inportb(0xA1) & ~mask);
			outportb(0x21, inportb(0x21) & ~0x04);
		}
		return port;
	}
printf("serial_create: no serial chip at I/O address 0x%03X\n", io_adr);
	serial_destroy(port, 0);
	return NULL;
}
/*****************************************************************************
Sets bit rate and number of data bits and optionally enables FIFO.

Also enables all interrupts except transmit
and clears dangling interrupts.
*****************************************************************************/
int serial_setup(serial_t *port, unsigned long baud,
		unsigned bits, int enable_fifo)
{
	unsigned divisor, io_adr, i;

	if(baud > 115200L || baud < 2)
	{
printf("serial_setup: bit rate (%lu) must be < 115200 and > 2\n", baud);
		return -1;
	}
	divisor = (unsigned)(115200L / baud);
	if(bits < 7 || bits > 8)
	{
printf("serial_setup: number of data bits (%u) must be 7 or 8\n", bits);
		return -1;
	}
	io_adr = port->io_adr;
/* set bit rate. b7 of register 3 (DLAB)=1:
access bit-rate divisor at registers 0 and 1 */
	outportb(io_adr + 3, 0x80);
	outportb(io_adr + 0, divisor);
	divisor >>= 8;
	outportb(io_adr + 1, divisor);
/* set 7 or 8 data bits, no parity, 1 stop bit. b7 of register 3 (DLAB)=0:
access tx/rx at register 0 and interrupt enable at register 1 */
	outportb(io_adr + 3, (bits == 7) ? 2 : 3);
/* enable all interrupts at the serial chip */
	outportb(io_adr + 1, 0x0F);
/* clear FIFO (if any), then enable */
	outportb(io_adr + 2, 0x06);
	if(port->fifo_size > 1 && enable_fifo)
		outportb(io_adr + 2, 0xC7);
	else
		outportb(io_adr + 2, 0);
/* loopback off, interrupt gate (Out2) on, handshaking lines (RTS, DTR) off */
	outportb(io_adr + 4, 0x08);
/* clear dangling interrupts */
	while(1)
	{
		i = inportb(io_adr + 2);
		if(i & 0x01)
			break;
		(void)inportb(io_adr + 0);
		(void)inportb(io_adr + 5);
		(void)inportb(io_adr + 6);
	}
	return 0;
}
/*****************************************************************************
xxx - should this function have a timeout; like serial_rx()?
*****************************************************************************/
void serial_tx(serial_t *port, char *s)
{
	unsigned i;

/* if serial chip is not currently transmitting... */
	if(!port->tx_busy)
	{
/* ...fill the serial chip's hardware transmit FIFO */
		for(i = port->fifo_size; i != 0; i--)
		{
			if(*s == '\0')
				break;
			outportb(port->io_adr + 0, *s);
			s++;
		}
		port->tx_busy = 1;
	}
/* store remaining bytes to be transmitted in the software transmit queue */
	for(; *s != '\0'; s++)
	{
		while(inq(&port->tx, *s))
			/* queue full; wait */;
	}
}
/*****************************************************************************
*****************************************************************************/
char *serial_rx(serial_t *port, unsigned timeout0)
{
	unsigned timeout, alloc, len;
	char *s;
	int c;

	timeout = timeout0 / 10;
	alloc = len = 0;
	s = NULL;
	while(1)
	{
		c = deq(&port->rx);
/* no data from serial port: do timeout */
		if(c < 0)
		{
			delay(10);
			timeout--;
/* terminate string and return */
			if(timeout == 0)
			{
				if(s != NULL && len < alloc)
					s[len] = '\0';
				return s;
			}
		}
/* a byte is available from the serial port: */
		else
		{
/* reset timeout */
			timeout = timeout0 / 10;
/* enlarge return value string, if necessary
leave room for 0 byte at end of string */
			if(len + 1 >= alloc)
			{
				unsigned new_alloc;
				char *new_s;

/* start with 16 bytes, then double the allocated memory size
every time the string overflows */
				new_alloc = (alloc == 0) ? 16 : (alloc * 2);
				new_s = realloc(s, new_alloc);
/* no memory left for more data? return what data we have */
				if(new_s == NULL)
				{
					if(s != NULL && len < alloc)
						s[len] = '\0';
					return s;
				}
				s = new_s;
				alloc = new_alloc;
			}
/* store received byte in string */
			s[len] = c;
			len++;
		}
	}
}
/*----------------------------------------------------------------------------
MAIN ROUTINE
----------------------------------------------------------------------------*/
#include <stdlib.h> /* free() */
#include <string.h> /* strstr() */
#include <stdio.h> /* printf() */

/* opaque type */
//typedef struct { char dummy; } serial_t;

extern serial_t *g_port;

void serial_destroy(serial_t *port, int do_stats);
serial_t *serial_create(unsigned io_adr);
int serial_setup(serial_t *port, unsigned long baud,
		unsigned bits, int enable_fifo);
void serial_tx(serial_t *port, char *s);
char *serial_rx(serial_t *port, unsigned timeout0);
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
/*#include <alloc.h> heapcheck(), coreleft() */

int main(void)
{
	unsigned i, io_adr;
	serial_t *port;
	char *rx;

/*printf("heapcheck()=%d, coreleft()=%u\n", heapcheck(), coreleft());*/
/* use serial ports known to the BIOS (COM1-COM4) */
	for(i = 0; i < 4; i++)
	{
#undef peekw
#define peekw(S,O)	*(unsigned short far *)MK_FP(S,O)
		io_adr = peekw(0x40, 0 + 2 * i);
		if(io_adr == 0)
			continue;
/* open port */
		g_port = port = serial_create(io_adr);
		if(port == NULL)
			continue;
/* set 115200 baud, 8N1, FIFO (if any) on */
		if(serial_setup(port, 115200L, 8, 1) != 0)
//		if(serial_setup(port, 115200L, 8, 0) != 0)
		{
			printf("Error setting serial port bit rate\n");
			serial_destroy(port, 0);
			continue;
		}
/* detect hardware modem by sending reset command... */
		serial_tx(port, "ATZ\r");
/* ...and waiting up to 1/2 second... */
		rx = serial_rx(port, 500);
		DEBUG(
		printf("Received from serial port:\n");
		if(rx != NULL)
			dump(rx, strlen(rx));
		else
			printf("(nothing)\n");
		)
/* ...for echoed command and "OK" reply */
		if(rx == NULL)
			DEBUG(printf("FAIL: serial_rx() returned NULL\n"));
		else if(strstr(rx, "ATZ") && strstr(rx, "\r\nOK\r\n"))
			printf("*** Hardware modem detected at I/O address "
				"0x%03X ***\n", io_adr);
		else
			DEBUG(printf("FAIL: modem did not send 'OK'\n"));
		if(rx != NULL)
			free(rx);
/* close port (_DEBUG=1 to display serial port statistics) */
		serial_destroy(port, _DEBUG);
	}
/*printf("heapcheck()=%d, coreleft()=%u\n", heapcheck(), coreleft());*/
	return 0;
}
