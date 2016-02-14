#include <libGx/GxStatusBar.hh>
#include "GxDefines.hh"

GxStatusBar::GxStatusBar(GxRealOwner *pOwner) :
  GxWin(pOwner)
{
  message[0] = '\0';
}

GxStatusBar::~GxStatusBar(void)
{}

UINT GxStatusBar::GetDesiredHeight(void) const
{
  return 2*GX_BORDER_WD + dInfo.pDefaultFont->ascent +
    dInfo.pDefaultFont->descent + 2;
}

void GxStatusBar::SetMessage(const char *pStr)
{
  unsigned junkLen = 0;
  GxSetLabel(message, GX_STATUS_BAR_MESSAGE_LEN, pStr, junkLen);

  if(Created())
    {
      XClearArea(dInfo.display, xWin, GX_BORDER_WD, GX_BORDER_WD,
		 width - 2*GX_BORDER_WD, height - 2*GX_BORDER_WD, false);
      Draw();
    };
}

void GxStatusBar::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      {
	Draw();
	return;
      };
};


void GxStatusBar::Draw(void)
{
  if(!Created()) return;

  Draw3dBorder(0,0, width, height, false);
  XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
  XSetFont(dInfo.display, vData.borderGC, dInfo.pDefaultFont->fid);
  int yBase = height/2 +
    (dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent)/2;
  XDrawString(dInfo.display, xWin, vData.borderGC, GX_BORDER_WD+1,
	      yBase, message, strlen(message));
}

void GxStatusBar::GetWindowData(XSetWindowAttributes &winAttrib,
				ULINT &valueMask)
{
  winAttrib.event_mask = ExposureMask;
  valueMask |= CWEventMask;
}

