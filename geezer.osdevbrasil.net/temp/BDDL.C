/*****************************************************************************
BDDL: Block Device Driver Library
Chris Giese <geezer@execpc.com>
http://www.execpc.com/~geezer

The ATA and ATAPI code won't run in a Windows DOS box
(the floppy code _will_ work).

If this code is configured to use the floppy, do NOT run it unless
the floppy drive light is off. A timer interrupt in the BIOS may
shut off the floppy after this code turns it on. That will, in turn,
cause this code to freeze up.

Ctrl-Break may get you out of this program if it does freeze up,
but the IDE and floppy interrupt vectors will not be restored.
You will have to reset the PC (or, if run from a Windows DOS
box, exit the DOS box).

With the current #ifdef's, this code will
- expect an ATAPI (IDE) CD-ROM drive as the slave drive
	on the primary interface (2nd drive at I/O=1F0h)
- expect a data CD-ROM inserted in the CD-ROM drive
- expect a formatted, BLANK 1.44 meg floppy inserted in drive A:
	(any data on the floppy will be overwritten)
- copy 180K (10 floppy tracks) from the CD-ROM to the floppy
- print great wads of debug information (it's probably NOT a
	good idea to redirect these messages to a file...)




to do:
- figure out a common API for the floppy, ATA, and ATAPI device drivers
- use the above API to implement a RAM disk
- add a disk cache

- finish ide_error()
- ide_enable_multmode() should also be able to disable multiple mode
- ide_enable_multmode() needs to pass a request_t struct to ide_error()
- need to call ide_select() drive? should I retry it if it fails?
- >>> finish ide_write_sectors()
- review the info retrieved from the drive in ide_identify()
- finish atapi_error()
- atapi_toc_ent() and atapi_mode_sense() request < 2048 bytes
	(less than 1 block) from atapi_cmd()
- need a floppy_select() function separate from floppy_motor_on()
- floppy_motor_on() needs to use fdev->blkdev.unit
- floppy_motor_on() has a 500 ms delay that is not interrupt-driven
- floppy_seek() has a 15 ms delay that is not interrupt-driven
- should floppy_error() seek track 1 or track 0 on disk change?
	if track 0, should it do recalibrate instead?
- finish floppy_error()
- probe geometry of inserted floppy disk (1.44M, 1.68M, other - how?)


- floppy code needs a timer interrupt to decrement dev->timeout
	and shut off the floppy motor when it reaches zero
- add code to set IDE translation? what is this function used for?
- the CD-ROM transfers only 2K on each interrupt? is this normal?
- maybe a flatter hierarchy of structures? this looks ridiculous:
	ioadr = dev->blkdev.io.adr[0];
- >>> request queues w/ validation of requests
- add code to format floppy track
- alternate floppy code that uses PIO instead of DMA?
	(the CPU will have to poll the FDC status register
	and read/write a byte every 13 us; maybe faster)

testing to do:
- WD, Conner, and Maxtor hard drives
- drives on primary interface (I/O=1F0h) and secondary (I/O=170h)
- master (unit=0xA0) and slave (unit=0xB0) drives on both interfaces
- CD-ROM as master, CD-ROM as slave w/ hard drive master, CD-ROM as
  stand-alone slave (illegal?), hard drive as stand-alone slave (illegal?)
- multiple floppy drives

notes:
- IDE/ATAPI interrupt usage:
  - irq14() and irq15() clear the IRQ at the 8259 PIC chips,
    and set _interrupt_occurred (xxx - should clear IRQ at
    the drive from the ISR, in case the IRQ is level-sensitive)
  - (ide_)await_interrupt() clears _interrupt_occurred
  - other code is responsible for clearing the interrupt
    at the drive (usu. by reading the status register)
*****************************************************************************/
#include <string.h> /* memset() */
#include <stdio.h> /* printf(), putchar() */
#include <dos.h> /* delay(), inport(), outport(), inportb(), outportb() */

#if 1
#define	DEBUG(X)	X
#else
#define	DEBUG(X)	/* nothing */
#endif

#if defined(__TURBOC__)||!defined(__cplusplus)
typedef enum
{
	false = 0, true = 1
} bool;
#endif

/* linear address of the sector buffer */
#define	LMEM		0x90000L

#if defined(__TURBOC__)
#define	INLINE		/* nothing */
//#define	_DS		_DS
/* Put the floppy track buffer at 9000h:0000 */
#define	LMEM_SEG	(LMEM >> 4)
#define	LMEM_OFF	(LMEM & 15)

#define	msleep(X)	delay(X)

#elif defined(__DJGPP__)
#define	INLINE		__inline__
#define	_DS		_my_ds()
/* Floppy track buffer at linear address 90000h */
#define	LMEM_SEG	_dos_ds
#define	LMEM_OFF	LMEM

#define	msleep(X)	delay(X)
/* The nice thing about standards is... */
#define	inport(P)	inportw(P)
#define	outport(P,V)	outportw(P,V)

#else
#error Not Turbo C, not DJGPP. Sorry.
#endif

/* geezer's Portable Interrupt Macros (tm) */
#if defined(__TURBOC__)
/* getvect(), setvect() in dos.h */

#define	INTERRUPT	interrupt

#define SAVE_VECT(num, vec)	vec = getvect(num)
#define	SET_VECT(num, fn)	setvect(num, fn)
#define	RESTORE_VECT(num, vec)	setvect(num, vec)

typedef void interrupt(*vector_t)(void);

#elif defined(__DJGPP__)
#include <dpmi.h>	/* _go32_dpmi_... */
#include <go32.h>	/* _my_cs() */

#define	INTERRUPT	/* nothing */

#define	SAVE_VECT(num, vec)	\
	_go32_dpmi_get_protected_mode_interrupt_vector(num, &vec)
#define	SET_VECT(num, fn)					\
	{							\
		_go32_dpmi_seginfo new_vector;			\
								\
		new_vector.pm_selector = _my_cs();		\
		new_vector.pm_offset = (unsigned long)fn;	\
		_go32_dpmi_allocate_iret_wrapper(&new_vector);	\
		_go32_dpmi_set_protected_mode_interrupt_vector	\
			(num, &new_vector);			\
	}
#define	RESTORE_VECT(num, vec)	\
	_go32_dpmi_set_protected_mode_interrupt_vector(num, &vec)

typedef _go32_dpmi_seginfo vector_t;

#endif

/* hardware resources used by a device */
#define	NUM_IO_SPANS	2
typedef struct
{
	unsigned char dma;			/* 8-bit DMA mask */
	unsigned short irq;			/* 16-bit IRQ mask */
	unsigned short adr[NUM_IO_SPANS];	/* start of I/O range */
	unsigned short span[NUM_IO_SPANS];	/* length of I/O range */
} io_t;

typedef struct
{
/* hardware interface (hwif; or "bus") */
	io_t io;
/* which drive on the hwif? 2 for IDE, 4 for floppy, 7 for SCSI */
	unsigned char unit;
/* generic info (i.e. used by ALL types of block device) */
	unsigned long num_blks;
	unsigned short bytes_per_blk;
/* floppy and CHS IDE only */
	unsigned short sectors, heads, cyls;
} blkdev_t;

struct _request
{
// sem_t semaphore;
	enum
	{
		BLK_CMD_READ = 1, BLK_CMD_WRITE = 2
	} cmd;			/* read or write? */
	void *dev;		/* from/to which device? */
	unsigned long blk;	/* starting at which block? */
	unsigned num_blks;	/* how many blocks? */
// unsigned current_num_blks;
	unsigned char *buf;	/* from/to what memory location? */
	unsigned errors;	/* status */
// struct _request *next;
};
typedef struct _request request_t;
//////////////////////////////////////////////////////////////////////////////
// MISC/HELPER FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
volatile unsigned short _interrupt_occurred;
/*****************************************************************************
*****************************************************************************/
unsigned short await_interrupt(unsigned short irq_mask,
		unsigned timeout)
{
	unsigned short intr = 0, time;

	DEBUG(printf("await_interrupt: irq_mask=0x%04X, timeout=%u\n",
		irq_mask, timeout);)
	for(time = timeout; time != 0; time--)
	{
		intr = _interrupt_occurred & irq_mask;
		if(intr != 0)
			break;
		msleep(1);
	}
	DEBUG(printf("await_interrupt: waited %3u ms\n", timeout - time);)
	if(time == 0)
		return 0;
	_interrupt_occurred &= ~intr;
	return intr;
}
/*****************************************************************************
*****************************************************************************/
#pragma argsused
void nsleep(unsigned nanosecs)
{
}
/*****************************************************************************
*****************************************************************************/
INLINE void insw(short port, unsigned short *data, unsigned count)
{
	for(; count != 0; count--)
	{
		*data = inport(port);
		data++;
	}
}
/*****************************************************************************
*****************************************************************************/
INLINE void outsw(short port, unsigned short *data, unsigned count)
{
	for(; count != 0; count--)
	{
		outport(port, *data);
		data++;
	}
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
/****************************************************************************
****************************************************************************/
unsigned short bswap16(unsigned short arg)
{
	return ((arg << 8) & 0xFF00) |
		((arg >> 8) & 0xFF00);
}

/* these assume little-endian CPU (e.g. x86) and sizeof(short)==2 */
#define	read_le16(X)	*(unsigned short *)(X)
#define	read_be16(X)	bswap16(*(unsigned short *)(X))
//////////////////////////////////////////////////////////////////////////////
// IDE (ATA) HARD DRIVES
//////////////////////////////////////////////////////////////////////////////
/* ATA register file (offsets from 0x1F0 or 0x170) */
#define	ATA_REG_DATA		0	/* data (16-bit) */
#define	ATA_REG_FEAT		1	/* write: feature reg */
#define	ATA_REG_ERROR	ATA_REG_FEAT	/* read: error */
#define	ATA_REG_COUNT		2	/* sector count */
#define	ATA_REG_SECTOR		3	/* sector */
#define	ATA_REG_LOCYL		4	/* LSB of cylinder */
#define	ATA_REG_HICYL		5	/* MSB of cylinder */
#define	ATA_REG_DRVHD		6	/* drive select; head */
#define	ATA_REG_CMD		7	/* write: drive command */
#define	ATA_REG_STATUS		7	/* read: status and error flags */
#define	ATA_REG_DEVCTRL		0x206	/* write: device control */
//efine	ATA_REG_ALTSTAT		0x206	/* read: alternate status/error */

/* a few of the ATA registers are used differently by ATAPI... */
#define	ATAPI_REG_REASON	2	/* interrupt reason */
#define	ATAPI_REG_LOCNT		4	/* LSB of transfer count */
#define	ATAPI_REG_HICNT		5	/* MSB of transfer count */

/* ATA command bytes */
#define	ATA_CMD_READ		0x20	/* read sectors */
#define	ATA_CMD_PKT		0xA0	/* signals ATAPI packet command */
#define	ATA_CMD_PID		0xA1	/* identify ATAPI device */
#define	ATA_CMD_READMULT	0xC4	/* read sectors, one interrupt */
#define	ATA_CMD_MULTMODE	0xC6
#define	ATA_CMD_ID		0xEC	/* identify ATA device */

typedef struct
{
/* generic block device info */
	blkdev_t blkdev;
/* information specific to IDE drive */
	unsigned has_lba : 1;
	unsigned use_lba : 1;
	unsigned has_dma : 1;
	unsigned use_dma : 1;
	unsigned has_multmode : 1;
	unsigned use_multmode : 1;
	unsigned short mult_count;
} ide_t;
/*****************************************************************************
BUSY  READY  DF  DSC		DRQ1  CORR  IDX  ERR
*****************************************************************************/
int ide_poll_status(unsigned short ioadr, unsigned timeout,
		unsigned char stat_mask, unsigned char stat_bits)
{
	unsigned time, decr;
	unsigned char stat = 0;

	DEBUG(printf("ide_poll_status: ioadr=0x%X, stat mask=0x%X, "
		"stat bits=0x%X\n", ioadr, stat_mask, stat_bits);)
/* if short timeout, poll every 1 millisecond, else every 50 ms.
If the host OS has a real-time scheduler, and that scheudler runs
every 50 ms, you could do yield() instead of msleep(50) */
	decr = (timeout < 500) ? 1 : 50;
	for(time = timeout; time != 0; time -= decr)
	{
		stat = inportb(ioadr + ATA_REG_STATUS);
		if((stat & stat_mask) == stat_bits)
			break;
		msleep(decr);
	}
	DEBUG(printf("ide_poll_status: waited %3u ms, got stat=0x%X\n",
		timeout - time, stat);)
	if(time == 0)
	{
		printf("ide_poll_status: error: timeout, stat=0x%X\n", stat);
		return -1;
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int ide_select_drive(unsigned short ioadr, unsigned char unit)
{

	DEBUG(printf("ide_select_drive: ioadr=0x%X, unit=0x%X\n",
		ioadr, unit);)
	outportb(ioadr + ATA_REG_DRVHD, unit);
/* Hale Landis: ATA-4 needs delay after drive select/head reg written */
	nsleep(400);
/* wait up to 5 seconds (longer for laptop or APM computers) for status =
BUSY=0  READY=1  DF=?  DSC=?		DRQ=0  CORR=?  IDX=?  ERR=? */
	//return ide_poll_status(ioadr, 5000, 0xC8, 0x40);

/* "ATA Packet Commands can be issued regardless of the state of the
DRDY Status Bit."  "1. The host Polls for BSY=0, DRQ=0 then initializes
the task file by writing the required parameters to the Features,
Byte Count, and Drive/Head registers."

BUSY=0  READY=?  DF=?  DSC=?		DRQ=0  CORR=?  IDX=?  ERR=? */
	return ide_poll_status(ioadr, 5000, 0x88, 0x00);
}
/*****************************************************************************
### - report time spent waiting for interrupt, like ide_poll_status() does
*****************************************************************************/
int ide_await_interrupt(unsigned short ioadr,
		unsigned short irqmask, unsigned timeout,
		unsigned char stat_mask, unsigned char stat_bits)
{
	int temp;
	unsigned char stat;

	DEBUG(printf("ide_await_interrupt: ioadr=0x%X, irqmask=0x%X, "
		"timeout=%u, stat mask=0x%X, stat bits=0x%X\n",
		ioadr, irqmask, timeout, stat_mask, stat_bits);)
	temp = await_interrupt(irqmask, timeout);
/* timeout */
	if(temp == 0)
	{
		printf("ide_await_interrupt: error: timeout\n");
		return -1;
	}
/* no timeout, but did we get the error status we wanted? */
	stat = inportb(ioadr + ATA_REG_STATUS);
	if((stat & stat_mask) != stat_bits)
	{
		printf("ide_await_interrupt: error: bad status 0x%X\n",
			stat);
		return -1;
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
#define min(a,b)    (((a) < (b)) ? (a) : (b))

void ide_write_regs(ide_t *dev, unsigned long blk, unsigned num_blks)
{
	unsigned num_sects;
	unsigned short ioadr, sector, head;

	DEBUG(printf("ide_write_regs:\n");)
	ioadr = dev->blkdev.io.adr[0];
	num_sects = min(num_blks, dev->mult_count);
	outportb(ioadr + ATA_REG_COUNT, num_sects);	/* num sectors */
/* add partition start */
//	DEBUG(printf("ide_write_regs: blk=%lu relative; ", blk);)
//	blk += dev->start_blk;
	DEBUG(printf("blk=%lu absolute, num_sects=%u of %u, ",
		blk, num_sects, num_blks);)
/* LBA */
	if(dev->has_lba && dev->use_lba)
	{
		DEBUG(printf("LBA\n");)
		outportb(ioadr + ATA_REG_SECTOR, blk);	/* sector */
		blk >>= 8;
		outportb(ioadr + ATA_REG_LOCYL, blk);	/* cylinder LSB */
		blk >>= 8;
		outportb(ioadr + ATA_REG_HICYL, blk);	/* cylinder MSB */
		blk >>= 8;
		blk &= 0x0F;
		outportb(ioadr + ATA_REG_DRVHD, 0x40 |	/* head */
			dev->blkdev.unit | blk);	/* b6 enables LBA */
	}
/* CHS */
	else
	{
/* I thought maybe the ANSI div() or ldiv() functions
could save us two divides, but those functions are
suprisingly large */
		sector = blk % dev->blkdev.sectors + 1;
		blk /= dev->blkdev.sectors;
		head = blk % dev->blkdev.heads;
		blk /= dev->blkdev.heads;

		DEBUG(printf("CHS=%lu:%u:%u\n", blk, head, sector);)
		outportb(ioadr + ATA_REG_SECTOR, sector);/* sector */
		outportb(ioadr + ATA_REG_LOCYL, blk);	/* cylinder LSB */
		blk >>= 8;
		outportb(ioadr + ATA_REG_HICYL, blk);	/* cylinder MSB */
		outportb(ioadr + ATA_REG_DRVHD,		/* head */
			dev->blkdev.unit | head);
	}
/* Hale Landis: ATA-4 needs delay after drive select/head reg written */
	nsleep(400);
}
/*****************************************************************************
xxx - finish
*****************************************************************************/
#pragma argsused
int ide_error(request_t *req)
{
	DEBUG(printf("*** ide_error: ***\n");)

	(req->errors)++;
	if(req->errors >= 3)
		return -1;
	return 0;
}
/*****************************************************************************
xxx - this should be able to disable multmode, as well
*****************************************************************************/
int ide_enable_multmode(ide_t *dev)
{
	unsigned short ioadr;
	int err;

	DEBUG(printf("ide_enable_multmode:\n");)
	ioadr = dev->blkdev.io.adr[0];
	for(;;)
	{
/* select drive */
		err = ide_select_drive(ioadr, dev->blkdev.unit);
		if(err == 0)
		{
/* issue SET MULTIPLE MODE command */
			outportb(ioadr + ATA_REG_COUNT, dev->mult_count);
			outportb(ioadr + ATA_REG_CMD, ATA_CMD_MULTMODE);
			nsleep(400);
/* wait up to 10 seconds for status =
BUSY=0  READY=1  DF=?  DSC=?		DRQ=0  CORR=?  IDX=?  ERR=0 */
			err = ide_await_interrupt(ioadr,
				dev->blkdev.io.irq, 10000, 0xC9, 0x40);
			if(err == 0)
			{
/* it worked: enable multi-sector reads */
				dev->use_multmode = 1;
				break;
			}
		}
/* error, retry */
//		err = ide_error(dev);xxx - needs req_t, not ide_t
		if(err != 0)
			break;
	}
	return err;
}
/*****************************************************************************
*****************************************************************************/
int ide_read_sectors(request_t *req)
{
	ide_t *dev;
	unsigned short ioadr;
	int err;

	DEBUG(printf("ide_read_sectors:\n");)
	dev = (ide_t *)req->dev;
/* if the ide_t struct had a magic value in it, we could validate it here */
	ioadr = dev->blkdev.io.adr[0];
/* select drive (xxx - need to do this?) */
	err = ide_select_drive(ioadr, dev->blkdev.unit);
	if(err != 0)	/* xxx - no retry? */
		return err;
/* write drive registers, except for command byte */
AGAIN:
	ide_write_regs(dev, req->blk, req->num_blks);
/* write command byte. Use multi-sector reads if the drive supports it */
	outportb(ioadr + ATA_REG_CMD,
		(dev->has_multmode && dev->use_multmode) ?
		ATA_CMD_READMULT : ATA_CMD_READ);
	nsleep(400);
/* main read loop */
	while(req->num_blks != 0)
	{
		unsigned short num_blks, num_bytes;

/* wait up to 10 seconds for status =
BUSY=0  READY=?  DF=?  DSC=?		DRQ=1  CORR=?  IDX=?  ERR=0 */
		err = ide_await_interrupt(ioadr, dev->blkdev.io.irq,
			10000, 0x89, 0x08);
/* call ide_error() if there is an error */
		if(err != 0)
		{
			err = ide_error(req);
/* if ide_error() returns non-zero, then we are doomed... */
			if(err != 0)
				return err;
/* ...else try again */
			else
				goto AGAIN;
		}
/* transfer as many sectors as multiple mode allows */
		num_blks = min(req->num_blks, dev->mult_count);
		num_bytes = num_blks * dev->blkdev.bytes_per_blk;
/* read! */
		insw(ioadr + ATA_REG_DATA, (unsigned short *)req->buf,
			num_bytes >> 1);
		DEBUG(printf("ide_read_sectors: read %u sectors of %u\n",
			num_blks, req->num_blks);)
/* advance pointers */
		req->num_blks -= num_blks;
		req->blk += num_blks;
		req->buf += num_bytes;
	}
	return 0;
}
/*****************************************************************************
xxx - finish
*****************************************************************************/
#pragma argsused
int ide_write_sectors(request_t *req)
{
	return -1;
}
/*****************************************************************************
*****************************************************************************/
unsigned short atapi_read_and_discard(unsigned short ioadr,
		unsigned char *buf, unsigned short want)
{
	unsigned short got, count;

	got = inportb(ioadr + ATAPI_REG_HICNT);
	got <<= 8;
	got |= inportb(ioadr + ATAPI_REG_LOCNT);
	DEBUG(printf("atapi_read_and_discard: want %u bytes, got %u\n",
		want, got);)
	count = min(want, got);
	insw(ioadr + ATA_REG_DATA, (unsigned short *)buf, count >> 1);
/* excess data? read and discard it */
	if(got > count)
	{
/* read only 16-bit words where possible */
		for(count = got - count; count > 1; count -= 2)
			(void)inport(ioadr + ATA_REG_DATA);
/* if the byte count is odd, read the odd byte last */
		if(count != 0)
			(void)inportb(ioadr + ATA_REG_DATA);
	}
/* return number of bytes read into memory (not discarded) */
	return count;
}
/*****************************************************************************
*****************************************************************************/
int ide_identify(ide_t *idedev)
{
	unsigned char temp1, temp2, id_cmd, buf[512], swap_chars;
	unsigned short ioadr, temp, id_delay;
	blkdev_t *blkdev;

	DEBUG(printf("ata_identify:\n");)
	blkdev = &idedev->blkdev;
	ioadr = blkdev->io.adr[0];
/* sector count and sector registers both set to 1 after soft reset */
	temp1 = inportb(ioadr + ATA_REG_COUNT);
	temp2 = inportb(ioadr + ATA_REG_SECTOR);
	if(temp1 != 0x01 || temp2 != 0x01)
	{
		printf("no drive on interface 0x%X\n", ioadr);
		return -1;
	}
/* check cylinder registers */
	temp1 = inportb(ioadr + ATA_REG_LOCYL);
	temp2 = inportb(ioadr + ATA_REG_HICYL);
	temp = inportb(ioadr + ATA_REG_STATUS);
/* ATAPI device puts 0xEB14 in the cylinder registers */
	if(temp1 == 0x14 && temp2 == 0xEB)
	{
		(void)atapi_read_and_discard(ioadr, NULL, 0);
/* now then, where were we? */
		printf("ATAPI drive, ");
		id_cmd = ATA_CMD_PID;
		id_delay = 10000;	/* WAIT_PID */
		blkdev->bytes_per_blk = 2048;
	}
/* ATAPI device puts 0 in the cylinder registers */
	else if(temp1 == 0 && temp2 == 0 && temp != 0)
	{
		printf("ATA drive, ");
		id_cmd = ATA_CMD_ID;
		id_delay = 30000;	/* WAIT_ID */
		blkdev->bytes_per_blk = 512;
	}
	else
	{	printf("unknown drive type\n");
		return -1;
	}
	_interrupt_occurred = 0;
/* issue ATA or ATAPI IDENTIFY DEVICE command, then get results */
	outportb(ioadr + ATA_REG_CMD, id_cmd);
	nsleep(400);
	if(await_interrupt(0xC000, id_delay) == 0)
	{
/* could be very old drive that doesn't support IDENTIFY DEVICE.
Read geometry from partition table? Use CMOS? */
		printf("ata_identify: IDENTIFY DEVICE failed\n");
		return -1;
	}
	insw(ioadr + ATA_REG_DATA, (unsigned short *)buf, 256);/* xxx - read ATAPI xfer count */
/* print some info */
	swap_chars = 1;
/* model name is not byte swapped for NEC, Mitsumi FX, and Pioneer CD-ROMs */
	if(id_cmd == ATA_CMD_PID)
	{
		if((buf[54] == 'N' && buf[55] == 'E') ||
			(buf[54] == 'F' && buf[55] == 'X') ||
			(buf[54] == 'P' && buf[55] == 'i'))
				swap_chars = 0;
	}
	for(temp = 54; temp < 94; temp += 2)
	{
		putchar(buf[(temp + 0) ^ swap_chars]);
		putchar(buf[(temp + 1) ^ swap_chars]);
	}
	blkdev->cyls = read_le16(buf + 2);
	blkdev->heads = read_le16(buf + 6);
	blkdev->sectors = read_le16(buf + 12);
	printf("\nCHS=%u:%u:%u, ", blkdev->cyls, blkdev->heads,
		blkdev->sectors);
	if((buf[99] & 1) != 0)
	{
		printf("DMA, ");
		idedev->has_dma = 1;
	}
	if((buf[99] & 2) != 0)
	{
		printf("LBA, ");
		idedev->has_lba = 1;
	}
	if(((buf[119] & 1) != 0) && (buf[118] != 0))
	{
		temp = buf[94];
		printf("mult_count=%u, ", temp);
	}
	else
		temp = 1;
	idedev->mult_count = temp;
	printf("%uK cache\n", read_le16(buf + 42) >> 1);
#if 1
/* PIO and DMA transfer modes indicate how fast the drive can move data.
This is of interest only if you have an intelligent IDE controller
(e.g. EIDE VLB chip for 486 system, or Triton PCI for Pentium), where the
controller can be programmed to move data at the fastest rate possible

xxx - these may not be correct */
	printf("max PIO mode %u\n", read_le16(buf + 102));
	printf("max DMA mode %u\n", read_le16(buf + 104));
	printf("single-word DMA status 0x%X\n", read_le16(buf + 124));
	printf("multi-word DMA status 0x%X\n", read_le16(buf + 126));
	printf("advanced PIO mode supported=%u\n", buf[128]);
/* bits b6:b5 indicate if the drive can generate an IRQ when DRQ goes high.
This is good for performance, since you don't have to sit and poll the
drive for 3 milliseconds. (This code always polls for DRQ.) */
	switch((buf[0] >> 5) & 3)
	{
		case 0:
			printf("  polled DRQ\n");
			break;
		case 1:
			printf("  interrupt DRQ\n");
			break;
		case 2:
			printf("  accelerated DRQ\n");
			break;
	}
#endif
	printf("\n");
	return 0;
}
/*****************************************************************************
*****************************************************************************/
void ide_do_probe(ide_t *master, ide_t *slave)
{
	unsigned short ioadr;
	unsigned char byte1, byte2;
	blkdev_t *blkdev;

	ioadr = master->blkdev.io.adr[0];
/* poke interface */
	DEBUG(printf("ide_do_probe: poking interface 0x%X\n", ioadr);)
	outportb(ioadr + ATA_REG_COUNT, 0x55);
	outportb(ioadr + ATA_REG_SECTOR, 0xAA);
	byte1 = inportb(ioadr + ATA_REG_COUNT);
	byte2 = inportb(ioadr + ATA_REG_SECTOR);
/* nothing there */
	if(byte1 != 0x55 || byte2 != 0xAA)
	{
		DEBUG(printf("nothing there\n");)
		return;
	}
/* soft reset both drives on this I/F (selects master) */
	DEBUG(printf("found something on I/F 0x%X, "
		"doing soft reset...\n", ioadr);)
	outportb(ioadr + ATA_REG_DEVCTRL, 0x06);
	nsleep(400);
/* release soft reset AND enable interrupts from drive */
	outportb(ioadr + ATA_REG_DEVCTRL, 0x00);
	nsleep(400);
/* wait up to 2 seconds for status =
BUSY=0  READY=1  DF=?  DSC=?		DRQ=?  CORR=?  IDX=?  ERR=0 */
	blkdev = &master->blkdev;
	if(ide_poll_status(ioadr, 2000, 0xC1, 0x40) != 0)
	{
		DEBUG(printf("ide_do_probe: no master on interface 0x%X\n",
			ioadr);)
		return;
	}
/* identify master */
	printf("master drive on interface 0x%X:\n", ioadr);
	ide_identify(master);
/* select slave */
	blkdev = &slave->blkdev;
	if(ide_select_drive(ioadr, blkdev->unit) != 0)
	{
		DEBUG(printf("ide_do_probe: no slave on interface 0x%X\n",
			ioadr);)
		return;
	}
/* identify slave */
	ioadr = slave->blkdev.io.adr[0];
	printf("slave drive on interface 0x%X:\n", ioadr);
	ide_identify(slave);
}
/*****************************************************************************
*****************************************************************************/
ide_t _drives[4];

void ide_probe(void)
{
	DEBUG(printf("ide_probe:\n");)
/* no DMA */
	_drives[0].blkdev.io.dma = _drives[1].blkdev.io.dma = 0;
	_drives[2].blkdev.io.dma = _drives[3].blkdev.io.dma = 0;
/* IRQs 14 and 15 */
	_drives[0].blkdev.io.irq = _drives[1].blkdev.io.irq = 0x4000;
	_drives[2].blkdev.io.irq = _drives[3].blkdev.io.irq = 0x8000;
/* 8 bytes at 0x1F0, 1 byte at 0x3F6 */
	_drives[0].blkdev.io.adr[0] = _drives[1].blkdev.io.adr[0] = 0x1F0;
	_drives[0].blkdev.io.span[0] = _drives[1].blkdev.io.span[0] = 8;
	_drives[0].blkdev.io.adr[1] = _drives[1].blkdev.io.adr[1] = 0x3F6;
	_drives[0].blkdev.io.span[1] = _drives[1].blkdev.io.span[1] = 1;
/* 8 bytes at 0x170, 1 byte at 0x376 */
	_drives[2].blkdev.io.adr[0] = _drives[3].blkdev.io.adr[0] = 0x170;
	_drives[2].blkdev.io.span[0] = _drives[3].blkdev.io.span[0] = 8;
        _drives[2].blkdev.io.adr[1] = _drives[3].blkdev.io.adr[1] = 0x376;
	_drives[2].blkdev.io.span[1] = _drives[3].blkdev.io.span[1] = 1;
/* */
	_drives[0].blkdev.unit = _drives[2].blkdev.unit = 0xA0;
	_drives[1].blkdev.unit = _drives[3].blkdev.unit = 0xB0;
/* num_blks, bytes_per_blk, CHS, and ATA-/ATAPI-specific stuff
gets set by ide_do_probe() */
	ide_do_probe(_drives + 0, _drives + 1);
	ide_do_probe(_drives + 2, _drives + 3);
	DEBUG(printf("\n");)
}
//////////////////////////////////////////////////////////////////////////////
// IDE (ATAPI) CD-ROM DRIVES
//////////////////////////////////////////////////////////////////////////////
/* the command byte within an ATAPI command packet */
#define	ATAPI_CMD_START_STOP	0x1B	/* eject/load */
#define	ATAPI_CMD_READ10	0x28	/* read data sector(s) */
#define	ATAPI_CMD_READTOC	0x43	/* read audio table-of-contents */
#define	ATAPI_CMD_PLAY		0x47	/* play audio */
#define	ATAPI_CMD_PAUSE		0x4B	/* pause/continue audio */
/*****************************************************************************
xxx - finish
*****************************************************************************/
#pragma argsused
int atapi_error(request_t *req)
{
        const char *key[] =
	{
		"no sense", "recovered error", "not ready", "medium error",
		"hardware error", "illegal request", "unit attention", "data protect",
		"?", "?", "?", "aborted command",
		"?", "?", "miscompare", "?"
	};
	ide_t *dev;
	unsigned short ioadr;
	unsigned char temp;

	DEBUG(printf("*** atapi_error: ***\n");)
	dev = (ide_t *)req->dev;
	ioadr = dev->blkdev.io.adr[0];
	temp = inportb(ioadr + ATA_REG_STATUS);
	printf("error bit=%u, ", temp & 1);
	temp = inportb(ioadr + ATA_REG_ERROR);
	printf("sense key=%u (%s)\n", temp >> 4, key[temp >>4]);
	printf("MCR=%u, ", (temp & 8) >> 3);
	printf("ABRT=%u, ", (temp & 4) >> 2);
	printf("EOM=%u, ", (temp & 2) >> 1);
	printf("ILI=%u\n", temp & 1);

	(req->errors)++;
	if(req->errors >= 3)
		return -1;
	return 0;
}
/*****************************************************************************
DRQ	REASON	"phase"
 0	  0	ATAPI_PH_ABORT
 0	  1	bad
 0	  2	bad
 0	  3	ATAPI_PH_DONE
 8	  0	ATAPI_PH_DATAOUT
 8	  1	ATAPI_PH_CMDOUT
 8	  2	ATAPI_PH_DATAIN
 8	  3	bad

b0 of REASON register is CoD or C/nD (0=data, 1=command)
b1 of REASON register is IO (0=out to drive, 1=in from drive)
*****************************************************************************/
#define	ATAPI_PH_ABORT		0	/* other possible phases */
#define	ATAPI_PH_DONE		3	/* (1, 2, 11) are invalid */
#define	ATAPI_PH_DATAOUT	8
#define	ATAPI_PH_CMDOUT		9
#define	ATAPI_PH_DATAIN		10

int atapi_cmd(request_t *req, unsigned char *pkt)
{
        const char *phase_name[] =
	{
		"ABORT",	"invalid",	"invalid",	"DONE",
		"can't happen",	"can't happen",	"can't happen",	"can't happen",
		"DATA OUT",	"CMD OUT",	"DATA IN",	"invalid"
	};
	ide_t *dev;
	blkdev_t *bdev;
	unsigned short ioadr, num_blks, num_bytes;
	int err;
	unsigned char phase;

	DEBUG(printf("atapi_cmd:\n");)
	dev = (ide_t *)req->dev;
	bdev = &dev->blkdev;
/* if the ide_t struct had a magic value in it, we could validate it here */
	ioadr = bdev->io.adr[0];
/* select drive (xxx - need to do this?) */
	err = ide_select_drive(ioadr, bdev->unit);
	if(err != 0)	/* xxx - no retry? */
		return err;
AGAIN:
/* write ATA register file. Registers 2 (COUNT) and 3 (SECTOR) are not used */
	outportb(ioadr + ATA_REG_FEAT, 0);
/* max. bytes to transfer on each DRQ */
	outportb(ioadr + ATAPI_REG_LOCNT, 32768u);
	outportb(ioadr + ATAPI_REG_HICNT, 32768u >> 8);
/* select drive and set LBA bit */
	outportb(ioadr + ATA_REG_DRVHD, 0x40 | bdev->unit);
	nsleep(400);
/* packet command byte */
	outportb(ioadr + ATA_REG_CMD, ATA_CMD_PKT);
	nsleep(400);
/* wait up to 0.5 seconds for status =
BUSY=0  READY=?  DF=?  DSC=?		DRQ=1  CORR=?  IDX=?  ERR=?

i.e. await DRQ. This can be interrupt-driven for new drives,
instead of polled, but this code doesn't support it. */
	if(ide_poll_status(ioadr, 500, 0x88, 0x08) != 0)
	{
		printf("atapi_cmd: error: drive did not "
			"accept packet cmd byte\n");
		goto ERR;
	}
/* "4. When the Device is ready to accept the Command Packet,
the Device sets CoD and clears IO. DRQ shall then be asserted
simultaneous or prior to the de-assertion of BSY."
    CoD=1, IO=0, DRQ=1 --> ATAPI_PH_CMDOUT */
	phase = inportb(ioadr + ATA_REG_STATUS) & 0x08;
	phase |= (inportb(ioadr + ATAPI_REG_REASON) & 3);
	if(phase != ATAPI_PH_CMDOUT)
	{
		printf("atapi_cmd: error: drive did not "
			"enter command out phase\n");
		DEBUG(printf("atapi_cmd: ATAPI drive now in phase '%s'\n",
			(phase < 12) ? phase_name[phase] : "can't happen");)
		goto ERR;
	}
/* write the 12-byte ATAPI packet, 2 bytes at a time */
//        outsw(ioadr + ATA_REG_DATA, (unsigned short *)pkt, 6);
        outportsl(ioadr + ATA_REG_DATA, (unsigned long *)pkt, 3);
/* main read loop */
	for(;;)
	{
/* wait up to 10 seconds for IRQ15 or IRQ14 */
		if(await_interrupt(0xC000, 10000) == 0)
		{
			printf("atapi_cmd: error: "
				"packet command timed out\n");
			goto ERR;
		}
/* get the ATAPI "phase".
Reading the status register here clears the interrupt. */
		phase = inportb(ioadr + ATA_REG_STATUS) & 0x08;
		phase |= (inportb(ioadr + ATAPI_REG_REASON) & 3);
		DEBUG(printf("atapi_cmd: ATAPI drive now in phase '%s'\n",
			(phase < 12) ? phase_name[phase] : "can't happen");)
/* DONE phase is OK, if we are truly done...

"10. When the Device is ready to present the status, the Device places
the completion status into the Status Register, sets CoD, IO, DRDY and
clears BSY, DRQ, prior to asserting INTRQ."
	CoD=1, IO=1, DRQ=0 --> ATAPI_PH_DONE

xxx - we don't check if DRDY=1

"11. After detecting INTRQ & DRQ=0 the host reads the Status Register and
if necessary, the Error Register for the command completion status." */
		if(phase == ATAPI_PH_DONE)
		{
			if(req->num_blks == 0)
				return 0;
/* ...otherwise, it's an error */
			printf("atapi_cmd: error: data shortage\n");
			goto ERR;
		}
/* besides DONE, DATA IN is the only valid phase

"7. When data is available, the Device:(1) places the byte count
of the data available into the Cylinder High and Low Registers,
(2) sets IO and clears CoD, (3) sets DRQ and clears BSY, (4) sets INTRQ."
	IO=1, CoD=0, DRQ=1 --> ATAPI_PH_DATAIN */
		/*else*/ if(phase != ATAPI_PH_DATAIN)
		{
			printf("atapi_cmd: error: "
				"drive in unexpected phase\n");
ERR:
/* call atapi_error() if there is an error */
			err = atapi_error(req);
			if(err != 0)
				return err;
/* ...else try again */
			else
				goto AGAIN;
		}
/* read!
xxx - 16 is 32768 (max bytes per DRQ) / 2048 (bytes per sector) */
		num_blks = min(req->num_blks, 16);
/* if num_blks drops to 0, then we just read and discard, no worries */
		num_bytes = atapi_read_and_discard(ioadr, req->buf,
			num_blks * bdev->bytes_per_blk);
		num_blks = num_bytes / bdev->bytes_per_blk;
/* advance pointers */
		req->buf += num_bytes;
		req->num_blks -= num_blks;
		req->blk += num_blks;
	}
}
/*****************************************************************************
*****************************************************************************/
int atapi_read_sectors(request_t *req)
{
	unsigned char pkt[12] =
	{
		ATAPI_CMD_READ10, 0,
		0, 0, 0, 0,
		0, 0, 0,
		0, 0, 0,
	};

	DEBUG(printf("atapi_read_sectors:\n");)
	pkt[2]=req->blk >> 24;
	pkt[3]=req->blk >> 16;
	pkt[4]=req->blk >> 8;
	pkt[5]=req->blk;
//	pkt[6]=req->num_blks >> 16;
	pkt[7]=req->num_blks >> 8;
	pkt[8]=req->num_blks;
	return atapi_cmd(req, pkt);
}
/*****************************************************************************
My CD-ROM is an old piece of junk Mitsumi, and doesn't support this
function, so I have no way of testing it. I don't even know if the
command bytes 0xBE (READ CD) or 0xD5 (READ CD MSF) are correct.

This is a guess, but: if atapi_mode_sense() displays
	CD-DA commands: YES
then the drive can "rip" audio tracks.
*****************************************************************************/
int atapi_rip_audio(request_t *req)
{
	unsigned char pkt[12] =
	{
#if 0
		0xBE, 0,
		0, 0, 0, 0, /* start LBA */
		0, 0, 1, /* number of blocks */
#else
		0xD5, 0, 0,
		0, 0, 0,/* start MSF */
		0, 0, 1,/* end MSF */
#endif
/* 9=10h	flag bits
	b7=synch field, b6:b5=header(s) code, b4=user data,
	b3=EDC & ECC, b2:b1=error flag(s), b0=reserved */
		0x10,
/* 10=0	b2:b0 = sub-channel data selection bits, other bits reserved */
		0,
		0,
	};

	DEBUG(printf("atapi_read_sectors:\n");)
	pkt[2]=req->blk >> 24;
	pkt[3]=req->blk >> 16;
	pkt[4]=req->blk >> 8;
	pkt[5]=req->blk;
	pkt[6]=req->num_blks >> 16;
	pkt[7]=req->num_blks >> 8;
	pkt[8]=req->num_blks;
	return atapi_cmd(req, pkt);
}
/*****************************************************************************
*****************************************************************************/
int atapi_eject(ide_t *dev, bool load)
{
	unsigned char pkt[12] =
	{
		ATAPI_CMD_START_STOP,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0
	};
	request_t req;

	DEBUG(printf("atapi_eject:\n");)
	memset(&req, 0, sizeof(req));
	req.dev = dev;
	pkt[4] = 2 + (load ? 1 : 0);
	return atapi_cmd(&req, pkt);
}
/*****************************************************************************
*****************************************************************************/
int atapi_pause(ide_t *dev, bool cont)
{
	unsigned char pkt[12] =
	{
		ATAPI_CMD_PAUSE,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0
	};
	request_t req;

	DEBUG(printf("atapi_pause\n");)
	memset(&req, 0, sizeof(req));
	req.dev = dev;
	pkt[8] = cont ? 1 : 0;
	return atapi_cmd(&req, pkt);
}
/*****************************************************************************
Use MODE SENSE (0x5A) to read page 0x2A
("CD-ROM Capabilities & Mechanical Status Page")
*****************************************************************************/
int atapi_mode_sense(ide_t *dev)
{
	unsigned char buf[2048/*21*/], pkt[12] =
	{
		0x5A, 0, 0x2A,
		0, 0, 0, 0,
		0, 24,//21 //21 + 8,
		0, 0, 0
	};
	request_t req;

	DEBUG(printf("atapi_mode_sense:\n");)
	memset(&req, 0, sizeof(req));
	req.dev = dev;
//	req.num_blks = 1;
	req.buf = buf;
	(void)atapi_cmd(&req, pkt);
	dump(buf + 8, 21);

	printf("\nmulti-session:\t\t%s\n",
		(buf[12] & 0x40) ? "YES" : "NO");
	printf("mode 2/form 2:\t\t%s\n",
		(buf[12] & 0x20) ? "YES" : "NO");
	printf("mode 2/form 1:\t\t%s\n",
		(buf[12] & 0x10) ? "YES" : "NO");
	printf("XA:\t\t\t%s\n",
		(buf[12] & 0x02) ? "YES" : "NO");
	printf("audio play:\t\t%s\n",
		(buf[12] & 0x01) ? "YES" : "NO");

	printf("UPC:\t\t\t%s\n",
		(buf[13] & 0x40) ? "YES" : "NO");
	printf("ISRC:\t\t\t%s\n",
		(buf[13] & 0x20) ? "YES" : "NO");
	printf("C2 pointers:\t\t%s\n",
		(buf[13] & 0x10) ? "YES" : "NO");
	printf("R-W deinterleave and\n  correction:\t\t%s\n",
		(buf[13] & 0x08) ? "YES" : "NO");
	printf("R-W:\t\t\t%s\n",
		(buf[13] & 0x04) ? "YES" : "NO");
	printf("accurate CD-DA stream:\t%s\n",
		(buf[13] & 0x02) ? "YES" : "NO");
	printf("CD-DA commands:\t\t%s\n",
		(buf[13] & 0x01) ? "YES" : "NO");

	printf("loading mechanism type:\t%u\n",
		buf[14] >> 5);
	printf("eject:\t\t\t%s\n",
		(buf[14] & 0x08) ? "YES" : "NO");
	printf("prevent jumper:\t\t%s\n",
		(buf[14] & 0x04) ? "YES" : "NO");
	printf("lock state:\t\t%s\n",
		(buf[14] & 0x02) ? "YES" : "NO");
	printf("lock:\t\t\t%s\n",
		(buf[14] & 0x01) ? "YES" : "NO");

	printf("separate channel:\t%s\n",
		(buf[15] & 0x02) ? "YES" : "NO");
	printf("separate volume:\t%s\n",
		(buf[15] & 0x01) ? "YES" : "NO");
	printf("max speed:\t\t%u KBps\n", read_be16(buf + 16));
	printf("volume levels:\t\t%u\n", read_be16(buf + 18));
	printf("buffer size:\t\t%uK\n", read_be16(buf + 20));
	printf("current speed:\t\t%u KBps\n", read_be16(buf + 22));

	return 0;
}
/*****************************************************************************
*****************************************************************************/
int atapi_toc_ent(ide_t *dev, unsigned char *toc_ent,
		unsigned short toc_ent_len)
{
	unsigned char pkt[12] =
	{
		0x43,	/* ATAPI_CMD_READTOC */
		2, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0
	};
	request_t req;

	DEBUG(printf("atapi_toc_ent\n");)
	memset(&req, 0, sizeof(req));
	req.dev = dev;
/* xxx - pass toc_ent_len in req, somehow */
//	req.num_blks = 1;
	req.buf = toc_ent;

	pkt[7] = toc_ent_len >> 8;
	pkt[8] = toc_ent_len;
	return atapi_cmd(&req, pkt);
}
/*****************************************************************************
*****************************************************************************/
#define	MAX_TRACKS	32

typedef struct /* one frame == 1/75 sec */
{
	unsigned char min, sec, frame;
} msf_t;

int atapi_toc(ide_t *dev)
{
	unsigned char *entry, buf[4 + 8 * MAX_TRACKS];
	int temp;
	msf_t track[MAX_TRACKS];
	unsigned num_tracks;

	DEBUG(printf("atapi_toc:\n");)
/* read just the 4-byte header at first
    16-bit table-of-contents length (?)
    8-bit first track
    8-bit last track */
	temp = 4;
	temp = atapi_toc_ent(dev, buf, temp);
	if(temp != 0)
		return temp;
	num_tracks = buf[3] - buf[2] + 1;
	if(num_tracks <= 0 || num_tracks > 99)
	{
		printf("atapi_toc: error: bad number of tracks %d\n",
			num_tracks);
		return -1;
	}
	if(num_tracks > MAX_TRACKS)
	{
		printf("atapi-toc: warning: too many tracks (%u); "
			"reducing to %u.\n", num_tracks, MAX_TRACKS);
		num_tracks = MAX_TRACKS;
	}
/* read 4-byte header and 8-byte table-of-contents entries */
	temp = 4 + 8 * (num_tracks + 1);
	temp = atapi_toc_ent(dev, buf, temp);
	if(temp != 0)
		return temp;
/* point to first TOC entry */
	entry = buf + 4;
/* read num_tracks+1 entries
the last entry is for the disk lead-out */
	for(temp = 0; temp < num_tracks + 2; temp++)
	{
/* get track start time (minute, second, frame) from bytes 5..7 of entry */
		track[temp].min = entry[5];
		track[temp].sec = entry[6];
		track[temp].frame = entry[7];
		printf("%02u:%02u:%02u  ", track[temp].min,
			track[temp].sec, track[temp].frame);
/* advance to next entry */
		entry += 8;
	}
	printf("\n");
	return 0;
}
//////////////////////////////////////////////////////////////////////////////
// DMA ROUTINES
//////////////////////////////////////////////////////////////////////////////
#define	NUM_DMAS	8
#define	DMA_READ	0x44
#define	DMA_WRITE	0x48

/* 8-bit DMAC registers */
#define	DMA_BASE	0
#define	DMA_MASK	0x0A	/* mask register (write only) */
#define	DMA_MODE	0x0B	/* mode register */
#define	DMA_FF		0x0C	/* MSB/LSB flip flop */
#define	DMA_RESET	0x0D	/* master clear */

/* Upper 4 bits (8 bits ?) of 20-bit (24-bit?) address
come from the DMAC page registers: */
const unsigned char _page_reg_ioadr[NUM_DMAS] =
{
	0x87, 0x83, 0x81, 0x82, 0, 0x8B, 0x89, 0x8A
};
/****************************************************************************
****************************************************************************/
int dma_read(unsigned char chan, unsigned long linear,
		unsigned long tc)
{
	unsigned char scaled_chan, cmd;

/* validate chan */
	if(chan > 3)
		return -1;
/* set cmd byte  */
	cmd = DMA_READ + (chan & 3);
/* make sure transfer doesn't exceed 16 meg or cross a 64K boundary */
	if(linear + tc >=  0x1000000L)
		return -1;
	if((linear & 0x10000L) != ((linear + tc) & 0x10000L))
		return -1;
/* 8-bit transfers */
	scaled_chan = chan << 1;
	outportb(DMA_FF, 0);		/* set base adr */
	outportb(DMA_BASE + scaled_chan, linear);
	outportb(DMA_BASE + scaled_chan, linear >> 8);
	outportb(DMA_BASE + 1 +		/* set xfer len */
		scaled_chan, tc);
	outportb(DMA_BASE + 1 + scaled_chan, tc >> 8);
	outportb(_page_reg_ioadr[chan],	/* set page */
		linear >> 16);
	outportb(DMA_MODE, cmd);	/* do it */
	outportb(DMA_MASK, chan);
	return 0;
}
/****************************************************************************
****************************************************************************/
int dma_write(unsigned char chan, unsigned long linear,
		unsigned long tc)
{
	unsigned char scaled_chan, cmd;

/* validate chan */
	if(chan > 3)
		return -1;
/* set cmd byte  */
	cmd = DMA_WRITE + (chan & 3);
/* make sure transfer doesn't exceed 16 meg or cross a 64K boundary */
	if(linear + tc >=  0x1000000L)
		return -1;
	if((linear & 0x10000L) != ((linear + tc) & 0x10000L))
		return -1;
/* 8-bit transfers */
	scaled_chan = chan << 1;
	outportb(DMA_FF, 0);		/* set base adr */
	outportb(DMA_BASE + scaled_chan, linear);
	outportb(DMA_BASE + scaled_chan, linear >> 8);
	outportb(DMA_BASE + 1 +		/* set xfer len */
		scaled_chan, tc);
	outportb(DMA_BASE + 1 + scaled_chan, tc >> 8);
	outportb(_page_reg_ioadr[chan],	/* set page */
		linear >> 16);
	outportb(DMA_MODE, cmd);	/* do it */
	outportb(DMA_MASK, chan);
	return 0;
}
//////////////////////////////////////////////////////////////////////////////
// FLOPPY ROUTINES
//////////////////////////////////////////////////////////////////////////////
/* offsets from 0x3F0 */
#define FDC_DOR		2	/* Digital Output Register */
#define FDC_MSR		4	/* Main Status Register */
#define FDC_DATA	5	/* Data Register */

#define	FDC_DIR		7	/* Digital Input Register (input) */
#define FDC_CCR		7	/* Configuration Control Register (output) */

typedef struct
{
/* generic block device info */
	blkdev_t blkdev;
/* information specific to floppy drive */
	unsigned short timeout;
	unsigned char curr_track, sr0;
} floppy_t;
/*****************************************************************************
floppy controller chips:

Intel 8272a	==NEC 765
NEC 765ED	Non-Intel 1MB-compatible FDC, can't detect
Intel 87072	==8272a + FIFO + DUMPREGS
82072a		only on Sparcs?
82077AA		no LOCK (?)
82077AA-1
82078		44-pin 82078 or 64-pin 82078SL
82078-1		2 Mbps
S82078B		first seen on Adaptec AVA-2825 VLB SCSI/EIDE/Floppy ctrl
87306
*****************************************************************************/
void floppy_send_byte(unsigned short ioadr, unsigned char byte)
{
	unsigned char msr;
	unsigned timeout;

	for(timeout = 128; timeout != 0; timeout--)
	{
		msr = inportb(ioadr + FDC_MSR);
		if((msr & 0xC0) == 0x80)
		{
			outportb(ioadr + FDC_DATA, byte);
			return;
		}
	}
/* xxx - else? */
}
/*****************************************************************************
*****************************************************************************/
int floppy_get_byte(unsigned short ioadr)
{
	unsigned char msr;
	unsigned timeout;

	for(timeout = 128; timeout != 0; timeout--)
	{
		msr = inportb(ioadr + FDC_MSR);
		if((msr & 0xD0) == 0xD0)
			return inportb(ioadr + FDC_DATA);
	}
	return -1; /* timeout */
}
/*****************************************************************************
*****************************************************************************/
void floppy_motor_on(floppy_t *dev)
{
	unsigned short ioadr, t;
	unsigned char unit;

	DEBUG(printf("floppy_motor_on:\n");)
	ioadr = dev->blkdev.io.adr[0];
	unit = dev->blkdev.unit;
	t = dev->timeout;
/* cancel motor-off timeout.
-1 tells timer interrupt not to decrement  */
	dev->timeout = -1;
	if(t != 0)
		return; /* motor already on */
/* xxx - top four bits enable motors, b3 enables DMA and I/O,
b2 enables the the FDC chip, b1 and b0 are drive select

xxx - need a static global variable to cache the value written to FDC_DOR

xxx - floppy_select_drive() should be a separate function, maybe */
	outportb(ioadr + FDC_DOR, 0x1C);//unit?);
/* delay 500ms for motor to spin up
xxx - use interrupts here? */
	msleep(500);
}
/*****************************************************************************
*****************************************************************************/
void floppy_motor_off(floppy_t *dev)
{
	DEBUG(printf("floppy_motor_off:\n");)
	dev->timeout = 36; /* 18 ticks, ~2 sec */
}
/*****************************************************************************
*****************************************************************************/
int floppy_await_interrupt(floppy_t *fdev)
{
	unsigned short irq, ioadr;
	unsigned char status[7];
	int temp;

	ioadr = fdev->blkdev.io.adr[0];
	irq = fdev->blkdev.io.irq;
	DEBUG(printf("floppy_await_interrupt:\n");)
/* wait up to 1 second for IRQ 6 */
	temp = await_interrupt(irq, 1000);
	if(temp == 0) /* timeout */
	{
		printf("floppy_await_interrupt: timeout\n");
		return -1;
	}
/* read in command result bytes */
	for(temp = 0; temp < 7; temp++)
	{
		if((inportb(ioadr + FDC_MSR) & 0x10) == 0)
			break;
		status[temp] = floppy_get_byte(ioadr);
	}
/* perform "sense interrupt" */
	floppy_send_byte(ioadr, 0x08); /* CMD_SENSEI */
	fdev->sr0 = floppy_get_byte(ioadr);
	fdev->curr_track = floppy_get_byte(ioadr);

/* */
#if 1
	DEBUG(printf("status byte returned by floppy controller: 0x%X\n",
		status[0]);)
#else
DEBUG(
	printf("status returned by floppy controller:\n");
		printf("drive %u, head %u\n", status[0] & 3,
			(status[0] >> 2) & 1);
	printf("not ready on R/W or SS access to head 1: %u\n",
		(status[0] >> 3) & 1);
	printf("equipment check: %u\n",
		(status[0] >> 4) & 1);
	printf("seek complete: %u\n",
		(status[0] >> 5) & 1);
	printf("last command status: ");
	switch(status[0] >> 6)
	{
		case 0:	printf("success\n");	break;
		case 1: printf("started OK but abnormal termination\n"); break;
		case 2: printf("invalid command issued\n"); break;
		case 3: printf("abnormal termination because READY changed state\n"); break;
	}
)
#endif
	return status[0];
}
/*****************************************************************************
*****************************************************************************/
int floppy_seek(floppy_t *dev, unsigned char track)
{
	unsigned short ioadr;

	DEBUG(printf("floppy_seek: seeking track %u\n", track);)
/* already there? */
	if(dev->curr_track == track)
		return 0;
/* turn on the motor */
	floppy_motor_on(dev);
/* send the actual command bytes */
	ioadr = dev->blkdev.io.adr[0];
	floppy_send_byte(ioadr, 0x0F);	/* CMD_SEEK */
	floppy_send_byte(ioadr, 0);
	floppy_send_byte(ioadr, track);
/* wait until seek finished */
	if(floppy_await_interrupt(dev) < 0)
		return -1;
/* now let head settle for 15ms
xxx - use interrupt */
	msleep(15);
	DEBUG(printf("floppy_seek: sr0=0x%X (should be 0x20), curr_track=%u "
		"(should be %u)\n", dev->sr0, dev->curr_track, track);)
	floppy_motor_off(dev);
/* check that seek worked */
	if(dev->sr0 != 0x20 || dev->curr_track != track)
		return -1;
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int floppy_recalibrate(floppy_t *fdev)
{
	unsigned short ioadr;

	DEBUG(printf("floppy_recalibrate:\n");)
	ioadr = fdev->blkdev.io.adr[0];
	floppy_send_byte(ioadr, 0x07);
	floppy_send_byte(ioadr, 0);
	return floppy_await_interrupt(fdev) < 0;
}
/*****************************************************************************
*****************************************************************************/
int floppy_error(request_t *req)
{
	floppy_t *fdev;
	unsigned short ioadr;

	DEBUG(printf("*** floppy_error: ***\n");)
/* increment req->errors */
	(req->errors)++;
	if(req->errors >= 3)
		return -1;
/* */
	fdev = (floppy_t *)req->dev;
	ioadr = fdev->blkdev.io.adr[0];
/* if disk change, seek track 1 (xxx - one? not zero?) */
	if(inportb(ioadr + FDC_DIR) & 0x80)
	{
		DEBUG(printf("floppy_error: disk change\n");)
		if(floppy_seek(fdev, 1) != 0)
		{
/* if that fails, we're screwed... */
			floppy_motor_off(fdev);
			return -1;
		}
	}
	return floppy_recalibrate(fdev);
}
/*****************************************************************************
*****************************************************************************/
int floppy_rw(request_t *req)
{
	unsigned short ioadr, sector, head, num_bytes, num_blks;
	unsigned long blk, adr;
	blkdev_t *blkdev;
	floppy_t *fdev;
	int temp;

	DEBUG(printf("floppy_rw:\n");)
	fdev = (floppy_t *)req->dev;
/* if floppy_t struct had a magic value in it, we could validate it here */
	blkdev = &fdev->blkdev;
/* select drive and spin up the disk */
	floppy_motor_on(fdev);
	ioadr = blkdev->io.adr[0];
	do
	{
		blk = req->blk;
		sector = blk % blkdev->sectors + 1;
		blk /= blkdev->sectors;
		head = blk % blkdev->heads;
		blk /= blkdev->heads;
		DEBUG(printf("floppy_rw: CHS=%lu:%u:%u\n", blk, head,
			sector);)
/* do not cross a track boundary */
		num_blks = min(req->num_blks, blkdev->sectors -
			req->blk % blkdev->sectors);
		num_bytes = num_blks * blkdev->bytes_per_blk;
		DEBUG(printf("floppy_rw: transferring %u blocks (%u bytes)\n",
			num_blks, num_bytes);)
/* disk change? */
		if(inportb(ioadr + FDC_DIR) & 0x80)
			goto ERR;
/* seek to correct track */
		if(floppy_seek(fdev, blk) != 0)
			goto ERR;
/* program data rate (500K/s) */
		outportb(ioadr + FDC_CCR, 0);
/* copy data to 64K-aligned conventional memory buffer */
		if(req->cmd == BLK_CMD_WRITE)
			movedata(_DS, (unsigned)req->buf,
				LMEM_SEG, LMEM_OFF,
				num_bytes);
#if defined(__TURBOC__)
		adr = LMEM_SEG;
		adr <<= 4;
		adr += LMEM_OFF;
#elif defined(__DJGPP__)
		adr = LMEM_OFF;
#endif
		if(req->cmd == BLK_CMD_READ)
		{
			DEBUG(printf("floppy_rw: read\n");)
			floppy_send_byte(ioadr, 0xE6);//CMD_READ);
/* xxx - 2 is DMA number. Can't use blkdev->io.dma; it is a mask
Dammit, I forgot to subtract one here to convert length into terminal count.
Made that error with my sound software, too. */
			if(dma_read(2, adr, num_bytes - 1) != 0)
			{
				printf("floppy_rw: error from dma_read()\n");
				goto ERR;
			}
		}
		else// if(req->cmd == BLK_CMD_WRITE)
		{
			DEBUG(printf("floppy_rw: write\n");)
			floppy_send_byte(ioadr, 0xC5);//CMD_WRITE);
			if(dma_write(2, adr, num_bytes - 1) != 0)
			{
				printf("floppy_rw: error from dma_write()\n");
				goto ERR;
			}
		}
/* head and drive select */
		floppy_send_byte(ioadr, (head << 2) | blkdev->unit);
/* C, H, S, N */
		floppy_send_byte(ioadr, blk);
		floppy_send_byte(ioadr, head);
		floppy_send_byte(ioadr, sector);
		floppy_send_byte(ioadr, 2);	/* 512 bytes/sector */
/* EOT */
		floppy_send_byte(ioadr, blkdev->sectors);
/* gap 3 length for 1.44M or 1.68M floppies */
		if(blkdev->sectors == 18)
			floppy_send_byte(ioadr, 0x1B);
		else //if(blkdev->sectors == 21)
			floppy_send_byte(ioadr, 0x1C);
		floppy_send_byte(ioadr, 0xFF);	/* DTL = unused */
/* wait up to 1 second for IRQ 6 */
		temp = floppy_await_interrupt(fdev);
		if((temp < 0) || (temp & 0xC0))
		{
ERR:
			temp = floppy_error(req);
/* if floppy_error() returns non-zero, then we are doomed... */
			if(temp != 0)
				return temp;
/* ...else try again */
			else
				continue;
		}
/* copy data from 64K-aligned conventional memory buffer */
		if(req->cmd == BLK_CMD_READ)
			movedata(LMEM_SEG, LMEM_OFF,
				_DS, (unsigned)req->buf,
				num_bytes);
/* advance pointers */
		req->num_blks -= num_blks;
		req->blk += num_blks;
		req->buf += num_bytes;
	} while (req->num_blks != 0);
	floppy_motor_off(fdev);
	return 0;
}
//////////////////////////////////////////////////////////////////////////////
// MAIN/DEMO ROUTINES
//////////////////////////////////////////////////////////////////////////////
/*****************************************************************************
*****************************************************************************/
void INTERRUPT irq6(void)
{
	_interrupt_occurred |= 0x0040;
	outportb(0x20, 0x20);
}
/*****************************************************************************
*****************************************************************************/
void INTERRUPT irq14(void)
{
	_interrupt_occurred |= 0x4000;
	outportb(0xA0, 0x20);
	outportb(0x20, 0x20);
}
/*****************************************************************************
*****************************************************************************/
void INTERRUPT irq15(void)
{
	_interrupt_occurred |= 0x8000;
	outportb(0xA0, 0x20);
	outportb(0x20, 0x20);
}
/*****************************************************************************
*****************************************************************************/
#define	_18K	18432
#define FLOPPY	1

#if defined(__TURBOC__)
#define	LEN	(32768u + 2048)
#elif defined(__DJGPP__)
#define	LEN	92160u
#endif


unsigned char _big_buf[LEN];

int main(void)
{
	vector_t vector6, vector14, vector15;
	request_t freq, creq;
	floppy_t fdev;
	unsigned temp, foo;
	unsigned char buf[_18K];

	memset(&fdev, 0, sizeof(fdev));
	fdev.blkdev.io.dma = 0x04;
	fdev.blkdev.io.irq = 0x0040;
	fdev.blkdev.io.adr[0] = 0x3F0;
/* 8 bytes at 0x3F0
xxx - steer clear of 0x3F6? Win95 says 0x3F2-0x3F5, but we need 0x3F7 too */
	fdev.blkdev.io.span[0] = 8;

//	fdev.blkdev.unit = ?;
	fdev.blkdev.num_blks = 2880;
	fdev.blkdev.bytes_per_blk = 512;
	fdev.blkdev.sectors = 18; /* xxx - probe this, somehow */
	fdev.blkdev.heads = 2;
	fdev.blkdev.cyls = 80;
	fdev.curr_track = -1;

	memset(&creq, 0, sizeof(creq));
	creq.cmd = BLK_CMD_READ;
	creq.dev = _drives + 1;

	memset(&freq, 0, sizeof(freq));
	freq.dev = &fdev;
/* */
	SAVE_VECT(14, vector6);
	SAVE_VECT(118, vector14);
	SAVE_VECT(119, vector15);
	SET_VECT(14, irq6);
	SET_VECT(118, irq14);
	SET_VECT(119, irq15);
/* */
	ide_probe();
/* */
#if 1
/******
check if CD-ROM drive can rip audio
look for "CD-DA commands: YES"
xxx - not sure about this
******/
	(void)atapi_mode_sense(_drives + 1);
#elif 0
/******
this is both incomplete and untested, so don't use it
1 block of CD-DA data = 2352 bytes = 1/75 sec of audio
******/
	creq.buf = buf;
	creq.num_blks = 1;
	(void)atapi_rip_audio(&creq);
#elif 1
/******
read from CD-ROM
******/
	creq.buf = buf;
	creq.num_blks = 9;
	creq.blk = 16;
	(void)atapi_read_sectors(&creq);
	dump(buf, 128);
#elif 0
/******
read from floppy
******/
	foo = fdev.blkdev.sectors *	/* =18 */
		fdev.blkdev.heads;	/* =2 */
	freq.cmd = BLK_CMD_READ;
	freq.buf = buf;
	freq.num_blks = foo;
	freq.blk = 0;
	(void)floppy_rw(&freq);
	dump(buf, 128);
#elif 0
/******
copy data from CD-ROM to disk file
******/
	creq.buf = _big_buf;
	creq.num_blks = LEN / 2048;
	creq.blk = 0;
	if(atapi_read_sectors(&creq) != 0)
	{
		printf("atapi_read_sectors failed\n");
		goto ERR;
	}
	RESTORE_VECT(14, vector6);
	RESTORE_VECT(118, vector14);
	RESTORE_VECT(119, vector15);
	{
		FILE *out;

		out = fopen("data.bin", "wb");
		if(out == NULL)
			exit(0);
		fwrite(_big_buf, 1, LEN, out);
		fclose(out);
		exit(0);
	}
#elif 0
/******
copy data from floppy to disk file
******/
	freq.buf = _big_buf;
	freq.num_blks = LEN / 512;
	freq.blk = 0;
	freq.cmd = BLK_CMD_READ;
	if(floppy_rw(&freq) != 0)
	{
		printf("floppy_rw failed\n");
		goto ERR;
	}
	RESTORE_VECT(14, vector6);
	RESTORE_VECT(118, vector14);
	RESTORE_VECT(119, vector15);
	{
		FILE *out;

		out = fopen("data.bin", "wb");
		if(out == NULL)
			exit(0);
		fwrite(_big_buf, 1, LEN, out);
		fclose(out);
		exit(0);
	}
#elif 0
/******
copy data from CD-ROM to floppy
******/
	foo = fdev.blkdev.sectors *	/* =18 */
		fdev.blkdev.heads;	/* =2 */
	for(temp = 0; temp < 10; temp++)
	{
/* read 18K (nine 2K blocks) from CD-ROM */
		creq.buf = buf;
		creq.num_blks = 9;
		creq.blk = temp * creq.num_blks;
		if(atapi_read_sectors(&creq) != 0)
		{
			printf("atapi_read_sectors failed\n");
			break;
		}
/* write 18K (36 0.5K blocks; 1 track) to floppy */
		freq.cmd = BLK_CMD_WRITE;
		freq.buf = buf;
		freq.num_blks = foo;
		creq.blk = temp * freq.num_blks;
		if(floppy_rw(&freq) != 0)
		{
			printf("floppy_rw failed\n");
			break;
		}
		if(kbhit())
			break;
	}
#endif
ERR:
outportb(0x3F2, 0); /* turn off floppy motor(s) */
	RESTORE_VECT(14, vector6);
	RESTORE_VECT(118, vector14);
	RESTORE_VECT(119, vector15);
	return 0;
}
