/*----------------------------------------------------------------------------
SERIAL PORT CHARACTER DEVICE

EXPORTS:
int open_ser(file_t *f, unsigned unit);
----------------------------------------------------------------------------*/
#include <string.h> /* NULL */
#include <system.h> /* inportb(), outportb() */
#include "_krnl.h"

/* IMPORTS
from SERIAL.C */
extern serial_t g_com[];

int serial_setup(serial_t *port, unsigned long baud,
		unsigned bits, char enable_fifo);

/* from THREADS.C */
int sleep_on(wait_queue_t *queue, unsigned *timeout);
void wake_up(wait_queue_t *queue);

/* from KBD.C */
int deq(queue_t *q, unsigned char *data);
int inq(queue_t *q, unsigned char data);
/*****************************************************************************
*****************************************************************************/
#if 0
static int close_ser(file_t *f_UNUSED)
{
	return 0;
}
#else
static int close_ser(file_t *f)
{
	serial_t *port;

	port = (serial_t *)f->data;
/* display statistics */
	kprintf("Total interrupts   : %5u\t", port->int_count);
	kprintf("Receive interrupts : %5u\n", port->rx_count);
	kprintf("Transmit interrupts: %5u\t", port->tx_count);
	kprintf("Framing errors     : %5u\n", port->ferr_count);
	kprintf("Parity errors      : %5u\t", port->perr_count);
	kprintf("Overrun errors     : %5u\n", port->oerr_count);
	return 0;
}
#endif
/*****************************************************************************
*****************************************************************************/
static int write_ser(file_t *f, unsigned char *buf, unsigned len)
{
	unsigned i, kick = 0;
	serial_t *port;

	port = (serial_t *)f->data;
	for(i = 0; i < len; i++)
	{
		while(inq(&port->tx, *buf))
		{
/* output queue is full? kick-start transmit */
			if(!kick)
			{
				outportb(port->io_adr + 1,
					inportb(port->io_adr + 1) | 0x02);
				kick = 1;
			}
			sleep_on(&port->wait, NULL); /* no timeout */
		}
		buf++;
	}
	if(!kick)
		outportb(port->io_adr + 1, inportb(port->io_adr + 1) | 0x02);
	return len;
}
/*****************************************************************************
*****************************************************************************/
static int read_ser(file_t *f, unsigned char *buf, unsigned len)
{
	serial_t *port;
	unsigned i;

	port = (serial_t *)f->data;
	for(i = 0; i < len; i++)
	{
		do
		{
			if(port->rx.out_ptr == port->rx.in_ptr) /* empty queue */
				sleep_on(&port->wait, NULL); /* no timeout */
		} while(deq(&port->rx, buf));
		buf++;
	}
	return len;
}
/*****************************************************************************
*****************************************************************************/
static int select_ser(file_t *f, unsigned access, unsigned *timeout)
{
	serial_t *port;
	unsigned ptr;
	int rv;

	port = (serial_t *)f->data;
	while(1)
	{
		rv = 0;
/* check if ready to write */
		ptr = port->tx.in_ptr + 1;
		if(ptr > port->tx.size)
			ptr = 0;
/* ready to write if output queue is not full */
		if(ptr != port->tx.out_ptr)
			rv |= 0x02;
/* ready to read if input queue is not empty */
		if(port->rx.out_ptr != port->rx.in_ptr)
			rv |= 0x01;
/* ready to read or write; return */
		rv &= access;
		if(rv)
			break;
/* not ready to read or write, but no timeout, so return */
		if(timeout == NULL)
			break;
/* not ready; wait... */
		if(sleep_on(&port->wait, timeout))
			break;
	}
	return rv;
}
/*****************************************************************************
*****************************************************************************/
int open_ser(file_t *f, unsigned unit)
{
	static const ops_t ops =
	{
		close_ser, write_ser, read_ser, select_ser
	};
/**/
	serial_t *port;

	port = g_com + unit;
/* zero stats */
	port->int_count = port->rx_count = port->tx_count = 0;
	port->ferr_count = port->perr_count = port->oerr_count = 0;
/* set 115200 baud, 8N1, FIFO on */
	if(serial_setup(port, 115200L, 8, 1))
//	if(serial_setup(port, 38400L, 8, 1))
	{
		kprintf("Error setting serial port bit rate\n");
		return -1;// xxx
	}
/* raise RTS and DTR or modem will "play dead"
after connecting to server (hardware handshaking) */
outportb(port->io_adr + 4, 0x0B);
	f->data = (void *)port;
	f->ops = &ops;
	return 0;
}
