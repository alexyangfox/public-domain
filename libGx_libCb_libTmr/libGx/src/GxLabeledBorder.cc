#include <libGx/GxLabeledBorder.hh>

#include "GxDefines.hh"

GxLabeledBorder::GxLabeledBorder(GxRealOwner *pOwner,
				 const char *pLabel) :
  GxOwnerWin(pOwner), labelLen(0)
{
  SetLabel(pLabel);
}

GxLabeledBorder::~GxLabeledBorder(void){}

void GxLabeledBorder::SetLabel(const char *pLabel)
{
  GxSetLabel(label, GX_DEFAULT_LABEL_LEN, pLabel, labelLen);

  if(labelLen)
    labelPixLen = XTextWidth(dInfo.pDefaultFont, label, labelLen);
  else
    labelPixLen = GX_BORDER_WD; //hack(ish)
}

UINT GxLabeledBorder::GetDesiredWidth(void) const
{
  if( !childList.empty() )
    return ( childList.front() )->GetDesiredWidth() + 6 +
      ( childList.front() )->LBorder() + ( childList.front() )->RBorder();
  else
    return width;
}

UINT GxLabeledBorder::GetDesiredHeight(void) const
{
  if( !childList.empty() )
    {
      return ( childList.front() )->GetDesiredHeight() +
	dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent + 5 +
	( childList.front() )->TBorder() + ( childList.front() )->BBorder();
    }else
      return height;
}

void GxLabeledBorder::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      {
 	XSetForeground(dInfo.display, vData.textGC, dInfo.labelTextPix);
	XDrawString(dInfo.display, xWin, vData.textGC, GX_SPACE_INC +2,
		    1+dInfo.pDefaultFont->ascent, label, labelLen);

 	XSetForeground(dInfo.display, vData.borderGC, dInfo.darkBorderPix);
	//the top lines y
	int tlY = (dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent)/2;
	
	XDrawLine(dInfo.display, xWin, vData.borderGC,
		  0,tlY, GX_SPACE_INC,tlY);
	XDrawLine(dInfo.display, xWin, vData.borderGC,
		  GX_SPACE_INC + labelPixLen + 4,tlY, width-1,tlY);

	XDrawLine(dInfo.display, xWin, vData.borderGC, width-2,tlY+1,
		  width-2,height-2);
	XDrawLine(dInfo.display, xWin, vData.borderGC, width-2,height-2,
		  1,height-2);

	XDrawLine(dInfo.display, xWin, vData.borderGC, 0,height-1, 0,tlY);

	XSetForeground(dInfo.display, vData.borderGC, dInfo.lightBorderPix);
	XDrawLine(dInfo.display, xWin, vData.borderGC,
		  1,tlY+1, GX_SPACE_INC,tlY+1);

	XDrawLine(dInfo.display, xWin, vData.borderGC,
		  GX_SPACE_INC + labelPixLen + 4,tlY+1, width-2,tlY+1);

	XDrawLine(dInfo.display, xWin, vData.borderGC, 1,height-2, 1,tlY+1);

	XDrawLine(dInfo.display, xWin, vData.borderGC, width-1,tlY,
		  width-1,height-1);
	XDrawLine(dInfo.display, xWin, vData.borderGC, width-1,height-1,
		  0,height-1);
      };
}

void GxLabeledBorder::PlaceChildren(void)
{
  int lX = 3;
  int rX = (int)width - 3;
  int tY = dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent + 2;
  int bY = (int)height - 3;

  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      //note each iteration through lX rX tY bY are posibly modified
      //by each child's GxGeomControl ?(if it has one)?
      (*cPlace)->Place(lX, rX, tY, bY);
      cPlace++;
    };
};

void GxLabeledBorder::GetWindowData(XSetWindowAttributes &winAttributes,
				    ULINT &valueMask)
{
  winAttributes.event_mask = ExposureMask;
  valueMask |= CWEventMask;
}
