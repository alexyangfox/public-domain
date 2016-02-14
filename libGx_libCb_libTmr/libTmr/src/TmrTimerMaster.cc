#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>

#ifdef DEBUG_BUILD
#include <iostream>
#endif

#include <libTmr/TmrTimerMaster.hh>

#include "TmrUtil.hh"

using namespace std;

TmrTimerMaster::TmrTimerMaster(void)
{}

TmrTimerMaster::~TmrTimerMaster(void)
{
  while( !timerList.empty() )
    {
      TmrTimer *pFront = timerList.front();
      timerList.pop_front();
      pFront->DeActivate();
    };
}

void TmrTimerMaster::AddTimer(TmrTimer *pNewTimer)
{
  if(!pNewTimer) return;
  
  timeval currTime;
  gettimeofday(&currTime,NULL);
  pNewTimer->SetSetTime(currTime);

  if( timerList.empty() )
    {
      timerList.push_front(pNewTimer);
      SetNewDelay(pNewTimer->TimeTill(currTime));
      return;
    };

  list<TmrTimer*>::iterator cPlace = timerList.begin();
  list<TmrTimer*>::iterator cEnd = timerList.end();
  while( cPlace != cEnd )
    {
      if(pNewTimer->TimeTill(currTime) < (*cPlace)->TimeTill(currTime) )
	{
	  cPlace = timerList.insert(cPlace, pNewTimer);
	  //hackish.
	  if( cPlace == timerList.begin() )
	    SetNewDelay(pNewTimer->TimeTill(currTime));
	  return;
	};
      cPlace++;
    };
	
  //if the timer occurs after every other timer in the list
  //the timer must go at the end of the list
  //we already took care of the eventuality that is was the only timer above.
  timerList.push_back(pNewTimer);
}

void TmrTimerMaster::RemoveTimer(TmrTimer *pExTimer)
{
  if(!pExTimer) return;

  list<TmrTimer*>::iterator cPlace = timerList.begin();
  list<TmrTimer*>::iterator cEnd = timerList.end();
  while( cPlace != cEnd )
    {
      if(*cPlace == pExTimer)
	{
	  cPlace = timerList.erase(cPlace);
	  return;
	};
      cPlace++;
    };
}

void TmrTimerMaster::CheckAndExecExpired(void)
{
#ifdef DEBUG_BUILD
  cout << "TmrTimerMaster::CheckAndExecExpired" << endl;
#endif

  if(timerList.empty())
    {
#ifdef DEBUG_BUILD
      cerr << "list is empty" << endl;
#endif
      return;
    };

  //get the time and call and remove all timers that should have occured
  //by now. if we have any timers left, set the first one on our list
  //current and set alarm() or whatever.
  timeval currTime;
  gettimeofday(&currTime,NULL);

  //fire all timers from the start of the list on.
  list<TmrTimer*>::iterator cPlace = timerList.begin();
  list<TmrTimer*>::iterator cEnd = timerList.end();
  while( cPlace != cEnd )
    {
      if( (*cPlace)->DoTimer(currTime) )
	cPlace = timerList.erase(cPlace);
      else
	break;
    };
}

bool TmrTimerMaster::GetNextTimerTime(timeval &rNextDelay)
{
  if( timerList.empty() ) return false;

  timeval currTime;
  gettimeofday(&currTime,NULL);

  TmrTimer* pFirst = timerList.front();
#ifdef DEBUIG_BUILD
  assert(pFirst);
#endif
  rNextDelay = pFirst->TimeTill(currTime);
  return true;
}

void TmrTimerMaster::SetNewDelay(const timeval &)
{}


// ************************************* start TmrSigAlarmMaster ***********************

TmrSigAlarmMaster::TmrSigAlarmMaster(void) :
  TmrTimerMaster()
{}

TmrSigAlarmMaster::~TmrSigAlarmMaster(void)
{
  if( pSAMaster == this )
    pSAMaster = 0;
}

void TmrSigAlarmMaster::InitializeHandler(void)
{
  pSAMaster = this;
  signal(SIGALRM, TmrSigAlarmMaster::AlarmSigHandler);

  timeval nextDelay;
  if( GetNextTimerTime(nextDelay) )
    SetNewDelay(nextDelay);
}

void TmrSigAlarmMaster::SetNewDelay(const timeval &rNewDelay)
{
#ifdef DEBUG_BUILD
  cout << "TmrSigAlarmrMaster::SetNewDelay" << endl;
#endif //DEBUG_BUILD
  
  if( pSAMaster != this ) return;

  itimerval newVal;
  newVal.it_interval.tv_sec = 0;
  newVal.it_interval.tv_usec = 0;

  newVal.it_value = rNewDelay;
  setitimer(ITIMER_REAL, &newVal, NULL);
}

void TmrSigAlarmMaster::SignalCB(int signum)
{
  CheckAndExecExpired();
  timeval nextDelay;
  if( GetNextTimerTime(nextDelay) )
    SetNewDelay(nextDelay);
}

//****************** start static stuff *******************
TmrSigAlarmMaster * TmrSigAlarmMaster::pSAMaster = 0;
void TmrSigAlarmMaster::AlarmSigHandler(int arg)
{
  if(!pSAMaster) return;
  
  pSAMaster->SignalCB(arg);

  signal(SIGALRM, TmrSigAlarmMaster::AlarmSigHandler); //reset handler
}
