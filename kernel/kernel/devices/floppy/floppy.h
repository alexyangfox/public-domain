#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/fs.h>
#include <kernel/block.h>
#include <kernel/callback.h>




/* Floppy drive units */

#define MAX_FLOPPY_DRIVES	2


/* I/O Ports used by floppy disk task. */
#define DOR            0x3F2	/* motor drive control bits */
#define FDC_STATUS     0x3F4	/* floppy disk controller status register */
#define FDC_DATA       0x3F5	/* floppy disk controller data register */
#define FDC_RATE       0x3F7	/* transfer rate register */
#define FDC_DIR        0x3F7    /* Data Input Register/DiskChange 0x80 */

#define DMA_ADDR       0x004	/* port for low 16 bits of DMA address */
#define DMA_TOP        0x081	/* port for top 4 bits of 20-bit DMA addr */
#define DMA_COUNT      0x005	/* port for DMA count (count =  bytes - 1) */
#define DMA_FLIPFLOP   0x00C	/* DMA byte pointer flip-flop */
#define DMA_MODE       0x00B	/* DMA mode port */
#define DMA_INIT       0x00A	/* DMA init port */
#define DMA_RESET_VAL   0x06

/* Status registers returned as result of operation. */
#define ST0             0x00	/* status register 0 */
#define ST1             0x01	/* status register 1 */
#define ST2             0x02	/* status register 2 */
#define ST3             0x00	/* status register 3 (return by DRIVE_SENSE) */
#define ST_CYL          0x03	/* slot where controller reports cylinder */
#define ST_HEAD         0x04	/* slot where controller reports head */
#define ST_SEC          0x05	/* slot where controller reports sector */
#define ST_PCN          0x01	/* slot where controller reports present cyl */

/* Fields within the I/O ports. */
/* Main status register. */
#define CTL_BUSY        0x10	/* bit is set when read or write in progress */
#define DIRECTION       0x40	/* bit is set when reading data reg is valid */
#define MASTER          0x80	/* bit is set when data reg can be accessed */

/* Digital output port (DOR). */
#define MOTOR_SHIFT        4	/* high 4 bits control the motors in DOR */
#define ENABLE_INT      0x0C	/* used for setting DOR port */
#define DOR_DMA         0x08


/* Digital Input Port (DIR) */

#define DISK_CHANGE		0x80	/* A disk change has occured */

/* ST0. */
#define ST0_BITS        0xF8	/* check top 5 bits of seek status */
#define TRANS_ST0       0x00	/* top 5 bits of ST0 for READ/WRITE */
#define SEEK_ST0        0x20	/* top 5 bits of ST0 for SEEK */

/* ST1. */
#define BAD_SECTOR      0x05	/* if these bits are set in ST1, recalibrate */
#define WRITE_PROTECT   0x02	/* bit is set if diskette is write protected */

/* ST2. */
#define BAD_CYL         0x1F	/* if any of these bits are set, recalibrate */

/* ST3 (not used). */
#define ST3_FAULT       0x80	/* if this bit is set, drive is sick */
#define ST3_WR_PROTECT  0x40	/* set when diskette is write protected */
#define ST3_READY       0x20	/* set when drive is ready */

/* Floppy disk controller command bytes. */
#define FDC_SEEK        0x0F	/* command the drive to seek */
#define FDC_READ        0xE6	/* command the drive to read */
#define FDC_WRITE       0xC5	/* command the drive to write */
#define FDC_SENSE_INT   0x08	/* command the controller to tell its status */
#define FDC_RECALIBRATE 0x07	/* command the drive to go to cyl 0 */
#define FDC_SPECIFY     0x03	/* command the drive to accept params */
#define FDC_READ_ID     0x4A	/* command the drive to read sector identity */
#define FDC_FORMAT      0x4D	/* command the drive to format a track */
#define FDC_SENSE_DRV	0x04	/* obtain drive status (write-protect) state */

/* DMA channel commands. */
#define DMA_READ        0x46	/* DMA read opcode */
#define DMA_WRITE       0x4A	/* DMA write opcode */

/* Parameters for the disk drive. */
#define HC_SIZE         2880	/* # sectors on largest legal disk (1.44MB) */
#define NR_HEADS        0x02	/* two heads (i.e., two tracks/cylinder) */
#define MAX_SECTORS	  18	/* largest # sectors per track */
#define DTL             0xFF	/* determines data length (sector size) */
#define SPEC2           0x02	/* second parameter to SPECIFY */
#define MOTOR_OFF      (3*HZ)	/* how long to wait before stopping motor */
#define WAKEUP	       (2*HZ)	/* timeout on I/O, FDC won't quit. */


/* No retries on some errors. */
#define err_no_retry(err)	((err) <= ERR_WR_PROTECT)

/* Encoding of drive type in minor device number. */
#define DEV_TYPE_BITS   0x7C	/* drive type + 1, if nonzero */
#define DEV_TYPE_SHIFT     2	/* right shift to normalize type bits */
#define FORMAT_DEV_BIT  0x80	/* bit in minor to turn write into format */

/* Miscellaneous. */
#define MAX_ERRORS         6	/* how often to try rd/wt before quitting */
#define MAX_RESULTS        7	/* max number of bytes controller returns */
#define unitS          2	/* maximum number of drives */
#define DIVISOR          128	/* used for sector size encoding */
#define SECTOR_SIZE_CODE   2	/* code to say "512" to the controller */
#define TIMEOUT	      500000L	/* microseconds waiting for FDC */
#define UNCALIBRATED       0	/* drive needs to be calibrated at next use */
#define CALIBRATED         1	/* no calibration needed */
#define BASE_SECTOR        1	/* sectors are numbered starting at 1 */
#define NO_SECTOR        (-1)	/* current sector unknown */
#define NO_CYL		 (-1)	/* current cylinder unknown, must seek */
#define NO_DENS		 100	/* current media unknown */
#define BSY_IDLE	   0	/* busy doing nothing */
#define BSY_IO		   1	/* busy doing I/O */
#define BSY_WAKEN	   2	/* got a wakeup call */


#define FLOPPY_NOT_PRESENT	0
#define FLOPPY_PRESENT		1



#define FLOPPY_DATA_RETRIES		20

#define FLOPPY_READ				0
#define FLOPPY_WRITE			1





struct Floppy
{
	int32 curcyl;
		
	int8 calibration;		/* CALIBRATED or UNCALIBRATED */
	int error;
	int media_unchecked;
	
	/* Can move result to global */
	uint8 result[MAX_RESULTS];
	
	int32 geo_cyl;			/* Geometry of drive? */
	int32 geo_spt;
	int32 geo_heads;
	int32 density;			/* NO_DENS = ?, 0 = 360K; 1 = 360K/1.2M; etc.*/

	bool motor_active;
	int32 disk_state;
	uint32 diskchange_cnt;
	int write_protect;
	
	int motor_off_delay;
	
	int unit;
	bool drive_enabled;
	int reference_cnt;
	
	LIST (Callback) callback_list;
};










/* Only 1.44MB and 720k 3.5 disks are supported and only determined by CMOS settings.
 *
 * 0  1.44M   1.44M    18	    80     300 RPM  500 kbps   PS/2, et al.
 * 1  720K     720K     9       80     300 RPM  250 kbps   Toshiba, et al.
 */
 
#define NT 2
extern int gap[NT];						/* Gap size */
extern int rate[NT];					/* Data rate 0=500, 1=200 */
extern int nr_sectors[NT];				/* sectors_per_track */
extern int nr_blocks[NT];				/* total sectors on disk */
extern int nr_cylinders[NT];			/* cylinders on disk */
extern struct TimeVal mtr_start[NT];	/* Motor spin up wait */
extern char spec1[NT];					/* step rate etc */


/* ????????????????? Got to set this properly */
extern int floppy_d;			/* diskette/drive combination  DENSITY Set in FloppyDoReadWrite ??? */
							/* Used as an index into above arrays?   Why is it not stored
								in the floppy structure?
								*/
extern int current_spec1;
extern bool floppy_needs_reset;
extern int32 floppy_busy;

extern struct ISRHandler *floppy_isr_handler;
extern int32 floppy_isr_signal;

extern struct Alarm floppy_alarm;
extern int32 floppy_alarm_signal;


extern struct Floppy floppy_drive[MAX_FLOPPY_DRIVES];

extern uint8 *floppy_buffer;
extern int32 floppy_buffer_unit;
extern int32 floppy_buffer_c;
extern int32 floppy_buffer_h;


extern int floppy_pid;
extern int floppy_init_error;

extern struct TimeVal floppy_motor_off_tv;
extern struct Timer floppy_timer;
extern int32 floppy_timer_signal;


extern struct MsgPort *floppy_msgport;
extern struct Device floppy_device;







/*
 *
 */
 
int floppy_init (void *elf);
void *floppy_expunge (void);
int floppy_opendevice (int unit, void *ioreq, uint32 flags);
int floppy_closedevice (void *ioreq);
void floppy_beginio (void *ioreq);
int floppy_abortio (void *ioreq);


/*
 *
 */

int32 FloppyTask (void *arg);
void FloppyTaskInit (void);
void FloppyTaskFini (void);

int FloppyInitUnits (void);

int32 FloppyISRHandler (int32 isr_idx, void *arg);

int32 WaitForFloppyInterrupt (void);
void StopMotor (int32 unit);
void StartMotor (int32 unit, bool wait);
int32 FloppySendByte (uint8 val);
int32 FloppyGetByte (uint8 *val);
int32 FloppyCommand (uint8 *cmd, int32 nbytes);
int32 FloppyGetResult (int32 unit, int32 nbytes);
void FloppyReset (void);
int32 FloppyReadID (int32 unit, uint32 head);
int32 FloppySeek (int32 unit, int32 head, int32 cylinder);
int32 FloppyRecalibrate (int32 unit);
void FloppyReadyDMA(int mode, int32 unit, int sector_cnt, void *buf);
int32 FloppyTransfer (int32 opcode, int32 unit, int32 cylinder, int32 head,
							int32 start_sector, int32 sector_cnt);
int32 FloppyBuffer (int32 direction, int32 unit, int32 cylinder, int32 head);

int32 FlushTrackBuffer (int32 unit);
void FloppySetTimer (int32 unit);
void FloppyTimerCallout (struct Timer *timer, void *arg);


void FloppyCmdOpenDevice (struct BlkReq *blkreq);
void FloppyCmdCloseDevice (struct BlkReq *blkreq);

void FloppyCmdReadWrite (struct BlkReq *blkreq);
int FloppyDoReadWrite (int cmd, struct AddressSpace *as, void *addr,
							int sector_cnt, int unit, int c, int h, int s);

void FloppyCmdRevalidate (struct BlkReq *blkreq);
void FloppyCmdMediaPresent (struct BlkReq *blkreq);

void FloppyCmdAddCallback (struct BlkReq *blkreq);
void FloppyCmdRemCallback (struct BlkReq *blkreq);




int32 FloppyCheckMediaPresent (int unit);

int32 FloppySenseDrive (int32 unit, uint32 *status);


void FloppyCallbacks (int unit);



