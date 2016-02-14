#include <string.h>
#include <sstream>

#include <libGx/GxPercentBar.hh>

#include "GxDefines.hh"

using namespace std;

GxPercentBar::GxPercentBar(GxRealOwner *pOwner) :
  GxWin(pOwner), cFraction(1,2)
{
  width = GX_PERCENT_BAR_DEFAULT_WIDTH;
  height = GX_PERCENT_BAR_DEFAULT_HEIGHT;
}

GxPercentBar::~GxPercentBar(void)
{}

void GxPercentBar::SetPercent(const GxFraction &rFraction)
{
  cFraction = rFraction;
  DrawInterior();
}

UINT GxPercentBar::GetDesiredWidth(void) const
{
  return GX_PERCENT_BAR_DEFAULT_WIDTH;
}

UINT GxPercentBar::GetDesiredHeight(void) const
{
  return GX_PERCENT_BAR_DEFAULT_HEIGHT;
}

void GxPercentBar::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      DrawInterior();
}

void GxPercentBar::GetWindowData(XSetWindowAttributes &winAttrib,
				 ULINT &valueMask)
{
  winAttrib.event_mask = ExposureMask;
  valueMask |= CWEventMask;
}

void GxPercentBar::DrawInterior(void)
{
  if(!Created()) return;

  Draw3dBorder(0,0, width, height, false);
  int availableWidth = width - 2*GX_BORDER_WD;
  if(availableWidth <= 1) return;
  int availableHeight = height - 2*GX_BORDER_WD;
  if(availableHeight <= 1) return;

  int filledWidth = cFraction.Convert(availableWidth);
  if(filledWidth < 0) return;

  if(filledWidth != 0)
    {
      XSetForeground(dInfo.display, vData.borderGC, dInfo.percentBarPix);
      XFillRectangle(dInfo.display, xWin, vData.borderGC, GX_BORDER_WD,
		     GX_BORDER_WD, filledWidth, availableHeight);
    };

  int clearWidth = availableWidth - filledWidth;
  if(clearWidth < 0) return;
  if(clearWidth != 0)
    {
      XSetForeground(dInfo.display, vData.borderGC, dInfo.whitePix);
      XFillRectangle(dInfo.display, xWin, vData.borderGC,
		     GX_BORDER_WD+filledWidth+1, GX_BORDER_WD,
		     clearWidth, availableHeight);
  };
 
  //draw the percent text
  int percentNum = cFraction.Convert(100);
  ostringstream percentStr;
  percentStr << percentNum << '%';

  std::string lPercentStr( percentStr.str() ); //gets rid of a valgrind error instead of calling percentStr.str().c_str()
  const char *pNumStr = lPercentStr.c_str();
  int numStrLen = strlen(pNumStr);
  int textWidth = XTextWidth(dInfo.pDefaultFont, pNumStr, numStrLen);

  int textX = (width - textWidth)/2;
  int textY = (height -
	       (dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent))/2 +
    dInfo.pDefaultFont->ascent;

  XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
  XSetFont(dInfo.display, vData.borderGC, dInfo.pDefaultFont->fid);
  XDrawString(dInfo.display, xWin, vData.borderGC, textX, textY,
	      pNumStr, numStrLen);
}
