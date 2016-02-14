#ifndef GXOWNERWIN_INCLUDED
#define GXOWNERWIN_INCLUDED

#include <libGx/GxCoreOwnerWin.hh>
#include <libGx/GxRealOwner.hh>

class GxOwnerWin : public GxCoreOwnerWin
{
public:
  GxOwnerWin(GxRealOwner *pOwner);
  virtual ~GxOwnerWin(void);

};

#endif //GXOWNERWIN_INCLUDED
