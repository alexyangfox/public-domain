#include <kernel/types.h>
#include <kernel/sync.h>




/*
 *
 */

#define PIPE_BUFFER_SZ	10000
#define PIPE_BUF		1000




/*
 *
 */

struct PipeFilp
{
	struct Device *device;
	struct Pipe *pipe;
	
	bool is_writer;
	int reference_cnt;
};




/*
 *
 */

struct Pipe
{
	struct Device *device;
	
	int reader_cnt;	
	int writer_cnt;
	
	int32 r_pos;
	int32 w_pos;
	int32 read_sz;
	int32 write_sz;
	
	struct Mutex mutex;
	struct Cond reader_cond;
	struct Cond writer_cond;
		
	uint8 *buf;
};




/*
 *
 */

int pipe_init (void *elf);
void *pipe_expunge (void);
int pipe_opendevice (int unit, void *ioreq, uint32 flags);
int pipe_closedevice (void *ioreq);
void pipe_beginio (void *ioreq);
int pipe_abortio (void *ioreq);
