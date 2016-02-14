#include <libGx/GxHLine.hh>

#include "GxDefines.hh"

GxHLine::GxHLine(GxRealOwner *pOwner) :
  GxWin(pOwner)
{
  height = 2*GX_SPACE_INC;
}

GxHLine::~GxHLine(void)
{}

void GxHLine::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      {
	XSetForeground(dInfo.display, vData.borderGC, dInfo.darkBorderPix);
	int cY = ((int)(height/2)) - 1;
	XDrawLine(dInfo.display, xWin, vData.borderGC, 0,cY, width,cY);
	
	cY++;
	XSetForeground(dInfo.display, vData.borderGC, dInfo.lightBorderPix);
	XDrawLine(dInfo.display, xWin, vData.borderGC, 0,cY, width,cY);
	XSetForeground(dInfo.display, vData.borderGC, dInfo.lightBorderPix);
      };
}

void GxHLine::GetWindowData(XSetWindowAttributes &winAttributes,
			    ULINT &valueMask)
{
  winAttributes.event_mask = ExposureMask;
  valueMask |= CWEventMask;
}
