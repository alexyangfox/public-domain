#include "WorkWin.hh"

#include <iostream>
using namespace std;

WorkWin::WorkWin(GxRealOwner *pOwner, UINT tPixWidth, UINT tPixHeight,
		 int scaleFactor) :
  GxWin(pOwner), cState(WW_DRAW), pixWidth(tPixWidth), pixHeight(tPixHeight),
  scale(scaleFactor)
{
  realPixWidth = 0;
  realPixHeight = 0;
  SizePixmap(pixWidth, pixHeight);
}

WorkWin::~WorkWin(void)
{
  if( Created() )
    XFreePixmap(dInfo.display, workMap);
}

void WorkWin::SizePixmap(UINT newWidth, UINT newHeight)
{
  pixWidth = newWidth;
  pixHeight = newHeight;
  realPixWidth = (pixWidth*(scale+1))+1;
  realPixHeight = (pixHeight*(scale+1))+1;

  if( !Created() ) return;

  XFreePixmap(dInfo.display, workMap);
  XClearWindow(dInfo.display, xWin);
  CreatePixmap();
}

void WorkWin::Scale(unsigned newScale)
{
  if( (unsigned)scale == newScale ) return;

  scale = newScale;
  if( !Created() ) return;

  SizePixmap(pixWidth, pixHeight);
}

void WorkWin::Update(const ImgData &rImgData)
{
  if( !Created() ) return;

  for(int ii = 0; ii < pixWidth; ii++)
    for(int jj = 0; jj < pixHeight; jj++)
	{
	  const PixValue &rVal = rImgData.GetValue(ii, jj);
	  ULINT pixVal = rVal.xcolor;
	  
	  int xPlace = 1 + (ii*(scale+1));
	  int yPlace = 1 + (jj*(scale+1));

	  XSetForeground(dInfo.display, vData.borderGC, pixVal);
	  XFillRectangle(dInfo.display, workMap, vData.borderGC,
			 xPlace, yPlace,scale, scale);

	  if( rVal.masked ) //if bit is 0 -> graphics not drawn
	    {
	      XSetForeground(dInfo.display, vData.borderGC, dInfo.whitePix);
	      XFillRectangle(dInfo.display, workMap, vData.borderGC,
			     xPlace, yPlace,scale/2, scale/2);
	      XFillRectangle(dInfo.display, workMap, vData.borderGC,
			     xPlace+scale/2, yPlace+scale/2, scale/2, scale/2);

	      XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
	      XFillRectangle(dInfo.display, workMap, vData.borderGC,
			     xPlace, yPlace+scale/2, scale/2, scale/2);
	      XFillRectangle(dInfo.display, workMap, vData.borderGC,
			     xPlace+scale/2, yPlace, scale/2, scale/2);
	    };

	  if( rVal.selected )
	    {
	      XSetForeground(dInfo.display, vData.borderGC, dInfo.whitePix);
	      XSetFunction(dInfo.display, vData.borderGC, GXxor);
	      XFillRectangle(dInfo.display, workMap, vData.borderGC,
			     xPlace,yPlace, scale,scale);
	      XSetFunction(dInfo.display, vData.borderGC, GXcopy);	      
	    };
	};

  //now draw the borders around the entire pixmap and around each pixel
  XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
  for(int ii = 0; ii < pixHeight+1; ii++)
    {
      //horizontal line
      XDrawLine(dInfo.display, workMap, vData.borderGC, 0, (ii*(scale+1)),
		realPixWidth, (ii*(scale+1)));

    };

  for(int ii = 0; ii < pixWidth+1; ii++)
    {
      //vertical line
      XDrawLine(dInfo.display, workMap, vData.borderGC, (ii*(scale+1)), 0,
		(ii*(scale+1)), realPixHeight);

    };

  //XClearWindow(dInfo.display, xWin);
  Draw();
}

void WorkWin::SetState(WW_STATE newState)
{
  //std::cout << "WorkWin::SetState" << std::endl;
  cState = newState;
}

void WorkWin::Draw(void)
{
  if( !Created() ) return;

  //blit the pixmap to the screen
  XCopyArea(dInfo.display, workMap, xWin, vData.borderGC, 0,0,
	    realPixWidth, realPixHeight, realPixX, realPixY);
}

void WorkWin::Resize(UINT tWidth, UINT tHeight)
{
  GxWin::Resize(tWidth, tHeight);
  realPixX = (width - realPixWidth)/2;
  realPixY = (height - realPixHeight)/2;
}

void WorkWin::Create(void)
{
  //std::cout << "workwin::Create" << std::endl;
  GxWin::Create();
  CreatePixmap();
}

void WorkWin::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      {
	Draw();
	return;
      };

  if(rEvent.type == ButtonPress)
    {
      if(rEvent.xbutton.button != 1)
	return;

      UINT clickX, clickY;
      if( !LookupPlace(clickX, clickY, rEvent.xbutton.x, rEvent.xbutton.y) )
	return;

      if(cState == WW_DRAW)
	{
	  XEvent event;
	  while(1)
	    {
	      while( XCheckTypedEvent(dInfo.display, ButtonPress, &event) );
	      XMaskEvent(dInfo.display, ButtonReleaseMask, &event);
	      if(event.xbutton.button == 1)
		{
		  UINT releaseX, releaseY;
		  if(!LookupPlace(releaseX,releaseY, event.xbutton.x,event.xbutton.y))
		    return;

		  if((releaseX == clickX) && (releaseY == clickY))
		    {
		      DrawData dData(DrawData::DATA_POINT);
		      dData.pointX = clickX;
		      dData.pointY = clickY;
		      dataEventCB(dData);
		    };	
		  break;
		};
	    };
	  return;
	};

      if(cState == WW_DRAW_CONT || cState == WW_SEL_AREA || cState == WW_DRAW_LINE)
	{
	  StartPointEvent(clickX, clickY);
	  XEvent event;
	  while(1)
	    {
	      //discard any additional button presses.
	      while( XCheckTypedEvent(dInfo.display, ButtonPress, &event) );
	      XMaskEvent(dInfo.display, (ButtonReleaseMask | Button1MotionMask | LeaveWindowMask), &event);
	      switch(event.type)
		{
		case MotionNotify: //color any we enter
		  UINT motionX, motionY;
		  if(!LookupPlace(motionX, motionY, event.xbutton.x,event.xbutton.y))
		    continue;
		  MotionPointEvent(motionX, motionY);
		  break;
		case ButtonRelease:
		  if(event.xbutton.button == 1)
		    {
		      bool valid = true;
		      if(!LookupPlace(clickX, clickY, event.xbutton.x,event.xbutton.y))
			valid = false;
		      EndPointEvent(clickX, clickY, valid);
		      return;
		    };
		default:
		  //leave window event currently unhandled
		  break;
		};
	    };
	};
      return;
    };
}

void WorkWin::GetWindowData(XSetWindowAttributes &winAttributes,
			    ULINT &valueMask)
{
  winAttributes.event_mask |= ButtonPressMask | ButtonReleaseMask |
    ExposureMask | Button1MotionMask;
  valueMask |= CWEventMask;
}

void WorkWin::StartPointEvent(unsigned x1, unsigned y1)
{
  DrawData dData(DrawData::DATA_POINT); //hack
  switch(cState)
    {
    case WW_DRAW_CONT:
      dData.dType = DrawData::DATA_POINT;
      dData.pointX = x1;
      dData.pointY = y1;
      dataEventCB(dData);
      break;
    case WW_DRAW_LINE:
      dData.dType = DrawData::DATA_LINE_RUBBER;
      dData.line_x1 = x1;
      dData.line_y1 = y1;

      dData.line_x2 = x1;
      dData.line_y2 = y1;
      dataEventCB(dData);

      startX = x1;
      startY = y1;
      break;
    case WW_SEL_AREA:
      dData.dType = DrawData::DATA_AREA_RUBBER;
      dData.area_x = x1;
      dData.area_y = y1;
      dData.width = 1;
      dData.height = 1;
      dataEventCB(dData);

      startX = x1;
      startY = y1;

      break;
    default:
      break; //unknown?
    };
}

void WorkWin::MotionPointEvent(unsigned cX, unsigned cY)
{
  if(cState == WW_SEL_AREA)
    {
      DrawData dData(DrawData::DATA_AREA_RUBBER);
      BuildSelArea(dData, cX, cY);
      dataEventCB(dData);
      return;
    };

  if(cState == WW_DRAW_LINE)
    {
      DrawData dData(DrawData::DATA_LINE_RUBBER);
      dData.line_x1 = startX;
      dData.line_y1 = startY;

      dData.line_x2 = cX;
      dData.line_y2 = cY;
      dataEventCB(dData);
      return;
    };

  if(cState == WW_DRAW_CONT)
    {
      DrawData dData(DrawData::DATA_POINT);
      dData.pointX = cX;
      dData.pointY = cY;
      dataEventCB(dData);
      return;
    };
}

void WorkWin::EndPointEvent(unsigned x, unsigned y, bool valid)
{
  if(cState == WW_SEL_AREA)
    {
      DrawData dData(DrawData::DATA_AREA);
      if(valid)
	BuildSelArea(dData, x,y);
      else
	{
	  BuildSelArea(dData, startX, startY);
	  dData.valid = false;
	};
      dataEventCB(dData);
      return;
    };

  if(cState == WW_DRAW_LINE)
    {
      DrawData dData(DrawData::DATA_LINE);
      dData.valid = valid;

      dData.line_x1 = startX;
      dData.line_y1 = startY;

      dData.line_x2 = x;
      dData.line_y2 = y;
      dataEventCB(dData);
      return;
    };
}

void WorkWin::BuildSelArea(DrawData &rData, unsigned cX, unsigned cY) const
{
  int fWidth = (int)cX - (int)startX;
  int fHeight = (int)cY - (int)startY;
  int winX = startX;
  int winY = startY;

  if(fWidth < 0)
    {
      winX += fWidth;
      fWidth = -fWidth;
    };

  if(fHeight < 0)
    {
      winY += fHeight;
      fHeight = -fHeight;
    };

  fWidth++;
  fHeight++;

  rData.area_x = winX;
  rData.area_y = winY;
  rData.width  = fWidth;
  rData.height = fHeight;
}

void WorkWin::CreatePixmap(void)
{
  if( !Created() ) return;

  realPixWidth = (pixWidth*(scale+1))+1;
  realPixHeight = (pixHeight*(scale+1))+1;

  workMap = XCreatePixmap(dInfo.display, xWin, realPixWidth, realPixHeight,
			  dInfo.cVisualInfo.depth);

  XSetForeground(dInfo.display, vData.borderGC, dInfo.whitePix);
  XFillRectangle(dInfo.display, workMap, vData.borderGC, 0,0, realPixWidth,
		 realPixHeight);

}

bool WorkWin::LookupPlace(UINT &xPix, UINT &yPix, int xClick, int yClick)
{
  //the +1's are for the border
  if( (xClick < (realPixX +1)) || (yClick < (realPixY + 1)) )
    return false;

  int startX = realPixX+1;
  xPix = 0;
  bool xFound = false;
  while( !xFound && (xPix < pixWidth))
    {
      if( (xClick > startX) && (xClick < (startX + (scale+1))) )
	{
	  xFound = true;
	  break;
	};

      startX += (scale+1);
      xPix++;
    };

  if( !xFound )
    return false;

  int startY = realPixY + 1;
  yPix = 0;
  bool yFound = false;
  while( !yFound && (yPix < pixHeight) )
    {
      if( (yClick > startY) && (yClick < (startY + (scale+1))) )
	{
	  yFound = true;
	  break;
	};

      startY += (scale+1);
      yPix++;
    };

  return yFound;
}
