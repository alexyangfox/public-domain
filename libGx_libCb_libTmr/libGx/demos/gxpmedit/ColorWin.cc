#include "ColorWin.hh"


BoxFiller::BoxFiller(const GxDisplayInfo &rDInfo, GxVolatileData &rVData, Drawable tXWin) :
  dInfo(rDInfo), vData(rVData), xWin(tXWin)
{}

BoxFiller::~BoxFiller(void)
{}

void BoxFiller::Fill(const ColorDef &rColor, int x, int y, unsigned size)
{
  XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
  XDrawRectangle(dInfo.display, xWin, vData.borderGC, x,y, size, size);
  x += 1;
  y += 1;
  size -= 1;

  if(!rColor.defined)
    {
      XSetForeground(dInfo.display, vData.borderGC, dInfo.backgroundPix);
      XFillRectangle(dInfo.display, xWin, vData.borderGC, x, y, size, size);

      XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
      XDrawLine(dInfo.display, xWin, vData.borderGC, x,y, x+size, y+size);
      return;
    };

  if(!rColor.transparent)
    {
      XSetForeground(dInfo.display, vData.borderGC, rColor.xcolor);
      XFillRectangle(dInfo.display, xWin, vData.borderGC, x, y, size, size);
      return;
    };

  //if here we are filling four ways.
  //hack. the +1's here are quite tacky.
  XSetForeground(dInfo.display, vData.borderGC, dInfo.whitePix);
  XFillRectangle(dInfo.display, xWin, vData.borderGC,
		 x,y, size/2, size/2);
  XFillRectangle(dInfo.display, xWin, vData.borderGC,
		 x+size/2, y+size/2, size/2+1, size/2+1);

  XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
  XFillRectangle(dInfo.display, xWin, vData.borderGC,
		 x, y+size/2, size/2+1, size/2+1);
  XFillRectangle(dInfo.display, xWin, vData.borderGC,
		 x+size/2, y, size/2+1, size/2);
}

// *************************** 

ColorDef::ColorDef(void) :
  defined(false), transparent(false), xcolor(0)
{}

ColorDef::ColorDef(const ColorDef &rhs) :
  defined(rhs.defined), transparent(rhs.transparent),
  xcolor(rhs.xcolor), colorName(rhs.colorName)
{} 

ColorDef::ColorDef(Pixel tXColor) :
  defined(false), transparent(false), xcolor(tXColor)
{}

ColorDef::~ColorDef(void)
{}

// *************************** 

ColorWin::ColorWin(GxRealOwner *pOwner, unsigned tStartColor, unsigned tNumColors) :
  GxWin(pOwner), startColor(tStartColor), numColors(tNumColors), colorMap(None), currentColor(dInfo.blackPix)
{
  pixWidth = cellSize * COLOR_COLS +1;
  pixHeight = cellSize * COLOR_ROWS +1;
}

ColorWin::~ColorWin(void)
{
  if(colorMap != None)
    XFreePixmap(dInfo.display, colorMap);
}

const ColorDef& ColorWin::GetDefaultColor(void) const
{
  return allColors[0];
}

void ColorWin::Refresh(void)
{
  if( !Created() ) return;

  FillPixmap();
  //XClearWindow(dInfo.display, xWin);
  DrawInternal();
}

UINT ColorWin::GetDesiredWidth(void) const
{
  return 12 + COLOR_COLS*cellSize;
}

UINT ColorWin::GetDesiredHeight(void) const
{
  return 12 + COLOR_ROWS*cellSize;
}

void ColorWin::Resize(UINT nWidth, UINT nHeight)
{
  GxWin::Resize(nWidth, nHeight);
  xPix = ((int)width - (int)pixWidth)/2;
  yPix = ((int)height - (int)pixHeight)/2;
}

void ColorWin::Create(void)
{
  GxWin::Create();
  colorMap = XCreatePixmap(dInfo.display, xWin, pixWidth, pixHeight, dInfo.cVisualInfo.depth);
  FillPixmap();
}

void ColorWin::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      {
	DrawInternal();
	return;
      };

  if(rEvent.type == ButtonPress)
    {
      //std::cout << "button press event" << std::endl;
      if(rEvent.xbutton.button != 1 && rEvent.xbutton.button != 2) return;
      unsigned buttonNum = rEvent.xbutton.button;
      int pressColorPlace = GetColorPlace(rEvent.xbutton.x, rEvent.xbutton.y);
      //std::cout << "pressColorPlace: " << pressColorPlace << std::endl;
      if(pressColorPlace < 0) return;
      XEvent event;
      while(1)
	{
	  while( XCheckTypedEvent(dInfo.display, ButtonPress, &event) );
	  XMaskEvent(dInfo.display, ButtonReleaseMask, &event);
	  if(event.xbutton.button == buttonNum)
	    {
	      int releaseColorPlace = GetColorPlace(event.xbutton.x, event.xbutton.y);
	      //std::cout << "releaseColorPlace: " << releaseColorPlace << std::endl;
	      if( (pressColorPlace == releaseColorPlace) &&
		  (pressColorPlace >= (int)startColor) &&
		  (pressColorPlace <  (int)(startColor+numColors)) )
		{
		  if(buttonNum == 1)
		    {
		      if( allColors[pressColorPlace].defined )
			colorChangeCB( allColors[pressColorPlace] );
		      return;
		    }else //buttonNum == 2
		      {
			colorDefineCB( allColors[pressColorPlace] );
			return;
		      };
		};
	      return;
	    };
	};
    };
}

void ColorWin::GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask)
{
  winAttributes.event_mask = ExposureMask | ButtonPressMask |
    ButtonReleaseMask;
  valueMask |= CWEventMask;
}

void ColorWin::FillPixmap(void)
{
  //fill the background with background color.
  XSetForeground(dInfo.display, vData.borderGC, dInfo.backgroundPix);
  XDrawRectangle(dInfo.display, colorMap, vData.borderGC, 0,0, pixWidth, pixHeight);
  XFillRectangle(dInfo.display, colorMap, vData.borderGC, 0,0, pixWidth, pixHeight);

  BoxFiller boxFiller(dInfo, vData, colorMap);

  for(UINT ii = startColor; ii < startColor+numColors; ii++)
    boxFiller.Fill(allColors[ii], ((ii-startColor)%COLOR_COLS)*cellSize, ((ii-startColor)/COLOR_COLS)*cellSize, cellSize);
}

int ColorWin::GetColorPlace(int xPlace, int yPlace)
{
  //std::cout << "got button press: xPix: " << xPix << " yPix: " << yPix << std::endl;
  if( (xPlace < xPix) || (yPlace < yPix) ) return -1;

  int diff = yPlace - yPix;
  int row = diff/cellSize;
  if(row > COLOR_ROWS) return -1;

  diff = xPlace - xPix;
  int column = diff/cellSize;
  if(column > COLOR_COLS) return -1;

  int matrixPlace = (row*COLOR_COLS) + column + startColor;
  if( matrixPlace >= (int)startColor && matrixPlace < (int)(startColor + numColors) )
    return matrixPlace;
  else
    return -1;
}

void ColorWin::DrawInternal(void)
{
  if( !Created() ) return;

  XCopyArea(dInfo.display, colorMap, xWin, vData.borderGC,
	    0,0, pixWidth, pixHeight, xPix, yPix);
}


// ***************************
ColorBox::ColorBox(GxRealOwner *pOwner) :
  GxWin(pOwner), currentColor(dInfo.blackPix)
{}

ColorBox::~ColorBox(void)
{}

UINT ColorBox::GetDesiredWidth(void) const
{
  return 25;
}

UINT ColorBox::GetDesiredHeight(void) const
{
  return 25;
}

void ColorBox::SetCurrentColor(const ColorDef &rColor)
{
  currentColor = rColor;
  if( !Created() ) return;

  DrawInternal();
}

const ColorDef &ColorBox::GetCurrentColor(void) const
{
  return currentColor;
}

void ColorBox::GetCurrentColor(PixValue &rValue) const
{
  rValue.xcolor = currentColor.xcolor;
  rValue.masked = currentColor.transparent;
  rValue.selected = false;
}

void ColorBox::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      {
	DrawInternal();
	return;
      };
}

void ColorBox::GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask)
{
  winAttributes.event_mask = ExposureMask | ButtonPressMask |
    ButtonReleaseMask;
  valueMask |= CWEventMask;
}

void ColorBox::DrawInternal(void)
{
  if( !Created() ) return;

  BoxFiller bFiller(dInfo, vData, xWin);
  bFiller.Fill(currentColor, (width-cellSize)/2, (height-cellSize)/2, cellSize);
}

// *****************

//static decleration
const char* ColorWin::default_color_names[] = {
  "black", "dim gray", "dark grey", "grey", "light grey", "white",
  "red", "green", "blue", "cyan", "magenta", "Yellow",
  "navy", "Cadet Blue", "Dark Green", "green yellow", "brown", "IndianRed",
  "Orange", "maroon", "coral"};

std::vector<ColorDef> ColorWin::allColors;

void ColorWin::AllocDefaultColors(const GxDisplayInfo &rDInfo)
{
  allColors.resize(NUM_COLORS);

  for(UINT ii = 0; ii < NUM_DEFAULT_COLORS-1; ii++)
    {
      XColor screen, exact;
      if( !XAllocNamedColor(rDInfo.display, rDInfo.cMap, default_color_names[ii],
			    &screen, &exact) )
	{
	  std::cout << "color alloc failed" << std::endl;
	  allColors[ii].defined = false;
	  allColors[ii].xcolor = rDInfo.blackPix;
	  allColors[ii].colorName = default_color_names[ii];
	}else
	  {
	    allColors[ii].defined = true;
	    allColors[ii].xcolor = screen.pixel;
	    allColors[ii].colorName = default_color_names[ii];
	  };
    };

  allColors[NUM_DEFAULT_COLORS-1].defined = true;
  allColors[NUM_DEFAULT_COLORS-1].transparent = true;
  allColors[NUM_DEFAULT_COLORS-1].xcolor = rDInfo.whitePix;
  allColors[NUM_DEFAULT_COLORS-1].colorName = "Transparent (none)";
}

void ColorWin::FreeColors(const GxDisplayInfo &rDInfo)
{

}

//end static

