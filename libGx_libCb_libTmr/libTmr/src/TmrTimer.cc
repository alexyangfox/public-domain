#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>

#ifdef DEBUG_BUILD
#include <iostream>
#endif

#include <libTmr/TmrTimer.hh>
#include <libTmr/TmrTimerMaster.hh>

#include "TmrUtil.hh"

using namespace std;

TmrTimer::TmrTimer(unsigned tDelay, const CbVoidBase &rCB) :
  cb(rCB), pMyMaster(0)
{
  delayTime.tv_sec = tDelay/100;
  delayTime.tv_usec = (tDelay%100)*10000;
}

TmrTimer::TmrTimer(const timeval &rVal, const CbVoidBase &rCB) :
  cb(rCB), delayTime(rVal)
{}

TmrTimer::~TmrTimer(void)
{
  DeActivate();
}

void TmrTimer::Activate(TmrTimerMaster &rMaster)
{
  if(pMyMaster) return; //we must already be active

  pMyMaster = &rMaster;
  pMyMaster->AddTimer(this);
}

void TmrTimer::DeActivate(void)
{
  if(!pMyMaster) return;

  pMyMaster->RemoveTimer(this);
  pMyMaster = 0;
}

void TmrTimer::SetDelay(unsigned delayInMS)
{
  delayTime.tv_sec = delayInMS/1000;
  delayTime.tv_usec = (delayInMS%1000)*100000;
}

timeval TmrTimer::GetDelay(void) const
{
  return delayTime;
}

void TmrTimer::SetSetTime(const timeval &rTVal)
{
  timeSet = rTVal;
}

timeval TmrTimer::TimeTill(const timeval &rCurrentTime)
{
  long sec = (long)delayTime.tv_sec + (long)timeSet.tv_sec - (long)rCurrentTime.tv_sec;
  long usec = (long)delayTime.tv_usec + (long)timeSet.tv_usec - (long)rCurrentTime.tv_usec;

  while(usec < 0)
    {
      sec--;
      usec += 1000000;
    };

  timeval retVal;
  retVal.tv_sec = sec;
  retVal.tv_usec = usec;

  if(sec < 0)
    {
      retVal.tv_sec = 0;
      retVal.tv_usec = 0;
    };
  
  return retVal;
}

bool TmrTimer::DoTimer(const timeval &rCurrentTime)
{
  timeval activateTime;
  activateTime.tv_sec = delayTime.tv_sec + timeSet.tv_sec;
  activateTime.tv_usec = delayTime.tv_usec + timeSet.tv_usec;
  unsigned secInUS = activateTime.tv_usec/1000000;
  activateTime.tv_sec += secInUS;
  activateTime.tv_usec %= 1000000;

  if(activateTime < rCurrentTime)
    {
      //DeActivate(); //don't do this.
      //we deactivate ourselves. the timer Master will remove the pointer to this when we return true.
      pMyMaster = 0; 
      cb();
      return true;
    }else
      {
	return false;
#ifdef DEBUG_BUILD
	cerr << "TmrTimer::DoTimer : not me!!!" << endl;
#endif
      };
}
