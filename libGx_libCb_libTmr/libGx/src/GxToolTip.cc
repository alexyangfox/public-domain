#include <string.h>

#include <libGx/GxToolTip.hh>

GxToolTip::GxToolTip(GxRealOwner *pOwner) :
  GxRootTransient(pOwner)
{
  textBuffer[0] = '\0';
}

GxToolTip::GxToolTip(GxRealOwner *pOwner, const char *pText) :
  GxRootTransient(pOwner)
{
  unsigned junkLen = 0;
  GxSetLabel(textBuffer, GX_DEFAULT_LABEL_LEN, pText, junkLen);
}

GxToolTip::~GxToolTip(void)
{}

void GxToolTip::Place(void)
{
  //one pixel top and bottom for black border two pixels top and bottom
  //for text gap
  height = dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent + 6;
  width = 6 + XTextWidth(dInfo.pDefaultFont, textBuffer, strlen(textBuffer));
}

void GxToolTip::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      {
	int textWidth = XTextWidth(dInfo.pDefaultFont, textBuffer,
				   strlen(textBuffer) );
	int textX = ((int)width - textWidth)/2;
	int textY = (height - (dInfo.pDefaultFont->ascent +
			       dInfo.pDefaultFont->descent))/2 +
	  dInfo.pDefaultFont->ascent; 

	XSetFont(dInfo.display, vData.borderGC, dInfo.pDefaultFont->fid);
	XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
	XDrawString(dInfo.display, xWin, vData.borderGC, textX, textY,
		    textBuffer, strlen(textBuffer) );
	XDrawRectangle(dInfo.display, xWin, vData.borderGC, 0,0,
		       width-1,height-1);
      };
}

void GxToolTip::GetWindowData(XSetWindowAttributes &winAttributes,
			      ULINT &valueMask)
{
  GxRootTransient::GetWindowData(winAttributes, valueMask);
  winAttributes.border_pixel = dInfo.blackPix;
  winAttributes.background_pixel = dInfo.toolTipBGPix;
  winAttributes.event_mask = ExposureMask;
  winAttributes.save_under = true;

  valueMask |= CWBackPixel | CWBorderPixel | CWEventMask | CWSaveUnder;
}
