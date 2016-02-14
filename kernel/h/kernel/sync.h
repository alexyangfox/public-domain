#ifndef KERNEL_SYNC_H
#define KERNEL_SYNC_H

#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/timer.h>


struct Mutex
{
	int locked;
	LIST (Process) blocked_list;
};

struct RecMutex
{
	int locked;
	struct Process *owner;
	int recursion_cnt;
	LIST (Process) blocked_list;
};

struct Cond
{
	LIST (Process) blocked_list;
};


struct RWLock
{	
	int32 i;
	int32 readers_waiting;
	
	struct Mutex mutex;
	struct Cond rd_cond;
	struct Cond wr_cond;
};


void MutexInit (struct Mutex *mutex);
int  MutexTryLock (struct Mutex *mutex);
void MutexLock (struct Mutex *mutex);
void MutexUnlock (struct Mutex *mutex);

void RecMutexInit (struct RecMutex *mutex);
int  RecMutexTryLock (struct RecMutex *mutex);
void RecMutexLock (struct RecMutex *mutex);
void RecMutexUnlock (struct RecMutex *mutex);

void CondInit (struct Cond *cond);
void CondWait (struct Cond *cond, struct Mutex *mutex);
void CondSignal (struct Cond *cond);
void CondBroadcast (struct Cond *cond);
int CondTimedWait (volatile struct Cond *cond, struct Mutex *mutex, struct TimeVal *tv);

void RWInit (struct RWLock *rwlock);
void RWReadLock (struct RWLock *rwlock);
void RWWriteLock (struct RWLock *rwlock);
void RWUnlock (struct RWLock *rwlock);


#endif

