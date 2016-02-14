#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/fs.h>
#include <kernel/block.h>



/*
 *
 */

#define ATA_CONFIG_NONE		0
#define ATA_CONFIG_UNKNOWN	1
#define ATA_CONFIG_ATA		2
#define ATA_CONFIG_ATAPI	3


#define ATA_BUFFER_SZ		65536
#define MAX_ATA_DRIVES		4
#define SECTOR_SIZE			512

#define MAX_ATA_TRIES		3
#define ATA_TIMEOUT			10

#define ATA_MEDIA_NOT_PRESENT	0
#define ATA_MEDIA_PRESENT		1




/*
 *
 */

/* Read and write registers */
#define REG_BASE0	0x1F0	/* base register of controller 0 */
#define REG_BASE1	0x170	/* base register of controller 1 */

#define REG_DATA	    0	/* data register (offset from the base reg.) */
#define REG_PRECOMP	    1	/* start of write precompensation */
#define REG_COUNT	    2	/* sectors to transfer */
#define REG_SECTOR	    3	/* sector number */
#define REG_CYL_LO	    4	/* low byte of cylinder number */
#define REG_CYL_HI	    5	/* high byte of cylinder number */
#define REG_LDH		    6	/* lba, drive and head */
#define   LDH_DEFAULT		0xA0	/* ECC enable, 512 bytes per sector */
#define   LDH_LBA			0x40	/* Use LBA addressing */
#define   ldh_init(drive)	(LDH_DEFAULT | ((drive) << 4))

/* Read only registers */
#define REG_STATUS	    7	/* status */
#define   STATUS_BSY		0x80	/* controller busy */
#define	  STATUS_RDY		0x40	/* drive ready */
#define	  STATUS_WF			0x20	/* write fault */
#define	  STATUS_SC			0x10	/* seek complete (obsolete) */
#define	  STATUS_DRQ		0x08	/* data transfer request */
#define	  STATUS_CRD		0x04	/* corrected data */
#define	  STATUS_IDX		0x02	/* index pulse */
#define	  STATUS_ERR		0x01	/* error */
#define	  STATUS_ADMBSY	    0x100	/* administratively busy (software) */
#define REG_ERROR	    1	/* error code */
#define	  ERROR_BB		0x80	/* bad block */
#define	  ERROR_ECC		0x40	/* bad ecc bytes */
#define	  ERROR_ID		0x10	/* id not found */
#define	  ERROR_AC		0x04	/* aborted command */
#define	  ERROR_TK		0x02	/* track zero error */
#define	  ERROR_DM		0x01	/* no data address mark */

#define REG_ASTAT		0x206	/* alternate status in */
#define REG_DC			0x206	/* device control out */


#define REG_ATAPI_STATUS	7
#define   ATAPI_STATUS_CHK	 0x001


/* Write only registers */
#define REG_COMMAND	    7	/* command */
#define   ATA_CMD_IDLE			0x00	/* for w_command: drive idle */
#define   ATA_CMD_RECALIBRATE	0x10	/* recalibrate drive */
#define   ATA_CMD_READ			0x20	/* read data */
#define   ATA_CMD_WRITE			0x30	/* write data */
#define   ATA_CMD_READVERIFY	0x40	/* read verify */
#define   ATA_CMD_FORMAT		0x50	/* format track */
#define   ATA_CMD_SEEK			0x70	/* seek cylinder */
#define   ATA_CMD_DIAG			0x90	/* execute device diagnostics */
#define   ATA_CMD_SPECIFY		0x91	/* specify parameters */
#define   ATA_CMD_IDENTIFY		0xEC	/* identify drive */
#define   ATA_CMD_IDENTIFY_PACKET_DEVICE	0xA1	/* identify packet device */
#define   ATA_CMD_PACKET                    0xA0	/* packet */

#define REG_CTL     0x206	/* control register */
#define   CTL_NORETRY		0x80	/* disable access retry */
#define   CTL_NOECC			0x40	/* disable ecc retry */
#define   CTL_HD15			0x00
#define   CTL_RESET			0x04	/* reset controller */
#define   CTL_NIEN			0x02	/* disable interrupts */



#define DELAY400NS			UDelay(1); 



/*
 *
 */

#define ERR				(-1)	/* general error */
#define ERR_BAD_SECTOR	(-2)	/* block marked bad detected */
#define ERR_TIMEOUT		(-3)	/* timeout */


/*
 * Macros for reading ATA Identify data block
 */

#define id_byte(n)	(&ata_buffer[2 * (n)])

#define id_word(n)	(((uint16) id_byte(n)[0] <<  0) \
						|((uint16) id_byte(n)[1] <<  8))
						
#define id_longword(n)	(((uint32) id_byte(n)[0] <<  0) \
						|((uint32) id_byte(n)[1] <<  8) \
						|((uint32) id_byte(n)[2] << 16) \
						|((uint32) id_byte(n)[3] << 24))


/*
 * struct Ata
 */

struct Ata
{
	int enabled;
	int config;
	
	int32 reference_cnt;

	uint32 state;		/* drive state: deaf, initialized, dead */
	uint32 base;		/* base register of the register file */
	
	int32 irq;			/* interrupt request line */
  	int32 isr_signal;
	uint32 lcylinders;	/* logical number of cylinders (BIOS) */
	uint32 lheads;		/* logical number of heads */
	uint32 lsectors;	/* logical number of sectors per track */
	uint32 pcylinders;	/* physical number of cylinders (translated) */
	uint32 pheads;		/* physical number of heads */
	uint32 psectors;	/* physical number of sectors per track */
	uint32 ldhpref;		/* top four bytes of the LDH (head) register */
	uint32 precomp;		/* write precompensation cylinder / 4 */
	uint32 max_count;	/* max request for this drive */
	uint32 open_ct;		/* in-use count */
	uint32 size;

	uint32 sector_sz;   /* Number of bytes per sector, 512 for HDs,  2048 for CDs */

	int atapi_protocol_type;	
	int atapi_type;
	int atapi_removable;
	int atapi_cmd_drq_type;
	int atapi_cmd_packet_size;

	uint8 atapi_error;
	
	int disk_state;
	int diskchange_cnt;
	
	struct Ata *device[2];
	
	struct Partition partition_table[4];
	
	uint32 partition_lba;
	
	LIST (Callback) callback_list;
};




/*
 *
 */
 
struct command
{
	uint8 precomp;	/* REG_PRECOMP, etc. */
	uint8 count;
	uint8 sector;
	uint8 cyl_lo;
	uint8 cyl_hi;
	uint8 ldh;
	uint8 command;
};






/*
 * ATAPI_IDENTIFY general configuration values
 */

#define ATAPI_PROTOCOL_ATAPI 	2

#define ATAPI_TYPE_DIRECT		0x00
#define ATAPI_TYPE_CDROM		0x05
#define ATAPI_TYPE_OPTICAL		0x07
#define ATAPI_TYPE_UNKNOWN		0x1F

#define ATAPI_REMOVABLE			1

#define ATAPI_CMD_DRQ_MICRO		0
#define ATAPI_CMD_DRQ_INTR		1
#define ATAPI_CMD_DRQ_ACCEL		2




/*
 * AtapiPacket() data transfer direction
 */

#define ATAPI_DIR_READ			0
#define ATAPI_DIR_WRITE			1


/* 
 * ATAPI Commands
 */
 
#define ATAPI_CMD_INQUIRY			0x12
#define ATAPI_CMD_LOADUNLOADCD		0xA6
#define ATAPI_CMD_MECHANISM_STATUS	0xBD
#define ATAPI_CMD_MODE_SELECT		0x55
#define ATAPI_CMD_MODE_SENSE		0x5A
#define ATAPI_CMD_PAUSERESUME		0x4B
#define ATAPI_CMD_PLAY_AUDIO		0x45
#define ATAPI_CMD_PLAY_CD			0xBC
#define ATAPI_CMD_REMOVAL			0x1E
#define ATAPI_CMD_READ				0x28
#define ATAPI_CMD_CDROM_CAPACITY	0x25
#define ATAPI_CMD_READ_CD			0xBE
#define ATAPI_CMD_READ_HEADER		0x44
#define ATAPI_CMD_READ_TOC			0x43
#define ATAPI_CMD_REQUEST_SENSE		0x03
#define ATAPI_CMD_SEEK				0x2B
#define ATAPI_CMD_STOP_PLAY			0x4E
#define ATAPI_CMD_STARTSTOP			0x1B
#define ATAPI_CMD_TEST_READY		0x00






/*
 * Variables
 */



extern struct Ata ata_drive[MAX_ATA_DRIVES];

extern bool ata_isr14_enabled;
extern bool ata_isr15_enabled;

extern struct ISRHandler *ata_isr14_handler;
extern struct ISRHandler *ata_isr15_handler;

extern int32 ata_isr14_signal;
extern int32 ata_isr15_signal;
extern int32 ata_alarm_signal;

extern int ata_init_error;
extern int ata_pid;

extern struct MsgPort *ata_msgport;
extern struct Device ata_device;

extern int ata_error;

extern struct Alarm ata_alarm;
extern bool ata_needs_reset;

extern bool use_interrupts;

extern uint8 *ata_buffer;

extern volatile uint8 ata_stored_status;	/* status after interrupt, saved
											so as to prevent clearing further
											interrupts.  May want to be per channel */

extern struct Timer ata_timer;
extern int32 ata_timer_signal;
extern struct TimeVal ata_timer_tv;

 

extern volatile int ata_isr14_cnt;
extern volatile int ata_isr15_cnt;


/*
 *
 */
 
int ata_init (void *elf);
void *ata_expunge (void);
int ata_opendevice (int unit, void *ioreq, uint32 flags);
int ata_closedevice (void *ioreq);
void ata_beginio (void *ioreq);
int ata_abortio (void *ioreq);




/*
 *
 */

int32 AtaTask (void *arg);
void DoAtaRead (struct BlkReq *blkreq);
void DoAtaWrite (struct BlkReq *blkreq);
void DoAtaMediaPresent (struct BlkReq *blkreq);
void DoAtaAddCallback (struct BlkReq *blkreq);
void DoAtaRemCallback (struct BlkReq *blkreq);
void DoAtaSCSICommand (struct BlkReq *blkreq);

/*
 *
 */

void AtaTaskInit (void);
void AtaTaskFini (void);
int AtaInitUnits(void);




/*
 *
 */

int AtaWaitFor (struct Ata *ata, uint8 mask, uint8 value);
int AtaIntrWait (struct Ata *ata);
int32 AtaISRHandler (int32 isr_idx, void *arg);

void AtaAtapiDelay (struct Ata *ata);




/*
 *
 */

void pio_insw (uint32 port, void *buf, int32 cnt);
void pio_insb (uint32 port, void *buf, int32 cnt);
void pio_outsw (uint32 port, void *buf, int32 cnt);
void pio_outsb (uint32 port, void *buf, int32 cnt);




/*
 *
 */

int AtaIdentify (struct Ata *ata, void *buf);
int AtaReadSectors (struct Ata *ata, int sector_cnt, uint32 block, void *buf, struct AddressSpace *as);
int AtaWriteSectors (struct Ata *ata, int sector_cnt, uint32 block, void *buf, struct AddressSpace *as);
int AtaPIODataIn (struct Ata *ata, struct command *cmd, void *buf, int cnt);
int AtaPIODataOut (struct Ata *ata, struct command *cmd, void *buf, int cnt);

int AtaReset (struct Ata *ata, int skip);
int AtaDetect (struct Ata *ata0, struct Ata *ata1);



/*
 *
 */

int AtapiMediaPresent (struct Ata *ata);

int AtapiReadSectors (struct Ata *ata, int sector_cnt, uint32 block, void *buf, struct AddressSpace *as);
int AtapiWriteSectors (struct Ata *ata, int sector_cnt, uint32 block, void *buf, struct AddressSpace *as);


int AtapiIdentify (struct Ata *ata, void *buf);
int AtapiPacket (struct Ata *ata, uint8 *cmd_packet, int cmd_packet_nbytes,
					void *data_buf, int data_nbytes, int dir, struct AddressSpace *as);
int CheckAtapiTimeout (void);
void AtapiReset (struct Ata *ata);





void AtaCallbacks (int unit);
void AtaTimerCallout (struct Timer *timer, void *arg);



