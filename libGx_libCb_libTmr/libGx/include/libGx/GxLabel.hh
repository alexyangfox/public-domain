#ifndef GXLABEL_INCLUDED
#define GXLABEL_INCLUDED

#include <libGx/GxWin.hh>

class GxLabel : public GxWin
{
public:
  GxLabel(GxRealOwner *pOwner, const char *pLabel = NULL);
  virtual ~GxLabel(void);

  void SetLabel(const char *pNewLabel);

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  void HandleEvent(const XEvent &rEvent);

protected:
  void GetWindowData(XSetWindowAttributes &winAttributes, ULINT &valueMask);

  void DrawLabel(void);
  char labelText[GX_LONG_LABEL_LEN];
  unsigned strLength;
};

#endif //GXLABEL_INCLUDED
