#include <kernel/types.h>
#include <kernel/sync.h>




/*
 *
 */
 
#define PROCFS_STR_SZ	79


/*
 */

struct ProcfsFilp
{
	struct Device *device;

	bool isdir;
	uint32 fpos;

	char buf[PROCFS_STR_SZ];
	int next_write;
	int reference_cnt;
};




/*
 */
 
struct ProcfsLookup
{
	char *pathname;
	
	struct Process *proc;
	int isdir;
	
};



extern struct Mutex procfs_mutex;


/*
 *
 */

int procfs_init (void *elf);
void *procfs_expunge (void);
int procfs_opendevice (int unit, void *ioreq, uint32 flags);
int procfs_closedevice (void *ioreq);
void procfs_beginio (void *ioreq);
int procfs_abortio (void *ioreq);


void ProcFSOpen (struct FSReq *fsreq);
void ProcFSDup (struct FSReq *fsreq);
void ProcFSClose (struct FSReq *fsreq);
void ProcFSOpenDir (struct FSReq *fsreq);
void ProcFSReadDir (struct FSReq *fsreq);
void ProcFSRewindDir (struct FSReq *fsreq);
void ProcFSRead (struct FSReq *fsreq);
void ProcFSSeek (struct FSReq *fsreq);
void ProcFSFStat (struct FSReq *fsreq);
void ProcFSStat (struct FSReq *fsreq);


int ProcFSLookup (struct ProcfsLookup *lookup);
