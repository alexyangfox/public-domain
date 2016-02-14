#ifndef GXSTATUSBAR_INCLUDED
#define GXSTATUSBAR_INCLUDED

#include <libGx/GxWin.hh>
#include <string.h>

//a bar with a background color drawn with a recesed border for displaying application state.
const unsigned GX_STATUS_BAR_MESSAGE_LEN = 512;
class GxStatusBar : public GxWin
{
public:
  GxStatusBar(GxRealOwner *pOwner);
  virtual ~GxStatusBar(void);

  virtual UINT GetDesiredHeight(void) const;

  virtual void SetMessage(const char *pStr);
  virtual void HandleEvent(const XEvent &rEvent);

protected:
  void Draw(void);
  virtual void GetWindowData(XSetWindowAttributes &winAttrib,
			     ULINT &valueMask);
  char message[GX_STATUS_BAR_MESSAGE_LEN];
};

#endif //GXSTATUSBAR_INCLUDED
