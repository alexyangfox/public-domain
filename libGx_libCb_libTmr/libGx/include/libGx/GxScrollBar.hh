#ifndef GXSCROLLBAR_INCLUDED
#define GXSCROLLBAR_INCLUDED

#include <libCb/CbCallback.hh>
#include <libTmr/TmrTimer.hh>

#include <libGx/GxFraction.hh>
#include <libGx/GxWin.hh>
#include <libGx/GxOwnerWin.hh>
#include <libGx/GxArrowButton.hh>
#include <libGx/GxSlideGrip.hh>

class GxScrollButton : public GxWin
{
public:
  GxScrollButton(GxRealOwner *pOwner, GX_DIRECTION bDir);
  virtual ~GxScrollButton(void);

  void HandleEvent(const XEvent &rEvent);

  CbVoidFO cb;

protected:
  void GetWindowData(XSetWindowAttributes &winAttributes,  ULINT &valueMask);

  TmrTimer sbTimer;
  void TimerCB(void);

  void DrawButton(void);
  GX_DIRECTION dir;
  bool pressed;
};

class GxScrollBar : public GxOwnerWin
{
public:
  GxScrollBar(GxRealOwner *pOwner);
  virtual ~GxScrollBar(void);

  //the fraction of stuff which is visable. this can be set and reset without
  //regard to below. If it must be changed we call our callback
  virtual void SetTotalFraction(GxFraction tfr) = 0;
  //how far down we have scrolled
  virtual void SetScrollFraction(GxFraction sfr) = 0;
  //same as above, only one call. This avoids Unnecessary PlaceChildren
  virtual void SetBothFractions(GxFraction totalFr, GxFraction scrollFr) = 0;

  //not sure if these should exist: they are totally convience oriented
  virtual void GetTotalFraction(GxFraction &rFr);
  virtual void GetScrollFraction(GxFraction &rFr);

  CbOneFO<GxFraction> scrollCB;

  void HandleEvent(const XEvent &rEvent);
protected:
  void GetWindowData(XSetWindowAttributes &winAttributes, ULINT &valueMask);
  //percentage of stuff showing
  GxFraction totalFr;
  //how far we have scrolled down
  GxFraction scrollFr;
};

class GxVScrollBar : public GxScrollBar
{
public:
  GxVScrollBar(GxRealOwner *pOwner);
  virtual ~GxVScrollBar(void);

  //revert to the GxWin behaviour
  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  virtual void SetTotalFraction(GxFraction tfr);
  virtual void SetScrollFraction(GxFraction sfr);

  virtual void SetBothFractions(GxFraction totalFr, GxFraction scrollFr);

  virtual void PlaceChildren(void);

  CbVoidFO scrollUpCB;
  CbVoidFO scrollDownCB;

private:
  void SlideGripCallback(int dist);

  void IntScrollUp(void);
  void IntScrollDown(void);

  GxScrollButton downButton;
  GxScrollButton upButton;
  GxVSlideGrip vGrip;
};

class GxHScrollBar : public GxScrollBar
{
public:
  GxHScrollBar(GxRealOwner *pOwner);
  virtual ~GxHScrollBar(void);

  //revert to the GxWin behaviour
  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  virtual void SetTotalFraction(GxFraction tfr);
  virtual void SetScrollFraction(GxFraction sfr);

  virtual void SetBothFractions(GxFraction totalFr, GxFraction scrollFr);

  virtual void PlaceChildren(void);

  CbVoidFO scrollLeftCB;
  CbVoidFO scrollRightCB;
private:
  void SlideGripCallback(int dist);

  void IntScrollLeft(void);
  void IntScrollRight(void);

  GxScrollButton rightButton;
  GxScrollButton leftButton;
  GxHSlideGrip hGrip;
};

#endif //GXSCROLLBAR_INCLUDED
