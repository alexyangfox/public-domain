#include <libGx/GxCoreWin.hh>

#include <libGx/GxOwner.hh>

#include "GxDefines.hh"

GxCoreWin::GxCoreWin(GxOwner *pOwner) :
  GxWinArea(pOwner), xWin(None),
  dInfo(pWinAreaOwner->GetDisplayInfo()),
  vData(pWinAreaOwner->GetVolatileData())
{}

GxCoreWin::~GxCoreWin(void)
{
  if( Created() )
    {
      if(pWinAreaOwner) //the owner may have (improperly) died before me.
	pWinAreaOwner->UnManageWindow(xWin);
      XDestroyWindow(dInfo.display, xWin);
      xWin = None;
    };
}

void GxCoreWin::Resize(UINT tWidth, UINT tHeight)
{
  if(!Created())
    {
      width = tWidth;
      height = tHeight;
      //GxWinArea::Resize(tWidth, tHeight);
    }else
      {
	if((tWidth == width) && (tHeight == height))
	  return;
	
	width = tWidth;
	height = tHeight;
	XResizeWindow(dInfo.display, xWin, width, height);
      };
}

void GxCoreWin::Move(int newX, int newY)
{
  if(!Created())
    GxWinArea::Move(newX, newY);
  else
    {
      if(newX == x && newY == y)
	return;

      x = newX;
      y = newY;
      XMoveWindow(dInfo.display, xWin, x,y);
    };
}

void GxCoreWin::Create(void)
{
  if( !Created() )
    CreateXWindow();
}

void GxCoreWin::Display(void)
{
  if(Created())
    XMapWindow(dInfo.display, xWin);
}

void GxCoreWin::Hide(void)
{
  if(Created())
    XUnmapWindow(dInfo.display, xWin);
}

void GxCoreWin::OwnerDeleted(void)
{
  GxWinArea::OwnerDeleted();
  xWin = None;
}

void GxCoreWin::CreateXWindow(void)
{
  XSetWindowAttributes winAttrib;
  winAttrib.background_pixel = dInfo.backgroundPix;
  winAttrib.border_pixel = dInfo.blackPix;
  winAttrib.event_mask = 0;
  ULINT valueMask = CWBackPixel | CWBorderPixel;

  GetWindowData(winAttrib, valueMask);

  xWin = XCreateWindow(dInfo.display, GetParentWindow(), x,y, width,height, 0,
		       dInfo.cVisualInfo.depth, InputOutput, dInfo.cVisualInfo.visual,
		       valueMask, &winAttrib);

  ( this->GetMapHolder() )->ManageNewWin(this, xWin);
}

Window GxCoreWin::GetParentWindow(void)
{
  if(!pWinAreaOwner)
    return None;
  else
    return pWinAreaOwner->GetClosestXWin();
}

GxMapHolder * GxCoreWin::GetMapHolder(void)
{
  //hack; study below
  if(!pWinAreaOwner)
    return (GxMapHolder*)NULL; //try and force a segfault
  else
    return pWinAreaOwner->GetClosestMapHolder(); //wouldn't this segfault anyway?
}

void GxCoreWin::GetWindowData(XSetWindowAttributes &, ULINT &)
{

}

void GxCoreWin::HandleEvent(const XEvent&)
{

}

void GxCoreWin::Draw3dBorder(int bX, int bY, UINT bWidth, UINT bHeight,
			     bool up)
{
  unsigned long ulPix;
  unsigned long lrPix;

  if(up)
    {
      ulPix = dInfo.lightBorderPix;
      lrPix = dInfo.darkBorderPix;
    }else
      {
	ulPix = dInfo.darkBorderPix;
	lrPix = dInfo.lightBorderPix;
      };

  //there is some fishyness if the line width is 1
  //XSetLineAttributes(dInfo.display, vData.borderGC, 0, LineSolid, CapButt, JoinMiter);

  for(UINT ii = 0; ii < GX_BORDER_WD; ii++)
    {
      XPoint ulPoints[3]; //upper left points
      XPoint lrPoints[3]; //lower right points
      
      //lower left corner
      (ulPoints[0]).x = bX + ii;
      (ulPoints[0]).y = bY + bHeight - ii - 1;

      //upper left corner
      (ulPoints[1]).x = bX + ii;
      (ulPoints[1]).y = bY + ii;

      //upper right corner
      (ulPoints[2]).x = bX + bWidth - ii - 1;
      (ulPoints[2]).y = bY + ii;

      lrPoints[0] = ulPoints[0]; //i.e. the lower left corner

      //lower right corner
      (lrPoints[1]).x = bX + bWidth - ii - 1;
      (lrPoints[1]).y = bY + bHeight - ii - 1;

      lrPoints[2] = ulPoints[2]; //i.e. the upper right corner.

      XSetForeground(dInfo.display, vData.borderGC, ulPix);
      XDrawLines(dInfo.display, xWin, vData.borderGC, ulPoints, 3,
		 CoordModeOrigin);
      XSetForeground(dInfo.display, vData.borderGC, lrPix);
      XDrawLines(dInfo.display, xWin, vData.borderGC, lrPoints, 3,
		 CoordModeOrigin);
    };
}

void GxCoreWin::DrawThinBorder(int bX, int bY, UINT bWidth, UINT bHeight)
{
  //GX_THIN_BORDER_WD
  XSetForeground(dInfo.display, vData.borderGC, dInfo.darkBorderPix);
  XDrawRectangle(dInfo.display, xWin, vData.borderGC, bX, bY, bWidth-1, bHeight-1);
}

void GxCoreWin::Draw3dArrow(int cX, int cY, UINT size,
			    GX_DIRECTION dir, bool up)
{
  ULINT topLeftPix;
  ULINT bottomRightPix;
  ULINT fillPix;
  if(up)
    {
      topLeftPix = dInfo.lightBorderPix;
      bottomRightPix = dInfo.darkBorderPix;
      fillPix = dInfo.backgroundPix;
    }else
      {
	topLeftPix = dInfo.darkBorderPix;
	bottomRightPix = dInfo.lightBorderPix;
	fillPix = dInfo.recessedPix;
      };

  size--;
  //there are lots of identical calculations here, and like a bad person;
  //I'm going to rely on the compiler to optimize it.
  XSegment singleSide[2];
  XPoint firstThree[3];
  XPoint secondThree[3];
    switch(dir)
    {
    case GX_UP:
      //must draw the dark lines first so the light ones make shaddow
      //more realistic
      XSetForeground(dInfo.display, vData.borderGC, bottomRightPix);
      //these two lines go from the tip to the lower right corner
      //to the lower left corner
      firstThree[0].x = cX + size/2;
      firstThree[0].y = cY;
      firstThree[1].x = cX + size;
      firstThree[1].y = cY + size;
      firstThree[2].x = cX;
      firstThree[2].y = cY + size;
      secondThree[0].x = cX + size/2;
      secondThree[0].y = cY + 1;
      secondThree[1].x = cX + size - 1;
      secondThree[1].y = cY + size - 1;
      secondThree[2].x = cX + 1;
      secondThree[2].y = cY + size - 1;
      XDrawLines(dInfo.display, xWin, vData.borderGC, firstThree, 3, CoordModeOrigin);
      XDrawLines(dInfo.display, xWin, vData.borderGC, secondThree, 3, CoordModeOrigin);
      //light lines go from the tip to the lower left corner
      singleSide[0].x1 = cX + size/2;
      singleSide[0].y1 = cY;
      singleSide[0].x2 = cX;
      singleSide[0].y2 = cY + size;
      singleSide[1].x1 = cX + size/2;
      singleSide[1].y1 = cY + 1;
      singleSide[1].x2 = cX + 1;
      singleSide[1].y2 = cY + size - 1;
      XSetForeground(dInfo.display, vData.borderGC, topLeftPix);
      XDrawSegments(dInfo.display, xWin, vData.borderGC, singleSide, 2);
      //now we fill the inner triangle; hack; we are doing this whether
      //or not this is necessary. rather than allocating annother XPoint
      //matrix; just reuse firstThree
      firstThree[0].x = cX + size/2;
      firstThree[0].y = cY + 1;
      firstThree[1].x = cX + size - 1;
      firstThree[1].y = cY + size - 1;
      firstThree[2].x = cX + 1;
      firstThree[2].y = cY + size - 1;
      XSetForeground(dInfo.display, vData.borderGC, fillPix);
      XFillPolygon(dInfo.display, xWin, vData.borderGC, firstThree, 3,
		   Convex, CoordModeOrigin);
      return;
    case GX_DOWN:
      //dark lines go from the upper right corner to the middle bottom
      singleSide[0].x1 = cX + size;
      singleSide[0].y1 = cY;
      singleSide[0].x2 = cX + size/2;
      singleSide[0].y2 = cY + size;
      singleSide[1].x1 = cX + size - 1;
      singleSide[1].y1 = cY + 1;
      singleSide[1].x2 = cX + size/2;
      singleSide[1].y2 = cY + size - 1;
      XSetForeground(dInfo.display, vData.borderGC, bottomRightPix);
      XDrawSegments(dInfo.display, xWin, vData.borderGC, singleSide, 2);
      //light lines go across the top of the box then from left top corner
      //to the tip which is at the middle of the bottom
      firstThree[0].x = cX + size;
      firstThree[0].y = cY;
      firstThree[1].x = cX;
      firstThree[1].y = cY;
      firstThree[2].x = cX + size/2;
      firstThree[2].y = cY + size;
      secondThree[0].x = cX + size - 1;
      secondThree[0].y = cY + 1;
      secondThree[1].x = cX + 1;
      secondThree[1].y = cY + 1;
      secondThree[2].x = cX + size/2;
      secondThree[2].y = cY + size - 1;
      XSetForeground(dInfo.display, vData.borderGC, topLeftPix);
      XDrawLines(dInfo.display, xWin, vData.borderGC, firstThree, 3,
		 CoordModeOrigin);
      XDrawLines(dInfo.display, xWin, vData.borderGC, secondThree, 3,
		 CoordModeOrigin);
      //now we fill the inner triangle; hack; we are doing this whether
      //or not this is necessary. rather than allocating annother XPoint
      //matrix; just reuse firstThree
      firstThree[0].x = cX + size/2;
      firstThree[0].y = cY + size - 1;
      firstThree[1].x = cX + 1;
      firstThree[1].y = cY + 1;
      firstThree[2].x = cX + size - 1;
      firstThree[2].y = cY + 1;
      XSetForeground(dInfo.display, vData.borderGC, fillPix);
      XFillPolygon(dInfo.display, xWin, vData.borderGC, firstThree, 3,
		   Convex, CoordModeOrigin);
      return;
    case GX_LEFT:
      //dark lines go from the top right corner to the bottom right corner
      //to the middle of the left side
      firstThree[0].x = cX + size;
      firstThree[0].y = cY;
      firstThree[1].x = cX + size;
      firstThree[1].y = cY + size;
      firstThree[2].x = cX;
      firstThree[2].y = cY + size/2;
      secondThree[0].x = cX + size - 1;
      secondThree[0].y = cY + 1;
      secondThree[1].x = cX + size - 1;
      secondThree[1].y = cY + size - 1;
      secondThree[2].x = cX + 1;
      secondThree[2].y = cY + size/2;
      XSetForeground(dInfo.display, vData.borderGC, bottomRightPix);
      XDrawLines(dInfo.display, xWin, vData.borderGC, firstThree, 3,
		 CoordModeOrigin);
      XDrawLines(dInfo.display, xWin, vData.borderGC, secondThree, 3,
		 CoordModeOrigin);
      //light lines go from the middle of the left side to the upper right
      singleSide[0].x1 = cX;
      singleSide[0].y1 = cY + size/2;
      singleSide[0].x2 = cX + size;
      singleSide[0].y2 = cY;
      singleSide[1].x1 = cX + 1;
      singleSide[1].y1 = cY + size/2;
      singleSide[1].x2 = cX + size - 1;
      singleSide[1].y2 = cY + 1;
      XSetForeground(dInfo.display, vData.borderGC, topLeftPix);
      XDrawSegments(dInfo.display, xWin, vData.borderGC, singleSide, 2);
      //now we fill the inner triangle; hack; we are doing this whether
      //or not this is necessary. rather than allocating annother XPoint
      //matrix; just reuse firstThree
      firstThree[0].x = cX + 1;
      firstThree[0].y = cY + size/2;
      firstThree[1].x = cX + size - 1;
      firstThree[1].y = cY + 1;
      firstThree[2].x = cX + size - 1;
      firstThree[2].y = cY + size - 1;
      XSetForeground(dInfo.display, vData.borderGC, fillPix);
      XFillPolygon(dInfo.display, xWin, vData.borderGC, firstThree, 3,
		   Convex, CoordModeOrigin);
      return;
    default: // GX_RIGHT
      //dark lines go from the upper left corner to the middle of the
      //right side to the lower left corner
      firstThree[0].x = cX;
      firstThree[0].y = cY;
      firstThree[1].x = cX + size;
      firstThree[1].y = cY + size/2;
      firstThree[2].x = cX;
      firstThree[2].y = cY + size;
      secondThree[0].x = cX + 1;
      secondThree[0].y = cY + 1;
      secondThree[1].x = cX + size - 1;
      secondThree[1].y = cY + size/2;
      secondThree[2].x = cX + 1;
      secondThree[2].y = cY + size - 1;
      XSetForeground(dInfo.display, vData.borderGC, bottomRightPix);
      XDrawLines(dInfo.display, xWin, vData.borderGC, firstThree, 3,
		 CoordModeOrigin);
      XDrawLines(dInfo.display, xWin, vData.borderGC, secondThree, 3,
		 CoordModeOrigin);
      //light lines go from the upper left corner lower right corner
      singleSide[0].x1 = cX;
      singleSide[0].y1 = cY;
      singleSide[0].x2 = cX;
      singleSide[0].y2 = cY + size;
      singleSide[1].x1 = cX + 1;
      singleSide[1].y1 = cY + 1;
      singleSide[1].x2 = cX + 1;
      singleSide[1].y2 = cY + size - 1;
      XSetForeground(dInfo.display, vData.borderGC, topLeftPix);
      XDrawSegments(dInfo.display, xWin, vData.borderGC, singleSide, 2);
      //now we fill the inner triangle; hack; we are doing this whether
      //or not this is necessary. rather than allocating annother XPoint
      //matrix; just reuse firstThree
      firstThree[0].x = cX + size - 1;
      firstThree[0].y = cY + size/2;
      firstThree[1].x = cX + 1;
      firstThree[1].y = cY + size - 1;
      firstThree[2].x = cX + 1;
      firstThree[2].y = cY + 1;
      XSetForeground(dInfo.display, vData.borderGC, fillPix);
      XFillPolygon(dInfo.display, xWin, vData.borderGC, firstThree, 3,
		   Convex, CoordModeOrigin);
      return;
    };
}

void GxCoreWin::DrawFilledArrow(int cX, int cY, UINT size, GX_DIRECTION dir)
{
  XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
  XPoint vertices[3];
  switch(dir)
    {
    case GX_UP:
      vertices[0].x = cX + size/2;
      vertices[0].y = cY;
      //terrible hack. attempts to make small arrows look more uniform top/bottom
      //a better solution would be to understand the XFillPolygon call
      //same problem probably exists with left/right arrows.
      if(size < 8) vertices[0].y = cY - 1;
      vertices[1].x = cX + size;
      vertices[1].y = cY + size;
      vertices[2].x = cX;
      vertices[2].y = cY + size;
      break;
    case GX_DOWN:
      vertices[0].x = cX + size/2;
      vertices[0].y = cY + size;
      vertices[1].x = cX;
      vertices[1].y = cY;
      vertices[2].x = cX + size;
      vertices[2].y = cY;
      break;
    case GX_LEFT:
      vertices[0].x = cX;
      vertices[0].y = cY + size/2;
      vertices[1].x = cX + size;
      vertices[1].y = cY;
      vertices[2].x = cX + size;
      vertices[2].y = cY + size;
      break;
    default: //GX_RIGHT
      vertices[0].x = cX + size;
      vertices[0].y = cY + size/2;
      vertices[1].x = cX;
      vertices[1].y = cY + size;
      vertices[2].x = cX;
      vertices[2].y = cY;
      break;
    };

  XFillPolygon(dInfo.display, xWin, vData.borderGC, vertices, 3,
	       Convex, CoordModeOrigin);
}

void GxCoreWin::DrawCheck(int cX, int cY, UINT size, unsigned long checkColor)
{
  XPoint vertices[4];

  //top most point of check mark
  vertices[0].x = cX + size;
  vertices[0].y = cY;

  //bottom most point of check pont
  vertices[1].x = cX + size/3;
  vertices[1].y = cY + size;

  //start (i.e. leftmost) point
  vertices[2].x = cX;
  vertices[2].y = cY + size/2;

  //top of check mark vertex
  vertices[3].x = cX + size/3;
  vertices[3].y = cY + size-4;

  XSetForeground(dInfo.display, vData.borderGC, checkColor);
  XFillPolygon(dInfo.display, xWin, vData.borderGC, vertices, 4,
	       Nonconvex, CoordModeOrigin);
}
