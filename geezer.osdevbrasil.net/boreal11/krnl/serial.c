/*----------------------------------------------------------------------------
SERIAL PORT ROUTINES

EXPORTS:
extern serial_t g_com[];

void serial_irq3(void);
void serial_irq4(void);
int init_serial(void);
int serial_setup(serial_t *port, unsigned long baud,
		unsigned bits, char enable_fifo);
----------------------------------------------------------------------------*/
#include <string.h> /* NULL */
#include <system.h> /* inportb(), outportb() */
#include <errno.h> /* ERR_... */
#include "_krnl.h"

/* IMPORTS
from MM.C */
void *kmalloc(unsigned size);
void kfree(void *blk);

/* from KBD.C */
int deq(queue_t *q, unsigned char *data);
int inq(queue_t *q, unsigned char data);

#define	BUF_SIZE	64

serial_t g_com[4];
/*****************************************************************************
*****************************************************************************/
static void serial_handler(serial_t *port)
{
/* count number of transmit and receive INTERRUPTS; not number of bytes */
	unsigned char rx_int = 0, tx_int = 0, c, reason;
	int stat, i;

	port->int_count++;
	reason = inportb(port->io_adr + 2);
/* careful: a loop inside an interrupt serial_handlerial can cause the system to hang */
	while((reason & 0x01) == 0)
	{
		reason >>= 1;
		reason &= 0x07;
/* line status interrupt (highest priority)
cleared by reading line status register (LSR; register 5) */
		if(reason == 3)
		{
			stat = inportb(port->io_adr + 5);
/* 0x80 == one or more errors in Rx FIFO
   0x10 == received BREAK */
			if(stat & 0x08) /* framing error */
				port->ferr_count++;
			if(stat & 0x04) /* parity error */
				port->perr_count++;
			if(stat & 0x02) /* overrun error */
				port->oerr_count++;
		}
/* receive data interrupt (2nd highest priority)
cleared by reading receiver buffer register (register 0) */
		else if(reason == 2)
		{
/* count ONE receive interrupt */
			if(!rx_int)
			{
				port->rx_count++;
				rx_int = 1;
			}
/* drain the receive FIFO completely!
poll the Data Ready bit; not the interrupt bits */
			while(inportb(port->io_adr + 5) & 0x01)
				(void)inq(&port->rx, inportb(port->io_adr + 0));
		}
/* character timeout interrupt (2nd highest priority; FIFO mode only)
cleared by receive buffer register */
		else if(reason == 6)
		{
/* count ONE receive interrupt */
			if(!rx_int)
			{
				port->rx_count++;
				rx_int = 1;
			}
/* drain the receive FIFO completely!
poll the Data Ready bit; not the interrupt bits */
			while(inportb(port->io_adr + 5) & 0x01)
				(void)inq(&port->rx, inportb(port->io_adr + 0));
		}
/* transmit holding register empty interrupt (3rd highest priority)
cleared by reading the interrupt ID register (IIR; register 2)
or by writing into transmit holding register (THR; register 0) */
		else if(reason == 1)
		{
/* queue up to port->fifo_size bytes */
			for(i = port->fifo_size; i != 0; i--)
			{
				if(deq(&port->tx, &c) < 0)
				{
/* empty transmit queue: disable further transmit interrupts */
					c = inportb(port->io_adr + 1);
					if(c & 0x02)
						outportb(port->io_adr + 1, c & ~0x02);
					break;
				}
/* count ONE transmit interrupt */
				if(!tx_int)
				{
					port->tx_count++;
					tx_int = 1;
				}
				outportb(port->io_adr + 0, c);
			}
		}
/* modem status interrupt (4th highest priority)
cleared by reading the modem status register (MSR; register 6) */
		else if(reason == 0)
		{
			(void)inportb(port->io_adr + 6);
		}
		reason = inportb(port->io_adr + 2);
	}
}
/*****************************************************************************
*****************************************************************************/
void serial_irq3(void)
{
	serial_t *port;

	port = g_com + 1;
	if(port->io_adr != 0)
		serial_handler(port);
	port = g_com + 3;
	if(port->io_adr != 0)
		serial_handler(port);
}
/*****************************************************************************
*****************************************************************************/
void serial_irq4(void)
{
	serial_t *port;

	port = g_com + 0;
	if(port->io_adr != 0)
		serial_handler(port);
	port = g_com + 2;
	if(port->io_adr != 0)
		serial_handler(port);
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
	kprintf("Serial chip type: ");
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
		if(i == 0xA5 && j == 0x5A)
			kprintf("8250A/16450 (no FIFO)\n");
		else
/* ALL 8250s (including 8250A) have serious problems... */
			kprintf("ewww, 8250/8250B (no FIFO)\n");
	}
	else if(i == 0x40)
		kprintf("UNKNOWN; assuming no FIFO\n");
	else if(i == 0x80)
		kprintf("16550; defective FIFO disabled\n");
	else if(i == 0xC0)
	{
/* for 16650+, should be able to read 0 from EFR
else will read 1 from FCR */
		outportb(io_adr + 3, 0xBF);
		if(inportb(io_adr + 2) == 0)
		{
			kprintf("16650+ (32-byte FIFO)\n");
			return 32;
		}
		else
		{
			kprintf("16550A (16-byte FIFO)\n");
			return 16;
		}
	}
	return 1;
}
/*****************************************************************************
*****************************************************************************/
int init_serial(void)
{
	unsigned i, io_adr;
	serial_t *port;

/* there are up to 4 COM port I/O addresses stored
in the BIOS data area, at address 400h */
	for(i = 0; i < 4; i++)
	{
		io_adr = *(uint16_t *)(0x400 + i * 2 - g_kvirt_to_phys);
		if(io_adr == 0)
			continue;
		port = g_com + i;
/* allocate memory for receive and transmit queues */
		port->rx.data = kmalloc(BUF_SIZE);
		if(port->rx.data == NULL)
			return -ERR_NO_MEM;
		port->tx.data = kmalloc(BUF_SIZE);
		if(port->tx.data == NULL)
		{
			kfree(port->rx.data);
			return -ERR_NO_MEM;
		}
		port->rx.size = port->tx.size = BUF_SIZE;
		port->io_adr = io_adr;
		port->fifo_size = serial_id(io_adr);
	}
	return 0;
}
/*****************************************************************************
Sets bit rate and number of data bits and optionally enables FIFO.

Also enables all interrupts except transmit
and clears dangling interrupts.
*****************************************************************************/
int serial_setup(serial_t *port, unsigned long baud,
		unsigned bits, char enable_fifo)
{
	unsigned divisor, io_adr, i;

	if(baud > 115200L || baud < 2)
	{
		kprintf("serial_setup: bit rate (%lu) "
			"must be < 115200 and > 2\n", baud);
		return -1;// xxx
	}
	divisor = (unsigned)(115200L / baud);
	if(bits < 7 || bits > 8)
	{
		kprintf("serial_setup: number of data bits (%u) "
			"must be 7 or 8\n", bits);
		return -1;// xxx
	}
/* set bit rate */
	io_adr = port->io_adr;
	outportb(io_adr + 3, 0x80);
	outportb(io_adr + 0, divisor);
	divisor >>= 8;
	outportb(io_adr + 1, divisor);
/* set number of data bits
This also sets parity=none and stop bits=1 */
	outportb(io_adr + 3, (bits == 7) ? 2 : 3);
/* enable all interrupts EXCEPT transmit */
	outportb(io_adr + 1, 0x0D);
/* turn on FIFO, if any */
	if(port->fifo_size > 1 && enable_fifo)
		outportb(io_adr + 2, 0xC7);
	else
		outportb(io_adr + 2, 0);
/* loopback off, interrupt gate (Out2) on,
handshaking lines (RTS, DTR) off */
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
