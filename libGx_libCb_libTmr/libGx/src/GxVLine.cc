#include <libGx/GxVLine.hh>

#include "GxDefines.hh"

GxVLine::GxVLine(GxRealOwner *pOwner) :
  GxWin(pOwner)
{
  width = GX_SPACE_INC;
}

GxVLine::~GxVLine(void)
{}

void GxVLine::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    {
      XSetForeground(dInfo.display, vData.borderGC, dInfo.darkBorderPix);
      int cX = width/2 - 1;
      XDrawLine(dInfo.display, xWin, vData.borderGC, cX,0, cX,height);

      cX++;
      XSetForeground(dInfo.display, vData.borderGC, dInfo.lightBorderPix);
      XDrawLine(dInfo.display, xWin, vData.borderGC, cX,0, cX,height);
      XSetForeground(dInfo.display, vData.borderGC, dInfo.lightBorderPix);
    };
}

void GxVLine::GetWindowData(XSetWindowAttributes &winAttributes,
			    ULINT &valueMask)
{
  winAttributes.event_mask = ExposureMask;
  valueMask |= CWEventMask;
}

