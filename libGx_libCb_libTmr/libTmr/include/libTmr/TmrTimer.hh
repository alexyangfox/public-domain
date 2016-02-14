#ifndef GXTIMER_INCLUDED
#define GXTIMER_INCLUDED

#include <sys/time.h>
#include <list>

#include <libCb/CbCallback.hh>

class TmrTimerMaster;

//Should TmrTimerMaster be my friend? it would speed things up where speed is critical.
class TmrTimer
{
public:
  //timeDelay is in 100ths of a second
  TmrTimer(unsigned timeDelay, const CbVoidBase &rCB);
  TmrTimer(const timeval &rVal, const CbVoidBase &rCB); //for fine settings
  virtual ~TmrTimer(void);

  //every time a timer fires it is DeActivated internally.
  void Activate(TmrTimerMaster &rMaster);
  //deactivating a timer clears any refrence to the timer master
  //ok to DeActivate a non-active timer
  void DeActivate(void);

  void SetDelay(unsigned delayInMS);
  timeval GetDelay(void) const;
  void SetSetTime(const timeval &rTVal);

  //given current time, how long till I need to be trigered
  timeval TimeTill(const timeval &rCurrentTime);

  //called by the TmrTimerMaster.
  //returns true if the timer decided it had expired and fired.
  bool DoTimer(const timeval &rCurrentTime);

  CbVoidFO cb;

protected:
  timeval timeSet;
  timeval delayTime;

  volatile bool doCallback;
  TmrTimerMaster *pMyMaster; //only set when we are active;
};

#endif //GXTIMER_INCLUDED
