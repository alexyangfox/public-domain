#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H

#include <kernel/types.h>
#include <kernel/lists.h>




#define DAYS_IN_4CENTURIES				146097
#define DAYS_IN_CENTURY					36524
#define DAYS_IN_FIRST_CENTURY 			36525
#define DAYS_IN_4YEARS					1461
#define DAYS_TO_1970					719529




/*
 *
 */

#define JIFFIES_PER_SECOND				100
#define MICROSECONDS_PER_JIFFY			10000
#define PIT_100HZ_INTERVAL 				11932
#define PIT_FREQ                        1193182

/*
 *
 */

#define TIMER_TYPE_RELATIVE				0
#define TIMER_TYPE_ABSOLUTE				1
#define TIMER_TYPE_PERIODIC				2




/*
 *
 */


struct TimeVal
{
	uint32 seconds;
	uint32 microseconds;
};



struct Tm
{
	uint32 sec;
	uint32 min;
	uint32 hour;
	uint32 mday;
	uint32 mon;
	uint32 year;
	uint32 wday;
	uint32 yday;
};







struct Timer
{
	LIST_ENTRY (Timer) timer_entry;
	bool expired;
	uint32 type;
	uint32 expiration_seconds;
	uint32 expiration_jiffies;
	void (*callout)(struct Timer *timer, void *arg);
	void *arg;
	struct Process *process;
};



struct MST
{
	uint32 prev_count;
	uint32 accum_count;
};


struct Alarm
{
	struct Timer timer;
	int32 signal;
	int pid;
	bool timedout;
};



extern uint32 loops_per_jiffy;
extern volatile uint32 hardclock_seconds, hardclock_jiffies;
extern volatile uint32 softclock_seconds, softclock_jiffies;
extern LIST_DECLARE (TimingWheelList, Timer) timing_wheel[JIFFIES_PER_SECOND];




/*
 *
 */
 
uint32 GetBootTime (void);


int32 LocalTime (struct Tm *tm, struct TimeVal *tv);
uint32 MakeTime (struct Tm *tm);
int IsLeap (uint32 year);
int32 YearToDays (int32 year);
int32 DaysToYear (int32 days, int32 *days_remaining);

void HardClockProcessing(void);
void SoftClockProcessing (void);
int EnqueueTimer (struct Timer *timer);
int DequeueTimer (struct Timer *timer);
int SetTimeOfDay (struct TimeVal *tv);

int KGetTimeOfDay (struct TimeVal *tv_out);
int UGetTimeOfDay (struct TimeVal *tv_out);


int32 DiffTime (struct TimeVal *start, struct TimeVal *end, struct TimeVal *result);
int32 CompareTime (struct TimeVal *tva, struct TimeVal *tvb);
int32 AddTime (struct TimeVal *dst, struct TimeVal *tva, struct TimeVal *tvb);

void UDelay (uint32 usec);
void UStart (struct MST *ms);
uint32 UElapsed (struct MST *ms);

void InitTimer (struct Timer *timer);
void SetTimer (struct Timer *timer, int32 type, struct TimeVal *tv, void (*callout)(struct Timer *timer, void *arg), void *arg);
void CancelTimer (struct Timer *timer);



int KSleep (struct TimeVal *tv);
int KSleep2 (uint32 seconds, uint32 microseconds);
void KSleepCallout (struct Timer *timer, void *arg);
int KAlarmSet (struct Alarm *alarm, uint32 seconds, uint32 microseconds, int signal);
int KAlarmCheck (struct Alarm *alarm);
int KAlarmCancel (struct Alarm *alarm);
void KAlarmCallout (struct Timer *timer, void *arg);


int USleep (uint32 seconds, uint32 microseconds);
unsigned int UAlarm (unsigned int seconds);
void UAlarmCallout (struct Timer *timer, void *arg);




#endif

