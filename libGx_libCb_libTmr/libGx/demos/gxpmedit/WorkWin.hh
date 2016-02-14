#ifndef WORKWIN_INCLUDED
#define WORKWIN_INCLUDED

#include <X11/Xlib.h>

#include <vector>

#include <libCb/CbCallback.hh>

#include <libGx/GxWin.hh>

#include "ImgData.hh"

class DrawData
{
public:
  enum DATA_TYPE{DATA_POINT, DATA_LINE, DATA_LINE_RUBBER, DATA_AREA, DATA_AREA_RUBBER};

  DrawData(DATA_TYPE tDType) :
    dType(tDType), valid(true) {};
  ~DrawData(void){};

  DATA_TYPE dType;

  unsigned pointX;
  unsigned pointY;

  bool valid; //only currently applies to areas and lines

  /* area variables */
  unsigned area_x;
  unsigned area_y;
  unsigned width;
  unsigned height;

  /* line variables */
  unsigned line_x1, line_y1, line_x2, line_y2;
};

enum WW_STATE{WW_DRAW, WW_DRAW_CONT, WW_DRAW_LINE, WW_SEL_AREA};

class WorkWin : public GxWin
{
public:
  WorkWin(GxRealOwner *pOwner, UINT tPixWidth, UINT tPixHeight, int scaleFactor);
  virtual ~WorkWin(void);

  void SizePixmap(UINT newWidth, UINT newHeight);

  //shouldn't this be an unsigned
  void Scale(unsigned newScale);

  void Update(const ImgData &rImgData);

  void SetState(WW_STATE newState);
  void Draw(void);

  CbVoidFO redrawCB;
  CbOneFO<const DrawData&> dataEventCB;

  virtual void Resize(UINT tWidth, UINT tHeight);
  virtual void Create(void); //we overload this to create the pixmap
  virtual void HandleEvent(const XEvent &rEvent);
  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);
private:
  WW_STATE cState;

  void StartPointEvent(unsigned x1, unsigned y1);
  void MotionPointEvent(unsigned x1, unsigned y1);
  void EndPointEvent(unsigned x1, unsigned y1, bool valid);

  unsigned startX, startY;

  void BuildSelArea(DrawData &rData, unsigned cX, unsigned cY) const;

  //creates the basic pixmap and draws the borders of the enlarged "pixels"
  void CreatePixmap(void);
  //from the xClick and yClick tries to find out if the click was within
  //a virtual pixel of workMap. If it was put that place in xPix&yPix and
  //return TRUE otherwise return FALSE
  bool LookupPlace(UINT &xPix, UINT &yPix, int xClick, int yClick);


  Pixmap workMap;

  int pixWidth, pixHeight;
  int scale;
  int realPixWidth, realPixHeight;
  int realPixX, realPixY;
};

#endif //WORKWIN_INCLUDED
