#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/timer.h>
#include <kernel/dbg.h>
#include <kernel/i386/i386.h>



/*
 *
 */

void UDelay (uint32 usec)
{
	int t;
	
	for (t=0; t<usec; t++)
	{
		InByte (0x80);
	}

	/*
	struct MST ms;

	UStart (&ms);
	while (UElapsed(&ms) < usec);
	*/
}




/*
 *
 */

void UStart (struct MST *ms)
{
	uint32 count;
	uint32 int_state;
	
	int_state = DisableInterrupts();
	OutByte (TMR_TCW, TMRC_MODE0);
	count = InByte (TMR_TMRCNT0);
	count |= (InByte (TMR_TMRCNT0) << 8);
	RestoreInterrupts (int_state);
	
	ms->prev_count = count;
	ms->accum_count = 0;
	
}




/*
 *
 */

uint32 UElapsed (struct MST *ms)
{
	uint32 count;
	uint32 int_state;
    
	int_state = DisableInterrupts();
	OutByte (TMR_TCW,  TMRC_MODE0);
	count = InByte (TMR_TMRCNT0);
	count |= (InByte (TMR_TMRCNT0) << 8);
	RestoreInterrupts (int_state);
	
	ms->accum_count += (count <= ms->prev_count) ? (ms->prev_count - count) : 1;
	ms->prev_count = count;
    
    if (ms->accum_count <= 1)
    	return 0;
    	

    /* frac = 1000000 / 1193182; */
    /* multiplier = (2^32) * frac */
    
    return MullFrac (ms->accum_count - 1, 3599591091UL);
}















