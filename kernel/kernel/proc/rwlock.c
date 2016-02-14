#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/sync.h>



/*
 * RWInit();
 *
 * Initialize Readers/Writers lock.
 */

void RWInit (struct RWLock *rwlock)
{
	rwlock->i = 0;
	rwlock->readers_waiting = FALSE;
	
	MutexInit (&rwlock->mutex);
	CondInit (&rwlock->wr_cond);
	CondInit (&rwlock->rd_cond);
}





/*
 * RWWriteLock();
 *
 * Obtain "Exclusive" access to the rwlock.
 */
 
void RWWriteLock (struct RWLock *rwlock)
{
	MutexLock (&rwlock->mutex);

	while (rwlock->i != 0)
		CondWait (&rwlock->wr_cond, &rwlock->mutex);
	
	rwlock->i = -1;

	MutexUnlock (&rwlock->mutex);
}



/*
 * RWReadLock();
 *
 * Obtain "shared" access to the rwlock.
 */

void RWReadLock (struct RWLock *rwlock)
{
	MutexLock (&rwlock->mutex);

	while (rwlock->i < 0)
	{
		rwlock->readers_waiting = TRUE;
		CondWait (&rwlock->rd_cond, &rwlock->mutex);
	}
	
	rwlock->i ++;
	
	MutexUnlock (&rwlock->mutex);
}




/*
 * RWUnlock();
 *
 * Release/Decrement the rwlock.
 *
 * FIXME: Current algorithm means that readers can starve any writers
 * from accessing the lock.
 */
 
void RWUnlock (struct RWLock *rwlock)
{
	MutexLock (&rwlock->mutex);
	
	if (rwlock->i == -1)
	{
		rwlock->i = 0;
		
		if (rwlock->readers_waiting == TRUE)
		{
			rwlock->readers_waiting = FALSE;	
			CondBroadcast (&rwlock->rd_cond);
		}
		else
			CondSignal (&rwlock->wr_cond);
	}
	else if (rwlock->i > 0)
	{
		rwlock->i -= 1;
		
		if (rwlock->i == 0)
			CondSignal (&rwlock->wr_cond);
	}

	
	MutexUnlock (&rwlock->mutex);
}



