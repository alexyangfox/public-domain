#ifndef GXMENUITEMOWNER_INCLUDED
#define GXMENUITEMOWNER_INCLUDED

#include <libGx/GxRealOwner.hh>

class GxMenuItemOwner
{
public:
  virtual ~GxMenuItemOwner(void);

  virtual bool GetButtonHeldMode(void) const = 0;
  virtual void SetButtonHeldMode(bool newMode) = 0;
  virtual void EndMenuEventGrab(void) = 0;
  virtual GxRealOwner & GetRealOwnerObject(void) = 0;

protected:
  GxMenuItemOwner(void);
};

#endif //GXMENUITEMOWNER_INCLUDED
