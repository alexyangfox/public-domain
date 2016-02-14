#ifndef GXREALOWNER_INCLUDED
#define GXREALOWNER_INCLUDED

#include <functional>

#include <libGx/GxInc.hh>
#include <libGx/GxWinArea.hh>
#include <libGx/GxOwner.hh>

class GxRealOwner : public GxOwner
{
public:
  virtual ~GxRealOwner(void);

  //Places all of my children. the default action should be  to place the
  //children according to their GxGeomControl. This is implemented in
  // GxGhost.cc and GxCoreOwnerWin.cc.
  virtual void PlaceChildren(void) = 0;

  virtual void CreateChildren(void);
  virtual void DisplayChildren(void);
  virtual void HideChildren(void); //not used for much.
protected:
  GxRealOwner(void);
};

#endif //GXREALOWNER_INCLUDED
