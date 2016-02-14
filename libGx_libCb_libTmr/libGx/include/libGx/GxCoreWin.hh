#ifndef GXCOREWIN_INCLUDED
#define GXCOREWIN_INCLUDED

#include <libGx/GxInc.hh>
#include <libGx/GxWinArea.hh>
#include <libGx/GxDisplayInfo.hh>
#include <libGx/GxVolatileData.hh>
#include <libGx/GxMapHolder.hh>

class GxCoreWin : public GxWinArea
{
public:
  virtual ~GxCoreWin(void);

  virtual void Resize(UINT tWidth, UINT tHeight);
  virtual void Move(int newX, int newY);

  virtual void Create(void);
  virtual void Display(void);
  virtual void Hide(void);
  //sets my window to None & removes pointer from map
  virtual void OwnerDeleted(void);

  //used a lot! particulary in the GxWinMaps.
  inline Window GetWindow(void){return xWin;};

  virtual void HandleEvent(const XEvent &rEvent);

  //used a lot!
  inline bool Created(void) const
  {
    if(xWin == None)
      return false;
    else
      return true;
  };

  //utility function which draws a 3d border. idea is from grafix
  //not sure if should be in some external class
  void Draw3dBorder(int bX, int bY, UINT bWidth, UINT bHeight, bool up);
  void DrawThinBorder(int bX, int bY, UINT bWidth, UINT bHeight);
  void Draw3dArrow(int cX, int cY, UINT size, GX_DIRECTION dir, bool up);
  void DrawFilledArrow(int cX, int cY, UINT size, GX_DIRECTION dir);
  //checkColor is a pixel
  void DrawCheck(int cX, int cY, UINT size, unsigned long checkColor);

protected:
  GxCoreWin(GxOwner *pOwner);

  //this is overloadable to allow for creation of windows which do not share
  //the visual of its parents, etc. (relativly rare occurance)
  //by design this calls many functions internally, and is therefore expensive,
  //however it is only called once, and this design results in savings in
  //memory consumption and protocall requests
  virtual void CreateXWindow(void);

  //used in CreateXWindow; default is to call pOwner->GetClosestXWin();
  //this is overloaded to allow for things like menu panes which belong
  //to menu objects, yet are subwindows of the root window
  virtual Window GetParentWindow(void);

  //used in CreateXWindow. In many some objects like GxPopupWindows it is
  //necessary (or at least very conveinent) for a XWindow to be in the map
  //owned by the object itself. By default calls pOwner->GetClosestMapHoder
  virtual GxMapHolder * GetMapHolder(void);

  //should depth visual and class also be quered for? would be more flexable.
  //probably bad idea. If want more specialized/custimizeable create
  //a new class and overload CreateXWindow.
  virtual void GetWindowData(XSetWindowAttributes &winAttrib,
			     ULINT &valueMask);

  //xWin is set to None in the constructor
  Window xWin;

  GxDisplayInfo &dInfo; //shouldn't this be const?
  GxVolatileData &vData;
};

#endif //GXCOREWIN_INCLUDED
