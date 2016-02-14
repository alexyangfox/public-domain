#include <sys/select.h>

#include <iostream>

#include <libCb/CbCallback.hh>

#include <libTmr/TmrTimer.hh>
#include <libTmr/TmrTimerMaster.hh>

using namespace std;

void TimerCB(int time)
{
  cout << "TimerCB time: " << time << endl;
}

int main(void)
{
  TmrTimer t1(100, CbVoidPlainObj<int>(TimerCB, 1) );
  TmrTimer t2(300, CbVoidPlainObj<int>(TimerCB, 3) );
  TmrTimer t3(400, CbVoidPlainObj<int>(TimerCB, 4) );
  TmrTimer t4(500, CbVoidPlainObj<int>(TimerCB, 5) );
  TmrTimer t5(600, CbVoidPlainObj<int>(TimerCB, 6) );

  cout << "starting select tests" << endl;

  TmrTimerMaster tm;

  t1.Activate(tm);
  t2.Activate(tm);
  t3.Activate(tm);
  t4.Activate(tm);
  t5.Activate(tm);

  timeval nextDelay;
  while( tm.GetNextTimerTime(nextDelay) )
    {
      //cout << "got nextDelay.tv_sec " << nextDelay.tv_sec << " nextDelay.tv_usec: " << nextDelay.tv_usec << endl;
      select(0, 0,0,0, &nextDelay);
      cout << "checking for timeouts" << endl;
      tm.CheckAndExecExpired();
    };

  cout << "starting sigalarm tests" << endl;

  TmrSigAlarmMaster sam;
  t1.Activate(sam);
  t2.Activate(sam);
  t3.Activate(sam);
  t4.Activate(sam);
  t5.Activate(sam);

  sam.InitializeHandler();

  timeval junkDelay;
  while( sam.GetNextTimerTime(junkDelay) )
    {
      cout << "sleepin'" << endl;
      sleep(2);
    };

  cout << "done sleepin'"<< endl;

  return 0;
}
