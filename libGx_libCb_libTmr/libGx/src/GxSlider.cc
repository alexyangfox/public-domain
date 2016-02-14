#include <libGx/GxSlider.hh>

#include "GxDefines.hh"

GxSlider::GxSlider(GxRealOwner *pOwner) :
  GxOwnerWin(pOwner), pGrip(0), sliderSize(2)
{}

GxSlider::~GxSlider(void)
{}

void GxSlider::SetSlideFr(const GxFraction &rFraction)
{
  slideFr = rFraction;
}

const GxFraction& GxSlider::GetSlideFr(void) const
{
  return slideFr;
}

void GxSlider::SetSliderSize(UINT newSize)
{
  sliderSize = newSize;
}

void GxSlider::DisplayChildren(void)
{
  if( !pGrip->Active() ) return;
  GxOwnerWin::DisplayChildren();
}

void GxSlider::Active(bool newState)
{
  pGrip->Active(newState);
  if(newState)
    DisplayChildren(); //have to set the new state before calling this
  else
    HideChildren();
}

bool GxSlider::Active(void) const
{
  return pGrip->Active();
}

void GxSlider::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      {
	//hackish. handle active.
	Draw3dBorder(0,0, width,height, false);
      };
}

void GxSlider::GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask)
{
  winAttributes.event_mask = ExposureMask;
  valueMask |= CWEventMask;
}
  

GxHSlider::GxHSlider(GxRealOwner *pOwner) :
  GxSlider(pOwner),
  hSlideGrip(this, CbOneMember<int, GxHSlider>(this, &GxHSlider::SGCallback) )
{
  pGrip = &hSlideGrip;
  pGrip->Resize(sliderSize*GX_SLIDER_WIDTH,
		GX_SLIDER_WIDTH - 2*GX_BORDER_WD);
  pGrip->Move(GX_BORDER_WD, GX_BORDER_WD);
}

GxHSlider::~GxHSlider(void)
{}

void GxHSlider::SetSlideFr(const GxFraction &rFraction)
{
  slideFr = rFraction;
  UINT gWidth = pGrip->Width();
  UINT slideDist = width - 2*GX_BORDER_WD - gWidth;
  pGrip->Move(GX_BORDER_WD + slideFr.Convert(slideDist), GX_BORDER_WD);
}

void GxHSlider::Resize(UINT newWidth, UINT)
{
  GxSlider::Resize(newWidth, GX_SLIDER_WIDTH);
}

UINT GxHSlider::GetDesiredWidth(void) const
{
  return GX_DEFAULT_SLIDER_LENGTH;
}

UINT GxHSlider::GetDesiredHeight(void) const
{
  return GX_SLIDER_WIDTH;
}

void GxHSlider::PlaceChildren(void)
{
  UINT gWidth = (int)pGrip->Width();
  UINT slideDist = width - 2*GX_BORDER_WD - gWidth;

  pGrip->Move(GX_BORDER_WD + slideFr.Convert(slideDist), GX_BORDER_WD);
}

void GxHSlider::SGCallback(int xSlide)
{
  int cX = pGrip->X();

  UINT gWidth = (int)pGrip->Width();
  UINT slideDist = width - 2*GX_BORDER_WD - gWidth;
  GxFraction newFr;

  if(xSlide > 0) //sliding down
    {
      if( (cX + xSlide) >= (int)(width - GX_BORDER_WD - gWidth) )
	{
	  pGrip->Move((int)(width - GX_BORDER_WD - gWidth), GX_BORDER_WD);
	  newFr = GX_MAX_FRACTION;
	}else
	  {
	    pGrip->Move(cX + xSlide, GX_BORDER_WD);
	    newFr =  GxFraction(cX + xSlide - GX_BORDER_WD, slideDist);
	  };
    }else
      {
	if( (cX + xSlide) <= (int)GX_BORDER_WD)
	  {
	    pGrip->Move(GX_BORDER_WD, GX_BORDER_WD);
	    newFr = GX_MIN_FRACTION;
	  }else
	    {
	      pGrip->Move(cX + xSlide, GX_BORDER_WD);
	      newFr = GxFraction(cX + xSlide - GX_BORDER_WD, slideDist);
	    };
      };

  if(newFr != slideFr)
    {
      slideFr = newFr;
      frCB(slideFr);
    };
}

GxVSlider::GxVSlider(GxRealOwner *pOwner) :
  GxSlider(pOwner),
  vSlideGrip(this, CbOneMember<int, GxVSlider> (this, &GxVSlider::SGCallback) )
{
  pGrip = &vSlideGrip;
  pGrip->Resize(GX_SLIDER_WIDTH - 2*GX_BORDER_WD,
		sliderSize*GX_SLIDER_WIDTH);
  pGrip->Move(GX_BORDER_WD, GX_BORDER_WD);
}

GxVSlider::~GxVSlider(void)
{}

void GxVSlider::SetSlideFr(const GxFraction &rFraction)
{
  slideFr = rFraction;
  UINT gHeight = pGrip->Height();
  UINT slideDist = height - 2*GX_BORDER_WD - gHeight;
  pGrip->Move(GX_BORDER_WD, GX_BORDER_WD + slideFr.Convert(slideDist));
}

void GxVSlider::Resize(UINT, UINT newHeight)
{
  GxSlider::Resize(GX_SLIDER_WIDTH, newHeight);
}

UINT GxVSlider::GetDesiredWidth(void) const
{
  return GX_SLIDER_WIDTH;
}

UINT GxVSlider::GetDesiredHeight(void) const
{
  return GX_DEFAULT_SLIDER_LENGTH;
}

void GxVSlider::PlaceChildren(void)
{
  UINT gWidth = (int)pGrip->Width();
  UINT slideDist = width - 2*GX_BORDER_WD - gWidth;

  pGrip->Move(GX_BORDER_WD, GX_BORDER_WD + slideFr.Convert(slideDist));
}

void GxVSlider::SGCallback(int ySlide)
{
  int cY = pGrip->Y();

  UINT gHeight = (int)pGrip->Height();
  UINT slideDist = height - 2*GX_BORDER_WD - gHeight;
  GxFraction newFr;

  if(ySlide > 0) //sliding down
    {
      if( (cY + ySlide) >= (int)(height - GX_BORDER_WD - gHeight) )
	{
	  pGrip->Move(GX_BORDER_WD, (int)(height - GX_BORDER_WD - gHeight) );
	  newFr = GX_MAX_FRACTION;
	}else
	  {
	    pGrip->Move(GX_BORDER_WD, cY + ySlide);
	    newFr = GxFraction(cY + ySlide - GX_BORDER_WD, slideDist);
	  };
    }else
      {
	if( (cY + ySlide) <= (int)GX_BORDER_WD)
	  {
	    pGrip->Move(GX_BORDER_WD, GX_BORDER_WD);
	    newFr = GX_MIN_FRACTION;
	  }else
	    {
	      pGrip->Move(GX_BORDER_WD, cY + ySlide);
	      newFr = GxFraction(cY + ySlide - GX_BORDER_WD, slideDist);
	    };
      };

  if(newFr != slideFr)
    {
      slideFr = newFr;
      frCB(slideFr);
    };
}
