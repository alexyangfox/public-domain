#ifndef GXMAINWIN_INCLUDED
#define GXMAINWIN_INCLUDED

#include <libGx/GxInc.hh>
#include <libGx/GxTopLevelWin.hh>
#include <libGx/GxDisplay.hh>

class GxMainWin : public GxTopLevelWin
{
public:
  GxMainWin(GxDisplay *pOwner);
  virtual ~GxMainWin(void);

protected:

};

#endif //GXMAINWIN_INCLUDED
