#ifndef GXSCROLLEDWIN_INCLUDED
#define GXSCROLLEDWIN_INCLUDED

#include <libGx/GxGhost.hh>
#include <libGx/GxScrollBar.hh>
#include <libGx/GxOwnerWin.hh>
#include <libGx/GxRealOwner.hh>

//this base class pretty much only provides geometry management
class GxBaseScrolledWin : public GxGhost
{
public:
  virtual ~GxBaseScrolledWin(void);

  void SetVSpacing(UINT newSpacing);
  void SetHSpacing(UINT newSpacing);

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  virtual void SetDesiredWidth(UINT newW);
  virtual void SetDesiredHeight(UINT newH);

  GxHScrollBar hScrollBar;
  GxVScrollBar vScrollBar;

  virtual void PlaceChildren(void);

protected:
  GxBaseScrolledWin(GxRealOwner *pOwner);

  unsigned desW, desH;

  //set this to whatever window is the clip window. this is used by this
  //class to do geometry management
  GxCoreWin *pClipWin;

  //this is the spacing between vBar and the side of the clip window
  UINT vSpacing;
  //this is the spacing between hBar and the bottom of the clip window
  UINT hSpacing;
};

class GxAutoScrolledWin : public GxBaseScrolledWin
{
public:
  GxAutoScrolledWin(GxRealOwner *pOwner);
  virtual ~GxAutoScrolledWin(void);

  //this is needed so user code can contruct the ScrolledWin
  GxOwnerWin& GetClipWindow(void);

  //feel free to add anything to or resize this window any way you want
  GxOwnerWin *pScrolledWin;
protected:
  GxOwnerWin clipWin;
};

class GxAppScrolledWin : public GxBaseScrolledWin
{
public:
  GxAppScrolledWin(GxRealOwner *pOwner);
  virtual ~GxAppScrolledWin(void);

  //the application is totally responsible for the contents of and
  //events from the clipWin, with the exception of resizes.
  //these can be gotten from the placeCB. (called everytime Place is called)
  //using this method prevents round-trip server loops for resizes.
  //and setting my scrollbar information. this class just manages
  //the geometry of all objects
  void SetClipWindow(GxWin *pWindow);
};

#endif //GXSCROLLEDWIN_INCLUDED
