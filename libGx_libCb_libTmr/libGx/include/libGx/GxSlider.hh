#ifndef GXSLIDER_INCLUDED
#define GXSLIDER_INCLUDED

#include <libGx/GxOwnerWin.hh>
#include <libGx/GxSlideGrip.hh>
#include <libGx/GxFraction.hh>

class GxSlider : public GxOwnerWin
{
protected:
  GxSlider(GxRealOwner *pOwner);
public:
  virtual ~GxSlider(void);

  virtual void SetSlideFr(const GxFraction &rFraction);
  virtual const GxFraction& GetSlideFr(void) const;
  virtual void SetSliderSize(UINT newSize);

  virtual void DisplayChildren(void);

  void Active(bool newState); //we record active in our slider. we don't store here.
  bool Active(void) const;

  CbOneFO<GxFraction> frCB;

  void HandleEvent(const XEvent &rEvent);
protected:
  void GetWindowData(XSetWindowAttributes &winAttributes, ULINT &valueMask);

  GxSlideGrip *pGrip;
  UINT sliderSize; //the height of the slider if we are vertical
  GxFraction slideFr; //how far the slider has slid;
};

class GxHSlider : public GxSlider
{
public:
  GxHSlider(GxRealOwner *pOwner);
  virtual ~GxHSlider(void);

  virtual void SetSlideFr(const GxFraction &rFraction);

  virtual void Resize(UINT newWidth, UINT newHeight);
  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  virtual void PlaceChildren(void);

protected:
  void SGCallback(int xSlide);
  GxHSlideGrip hSlideGrip;
};

class GxVSlider : public GxSlider
{
public:
  GxVSlider(GxRealOwner *pOwner);
  virtual ~GxVSlider(void);

  virtual void SetSlideFr(const GxFraction &rFraction);

  virtual void Resize(UINT newWidth, UINT newHeight);
  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  virtual void PlaceChildren(void);

protected:
  void SGCallback(int ySlide);
  GxVSlideGrip vSlideGrip;
};

#endif //GXSLIDER_INCLUDED
