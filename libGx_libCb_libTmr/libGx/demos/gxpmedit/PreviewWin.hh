#ifndef PREVIEWWIN_INCLUDED
#define PREVIEWWIN_INCLUDED

#include <X11/Xlib.h>

#include <iostream>

#include <libGx/GxWin.hh>

#include "ImgData.hh"

class PreviewWin : public GxWin
{
public:
  PreviewWin(GxRealOwner *pOwner);
  virtual ~PreviewWin(void);

  void SizePixmap(UINT newWidth, UINT newHeight);
  void UpdatePixmap(const ImgData &rData);

  void Draw(void);

  //overloaded to update pixX & pixY
  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  virtual void Resize(UINT tWidth, UINT tHeight);
  virtual void Create(void); //we overload this to create the pixmap
  virtual void HandleEvent(const XEvent &rEvent);
  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);
private:
  void CreatePixmap(void);

  Pixmap preview;
  //the pixmap is larger than the tPixWidth and tPixHeight passed into the
  //constructor to allow for drawing a border. these numbers represent that
  //larger size
  UINT pixWidth, pixHeight;
  //this is the location of the pixmap; used in XCopyArea to prevent
  //having to calculate it in every expose event
  int xPix, yPix;
};

#endif //PREVIEWWINp_INCLUDED
