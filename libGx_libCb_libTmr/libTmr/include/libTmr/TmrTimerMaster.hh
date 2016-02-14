#ifndef TMRTIMERMASTER_INCLUDED
#define TMRTIMERMASTER_INCLUDED

#include <libTmr/TmrTimer.hh>

//this is intended to be wired directly into a select loop using the timeout argument
//obtain the timeout argument via GetNextTimerTime
class TmrTimerMaster
{
public:
  TmrTimerMaster(void);
  virtual ~TmrTimerMaster(void);

  virtual void AddTimer(TmrTimer *pNewTimer);
  virtual void RemoveTimer(TmrTimer *pExTimer); //called by a Timer's destructor

  //calling this runs (in the order of their alarm time) any timers in our list
  //that are expired. After the timer has been run, it is removed
  virtual void CheckAndExecExpired(void);

  //return false if no active timers exist
  virtual bool GetNextTimerTime(timeval &rNextDelay);

protected:
  //Called when a new timer is added to the front of the timer list.
  //Does nothing by default
  virtual void SetNewDelay(const timeval &rNewDelay);

  //list of timer pointers in order of incresing time
  std::list<TmrTimer*> timerList;
};

//a timer master driven by sigalarm.
//works by probably borked. -> race conditions ?locking?

class TmrSigAlarmMaster : public TmrTimerMaster
{
public:
  TmrSigAlarmMaster(void);
  virtual ~TmrSigAlarmMaster(void);

  //calls signal.
  void InitializeHandler(void);

protected:
  //set the signals up according to this timer
  virtual void SetNewDelay(const timeval &rNewDelay);

  virtual void SignalCB(int arg);

  static TmrSigAlarmMaster *pSAMaster;
  static void AlarmSigHandler(int arg);
};

#endif //TMRTIMERMASTER_INCLUDED
