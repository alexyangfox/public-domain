#include <libGx/GxMainInterface.hh>
#include <libGx/GxScrollBar.hh>

#include "GxDefines.hh"

GxScrollButton::GxScrollButton(GxRealOwner *pOwner, GX_DIRECTION bDir) :
  GxWin(pOwner), sbTimer(10, CbVoidMember<GxScrollButton>(this, &GxScrollButton::TimerCB) )
{
  dir = bDir;
  pressed = false;
}

GxScrollButton::~GxScrollButton(void)
{}

void GxScrollButton::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    {
      if(rEvent.xexpose.count == 0)
	DrawButton();
      return;
    };

  if(rEvent.type == ButtonPress)
    {
      if(rEvent.xbutton.button == 1)
	{
	  pressed = true;
	  DrawButton();
	  dInfo.rMainInterface.ActivateTimer(sbTimer);
	  cb();
	};
      return;
    };

  if(rEvent.type == ButtonRelease)
    if( pressed && rEvent.xbutton.button == 1)
      {
	sbTimer.DeActivate();
	pressed = false;
	DrawButton();
      };
}

void GxScrollButton::TimerCB(void)
{
  dInfo.rMainInterface.ActivateTimer(sbTimer);
  cb();
}

void GxScrollButton::GetWindowData(XSetWindowAttributes &winAttributes,
				   ULINT &valueMask)
{
  winAttributes.background_pixel = dInfo.recessedPix;
  winAttributes.event_mask |= ExposureMask | EnterWindowMask |
    LeaveWindowMask | ButtonPressMask | ButtonReleaseMask;
  valueMask |= CWEventMask;
}

void GxScrollButton::DrawButton(void)
{
  //should always be square
  Draw3dArrow(0,0, width, dir, !pressed);
}

// ******************* start GxScrollBar *************************

GxScrollBar::GxScrollBar(GxRealOwner *pOwner) :
  GxOwnerWin(pOwner),
  totalFr(GX_MAX_FRACTION), scrollFr(GX_MIN_FRACTION)
{}

GxScrollBar::~GxScrollBar(void)
{}

void GxScrollBar::GetTotalFraction(GxFraction &rFr)
{
  rFr = totalFr;
}

void GxScrollBar::GetScrollFraction(GxFraction &rFr)
{
  rFr = scrollFr;
}

void GxScrollBar::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      Draw3dBorder(0,0, width, height, false);
}

void GxScrollBar::GetWindowData(XSetWindowAttributes &winAttributes,
				ULINT &valueMask)
{
  winAttributes.background_pixel = dInfo.recessedPix;
  winAttributes.event_mask |= ExposureMask;
  valueMask |= CWEventMask | CWBackPixel;
}


GxVScrollBar::GxVScrollBar(GxRealOwner *pOwner) :
  GxScrollBar(pOwner),
  downButton(this, GX_DOWN),
  upButton(this, GX_UP),
  vGrip(this, CbOneMember<int, GxVScrollBar>(this, &GxVScrollBar::SlideGripCallback) )
{
  width = GX_SLIDER_WIDTH; //+2*GX_BORDER_WD;

  downButton.Resize(GX_SLIDER_WIDTH-2*GX_BORDER_WD,GX_SLIDER_WIDTH-2*GX_BORDER_WD);
  //pButton->X(2); //we should not even bother here
  downButton.cb.Assign( CbVoidMember<GxVScrollBar>(this, &GxVScrollBar::IntScrollDown) );

  upButton.Resize(GX_SLIDER_WIDTH-2*GX_BORDER_WD,GX_SLIDER_WIDTH-2*GX_BORDER_WD);
  upButton.Move(GX_BORDER_WD,GX_BORDER_WD);
  upButton.cb.Assign( CbVoidMember<GxVScrollBar>(this, &GxVScrollBar::IntScrollUp) );

  vGrip.Width(GX_SLIDER_WIDTH-2*GX_BORDER_WD);
  vGrip.Move(GX_BORDER_WD,GX_SLIDER_WIDTH);
}

GxVScrollBar::~GxVScrollBar(void)
{}

UINT GxVScrollBar::GetDesiredWidth(void) const
{
  return GX_SLIDER_WIDTH;
}

UINT GxVScrollBar::GetDesiredHeight(void) const
{
  return height;
}

void GxVScrollBar::SetTotalFraction(GxFraction tfr)
{
  totalFr = tfr;
  PlaceChildren();
}

void GxVScrollBar::SetScrollFraction(GxFraction sfr)
{
  scrollFr = sfr;
  PlaceChildren();
}

void GxVScrollBar::SetBothFractions(GxFraction tFr, GxFraction sFr)
{
  totalFr = tFr;
  scrollFr = sFr;
  PlaceChildren();
}

void GxVScrollBar::PlaceChildren(void)
{
  downButton.Move(GX_BORDER_WD,height - GX_SLIDER_WIDTH +GX_BORDER_WD);
  //figure out free space
  UINT freeH = height - (2*GX_SLIDER_WIDTH);
  UINT initalH = totalFr.Convert(freeH);
  UINT gripHeight = (initalH < 15) ? 15 : initalH;
  vGrip.Height(gripHeight);
  UINT moveableSpace = freeH - gripHeight;
  int SliderY = scrollFr.Convert(moveableSpace);
  vGrip.Y(SliderY + GX_SLIDER_WIDTH);
}

void GxVScrollBar::SlideGripCallback(int dist)
{
  //we want to avoid making a callback at all costs; so we check if our new
  //fraction is different from the old one & only if it is do we actually
  //make the callback. This can happen if the scrollbar is already at the
  //top or bottom and the user continues to drag in that direction.
  //we also don't bother making a callback if our totalFr GxFraction is
  //GX_MAX_FRACTION
  if(GX_MAX_FRACTION == totalFr)
    return;

  GxFraction newScrollFr;
  int currentY = vGrip.Y();
  //I assume that the height is correct wrt totalFR
  UINT cHeight = vGrip.Height();
  if(dist < 0) //we are moving up
    {
      if((currentY + dist) < (int)GX_SLIDER_WIDTH)
	{
	  vGrip.Y(GX_SLIDER_WIDTH);
	  newScrollFr = GX_MIN_FRACTION;
	}else
	  {
	    vGrip.Y(currentY + dist);
	    //really just the max y value
	    UINT freeH = height - (2*GX_SLIDER_WIDTH) - cHeight;
	    UINT adjY = (currentY + dist) - GX_SLIDER_WIDTH;
	    newScrollFr = GxFraction(adjY, freeH);
	  };
    }else  //we are moving down
      {
	if((currentY + dist) > (int)(height - GX_SLIDER_WIDTH - cHeight))
	  {
	    vGrip.Y(height - GX_SLIDER_WIDTH - cHeight);
	    newScrollFr = GX_MAX_FRACTION;
	  }else
	    {
	      vGrip.Y(currentY + dist);
	      UINT freeH = height - (2*GX_SLIDER_WIDTH) - cHeight;
	      UINT adjY = (currentY + dist) - GX_SLIDER_WIDTH;
	      newScrollFr = GxFraction(adjY, freeH);
	    };
      };

  if(newScrollFr != scrollFr)
    {
      scrollFr = newScrollFr;
      scrollCB(scrollFr);
    };
}

void GxVScrollBar::IntScrollUp(void)
{
  scrollUpCB();
}

void GxVScrollBar::IntScrollDown(void)
{
  scrollDownCB();
}


// ********************** start GxHScrollBar ***********************

GxHScrollBar::GxHScrollBar(GxRealOwner *pOwner) :
  GxScrollBar(pOwner),
  rightButton(this, GX_RIGHT),
  leftButton(this, GX_LEFT),
  hGrip(this, CbOneMember<int, GxHScrollBar>(this, &GxHScrollBar::SlideGripCallback) )
{
  height = GX_SLIDER_WIDTH;

  rightButton.Resize(GX_SLIDER_WIDTH-2*GX_BORDER_WD,GX_SLIDER_WIDTH-2*GX_BORDER_WD);
  //pButton->Y(2); we should not even bother here
  rightButton.cb.Assign( CbVoidMember<GxHScrollBar>(this, &GxHScrollBar::IntScrollRight) );

  leftButton.Resize(GX_SLIDER_WIDTH-2*GX_BORDER_WD,GX_SLIDER_WIDTH-2*GX_BORDER_WD);
  leftButton.Move(GX_BORDER_WD,GX_BORDER_WD);
  leftButton.cb.Assign( CbVoidMember<GxHScrollBar>(this, &GxHScrollBar::IntScrollLeft) );

  hGrip.Height(GX_SLIDER_WIDTH-2*GX_BORDER_WD);
  hGrip.Move(GX_SLIDER_WIDTH,GX_BORDER_WD);
}

GxHScrollBar::~GxHScrollBar(void)
{}

UINT GxHScrollBar::GetDesiredWidth(void) const
{
  return width;
}

UINT GxHScrollBar::GetDesiredHeight(void) const
{
  return GX_SLIDER_WIDTH;
}

void GxHScrollBar::SetTotalFraction(GxFraction tfr)
{
  totalFr = tfr;
  PlaceChildren();
}

void GxHScrollBar::SetScrollFraction(GxFraction sfr)
{
  scrollFr = sfr;
  PlaceChildren();
}

void GxHScrollBar::SetBothFractions(GxFraction tFr, GxFraction sFr)
{
  totalFr = tFr;
  scrollFr = sFr;
  PlaceChildren();
}

void GxHScrollBar::PlaceChildren(void)
{
  rightButton.Move(width - GX_SLIDER_WIDTH+GX_BORDER_WD,GX_BORDER_WD);

  UINT freeW = width - (2*GX_SLIDER_WIDTH);
  UINT initalW = totalFr.Convert(freeW);
  UINT gripWidth = ((initalW < 15) ? 15 : initalW);
  hGrip.Width(gripWidth);

  UINT moveableSpace = freeW - gripWidth;
  int SliderX = scrollFr.Convert(moveableSpace);
  hGrip.X(SliderX + GX_SLIDER_WIDTH);
}

void GxHScrollBar::SlideGripCallback(int dist)
{
  //we want to avoid making a callback at all costs; so we check if our new
  //fraction is different from the old one & only if it is do we actually
  //make the callback. This can happen if the scrollbar is already at the
  //top or bottom and the user continues to drag in that direction.
  //we also don't bother making a callback if our totalFr GxFraction is
  //GX_MAX_FRACTION
  if(GX_MAX_FRACTION == totalFr)
    return;

  GxFraction newScrollFr;
  int currentX = hGrip.X();
  //I assume that the width is correct wrt totalFR
  UINT cWidth = hGrip.Width();
  if(dist < 0) //we are moving left
    {
      if((currentX + dist) < (int)GX_SLIDER_WIDTH)
	{
	  hGrip.X(GX_SLIDER_WIDTH);
	  newScrollFr = GX_MIN_FRACTION;
	}else
	  {
	    hGrip.X(currentX + dist);
	    UINT freeW = width - (2*GX_SLIDER_WIDTH) - cWidth;
	    UINT adjX = (currentX + dist) - GX_SLIDER_WIDTH;
	    newScrollFr = GxFraction(adjX, freeW);
	  };
    }else  //we are moving right
      {
	if((currentX + dist) > (int)(width - GX_SLIDER_WIDTH - cWidth))
	  {
	    hGrip.X(width - GX_SLIDER_WIDTH - cWidth);
	    newScrollFr = GX_MAX_FRACTION;
	  }else
	    {
	      hGrip.X(currentX + dist);
	      UINT freeW = width - (2*GX_SLIDER_WIDTH) - cWidth;
	      UINT adjX = (currentX + dist) - GX_SLIDER_WIDTH;
	      newScrollFr = GxFraction(adjX, freeW);
	    };
      };

  if(newScrollFr != scrollFr)
    {
      scrollFr = newScrollFr;
      scrollCB(scrollFr);
    };
}

void GxHScrollBar::IntScrollLeft(void)
{
  scrollLeftCB();
}

void GxHScrollBar::IntScrollRight(void)
{
  scrollRightCB();
}
