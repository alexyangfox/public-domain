#ifndef GXPERCENTBAR_INCLUDED
#define GXPERCENTBAR_INCLUDED

#include <libGx/GxInc.hh>
#include <libGx/GxWin.hh>
#include <libGx/GxFraction.hh>

class GxPercentBar : public GxWin
{
public:
  GxPercentBar(GxRealOwner *pOwner);
  virtual ~GxPercentBar(void);

  virtual void SetPercent(const GxFraction &rFraction);

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  virtual void HandleEvent(const XEvent &rEvent);
protected:
  virtual void GetWindowData(XSetWindowAttributes &winAttrib,
			     ULINT &valueMask);
  virtual void DrawInterior(void);

  GxFraction cFraction;
};

#endif //GXPERCENTBAR_INCLUDED
