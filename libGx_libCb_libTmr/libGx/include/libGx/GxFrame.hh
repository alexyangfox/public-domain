#ifndef GXFRAME_INCLUDED
#define GXFRAME_INCLUDED

//this class is only to be used as a visual element, it is basicly
//a GxOwnerWin which draws a 3d border around itself, if more advanced
//functionality is needed, it would be better to re-inherit from GxCoreOwnerWin

#include <libGx/GxOwnerWin.hh>

class GxFrame : public GxOwnerWin
{
public:
  GxFrame(GxRealOwner *pOwner);
  virtual ~GxFrame(void);

  //by default we return the desired size of the first child in our list plus 2*GX_BORDER_WD
  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  virtual void PlaceChildren(void);
  virtual void HandleEvent(const XEvent &rEvent);

protected:
  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);
};

#endif //GXFRAME_INCLUDED
