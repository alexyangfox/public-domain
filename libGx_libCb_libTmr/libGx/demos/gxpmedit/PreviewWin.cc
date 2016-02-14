#include "PreviewWin.hh"

PreviewWin::PreviewWin(GxRealOwner *pOwner) :
  GxWin(pOwner),
  pixWidth(27), pixHeight(27)
{
  //no point setting these here because we haven't yet been sized
  //xPix = (width - pixWidth)/2;
  //yPix = (height - pixHeight)/2;
}

PreviewWin::~PreviewWin(void)
{
  if( Created() )
    XFreePixmap(dInfo.display, preview);
}

void PreviewWin::SizePixmap(UINT newWidth, UINT newHeight)
{
  pixWidth = newWidth + 2;
  pixHeight = newHeight + 2;
  xPix = (width - pixWidth)/2;
  yPix = (height - pixHeight)/2;
  
  if( !Created() ) return;

  XClearWindow(dInfo.display, xWin);
  XFreePixmap(dInfo.display, preview);
  CreatePixmap();
  Draw();
}

void PreviewWin::UpdatePixmap(const ImgData &rData)
{
  if( !Created() ) return;
  
  //clear the pixmap with the transparent pixel
  XSetForeground(dInfo.display, vData.borderGC, dInfo.backgroundPix);
  //careful- remember esoteric X-filling we are staying outside of the 1 pixel black border
  XFillRectangle(dInfo.display, preview, vData.borderGC, 1,1, pixWidth-2, pixHeight-2);

  XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
  XDrawRectangle(dInfo.display, preview, vData.borderGC, 0,0, rData.Width()+1, rData.Height()+1);

  for(unsigned ii = 0; ii < rData.Width(); ii++)
    for(unsigned jj = 0; jj < rData.Height(); jj++)
      {
	const PixValue &rVal = rData.GetValue(ii,jj);
	if(!rVal.masked)
	  {
	    XSetForeground(dInfo.display, vData.borderGC, rVal.xcolor);
	    XDrawPoint(dInfo.display, preview, vData.borderGC, ii+1,jj+1);
	  };
      };

  XClearWindow(dInfo.display, xWin);
  Draw();
}

void PreviewWin::Draw(void)
{
  if( !Created() ) return;

  //blit the pixmap to the middle of the window and draw the border
  XCopyArea(dInfo.display, preview, xWin, vData.borderGC, 0,0,
	    pixWidth, pixHeight, xPix, yPix);
  //draw a 3d border around the entire window
  Draw3dBorder(0,0, width, height, true);
}

UINT PreviewWin::GetDesiredWidth(void) const
{
  return pixWidth + 10 + 4;
}

UINT PreviewWin::GetDesiredHeight(void) const
{
  return pixHeight + 10 + 4;
}

void PreviewWin::Resize(UINT tWidth, UINT tHeight)
{
  GxWin::Resize(tWidth, tHeight);
  xPix = (width - pixWidth)/2;
  yPix = (height - pixHeight)/2;
}

void PreviewWin::Create(void)
{
  GxWin::Create();
  CreatePixmap();
}

void PreviewWin::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      {
	Draw();
	return;
      };
}

void PreviewWin::GetWindowData(XSetWindowAttributes &winAttributes,
			       ULINT &valueMask)
{
  winAttributes.event_mask = ExposureMask;
  valueMask |= CWEventMask;
}

void PreviewWin::CreatePixmap(void)
{
  //to prevent having to draw a border around the pixmap with every expose
  //event; we just enlarge the pixmap by 2 pixes in width and height and draw
  //the border into the pixmap
  preview = XCreatePixmap(dInfo.display, xWin, pixWidth, pixHeight,
			  dInfo.cVisualInfo.depth);

  XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
  XDrawRectangle(dInfo.display, preview, vData.borderGC, 0,0,
		 pixWidth-1, pixHeight-1);
}
