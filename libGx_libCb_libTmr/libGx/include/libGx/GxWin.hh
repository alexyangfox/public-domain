#ifndef GXWIN_INCLUDED
#define GXWIN_INCLUDED

#include <libGx/GxCoreWin.hh>
#include <libGx/GxRealOwner.hh>

class GxWin : public GxCoreWin
{
public:
  GxWin(GxRealOwner *pOwner);
  virtual ~GxWin(void);
};

#endif //GXWIN_INCLUDED
