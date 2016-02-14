#include <string.h>

#ifdef LIBGX_DEBUG_BUILD
#include <iostream>
#include <assert.h>
#endif //LIBGX_DEBUG_BUILD

#include <libGx/GxTabManager.hh>

#include <libGx/GxTabPane.hh>
#include "GxDefines.hh"

using namespace std;

GxTabManager::GxTabManager(GxRealOwner *pOwner) :
  GxOwnerWin(pOwner)
{
  activeNum = 0;
}

GxTabManager::~GxTabManager(void)
{}

void GxTabManager::SetActiveNum(unsigned newActiveNum)
{
  if( Created() ) //hack. what we really want to know is if we have been displayed()
    {
      if(activeNum == newActiveNum) return;
      activeNum = newActiveNum;
      XClearWindow(dInfo.display, xWin);
      DisplayChildren();
      DrawInternal();
    }else
      activeNum = newActiveNum;
}

void GxTabManager::PlaceChildren(void)
{
  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
#ifdef LIBGX_DEBUG_BUILD
      cout << "GxTabManager placing a pane" << endl;
#endif //LIBGX_DEBUG_BUILD
      int lX = GX_BORDER_WD;
      int rX = (int)width - 2*GX_BORDER_WD;
      int tY = GX_BORDER_WD;
      int bY = (int)height - 2*GX_BORDER_WD;
      if(height > 25)
	bY -= 25;

#ifdef LIBGX_DEBUG_BUILD
      assert( (*cPlace) );
#endif //LIBGX_DEBUG_BUILD
      (*cPlace)->Place(lX, rX, tY, bY);
      cPlace++;
    };
}

void GxTabManager::DisplayChildren(void)
{
  unsigned cNum = 1;
  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
#ifdef LIBGX_DEBUG_BUILD
      assert( (*cPlace) );
#endif //LIBGX_DEBUG_BUILD

      if(cNum == activeNum)
	{
#ifdef LIBGX_DEBUG_BUILD
	  cout << "GxTabManager displaying a pane" << endl;
#endif //LIBGX_DEBUG_BUILD
	  (*cPlace)->Display();
	}else
	  ((GxTabPane*)(*cPlace))->Hide();

      cNum++;
      cPlace++;
    };
}

void GxTabManager::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      {
#ifdef LIBGX_DEBUG_BUILD
	cout << "tab manager got expose" << endl;
#endif //LIBGX_DEBUG_BUILD
	DrawInternal();
	return;
      };

  if(rEvent.type == ButtonPress)
    {
#ifdef LIBGX_DEBUG_BUILD
      cout << "got button press" << endl;
#endif //LIBGX_DEBUG_BUILD
      //hack. should transfer active tabs only on button releases
      unsigned id = SelTab(rEvent.xbutton.x, rEvent.xbutton.y);
      if(id && id != activeNum)
	{	  
	  tabChangeCB(id);
	  activeNum = id;
	  XClearWindow(dInfo.display, xWin);
	  DisplayChildren();
	  DrawInternal();
	};
    };
}

void GxTabManager::GetWindowData(XSetWindowAttributes &winAttributes,
				 ULINT &valueMask)
{
  winAttributes.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask;

  valueMask |= CWEventMask;
}

void GxTabManager::DrawInternal(void)
{
  Draw3dBorder(0,0, width, height-25, true);

  int cNum = 1;
  int cX = GX_BORDER_WD + 5;
  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
#ifdef LIBGX_DEBUG_BUILD
      assert( (*cPlace) );
#endif //LIBGX_DEBUG_BUILD
      GxTabPane* pCPlane = (GxTabPane*)(*cPlace);
      /*

      Draw3dBorder(0,0, width,height, !pressed);

      if(haveFocus)
	      {
	      XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
	      //draw a border around the text area of the button
	      XDrawRectangle(dInfo.display, xWin, vData.borderGC,
	      GX_BORDER_WD*2, GX_BORDER_WD*2,
	      width - 1 - GX_BORDER_WD*4, height - 1 - GX_BORDER_WD*4);
	      };
      */
      int active = true;
      if(cNum != activeNum)
	active = false;

      const char *pCName = pCPlane->GetName();
      int nameLen = strlen(pCName);
      unsigned textWidth;

      //hack; I really don't want to re-calculate textX and textY and textWidth
      //every time I go through here
      if(nameLen != 0)
	{
	  textWidth = XTextWidth(dInfo.pDefaultFont, pCName, nameLen);
	}else
	  textWidth = 0;

      if(active) //erase the border above me.
	{
	  XSetForeground(dInfo.display, vData.borderGC, dInfo.backgroundPix);
	  XFillRectangle(dInfo.display, xWin, vData.borderGC, cX+GX_BORDER_WD, height-28, textWidth+10-2*GX_BORDER_WD, 5);
	};

      DrawBottomTab(cX,height-25, textWidth+10, 25-GX_BORDER_WD, active);
  
      if(nameLen != 0)
	{
	  int textX, textY;
	  textX = cX + 5;
	  int textOffset = ( (25 - GX_BORDER_WD) -
			     ((dInfo.pDefaultFont->max_bounds).ascent + (dInfo.pDefaultFont->max_bounds).descent) )/2;
	  textY = (height - 25) + textOffset + (dInfo.pDefaultFont->max_bounds).ascent;

	  XSetForeground(dInfo.display, vData.textGC, dInfo.labelTextPix);
	  XDrawString(dInfo.display, xWin, vData.textGC, textX, textY, pCName, nameLen);
	};

      cX += textWidth + 10;
      cNum++;
      cPlace++;
    };
}

void GxTabManager::DrawBottomTab(unsigned tlX, unsigned tlY, unsigned width, unsigned height, bool active)
{
    if(!active)
      {
	//we want a slightly darker background;
	XSetForeground(dInfo.display, vData.borderGC, dInfo.recessedPix);
	XFillRectangle(dInfo.display, xWin, vData.borderGC, tlX,tlY, width,height);
      };

    XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
    for(int ii = 0; ii < GX_BORDER_WD; ii++)
      {
	XSetForeground(dInfo.display, vData.borderGC, dInfo.lightBorderPix);
	XDrawLine(dInfo.display, xWin, vData.borderGC, tlX+ii,tlY, tlX+ii,tlY+height-ii); //left

	XSetForeground(dInfo.display, vData.borderGC, dInfo.darkBorderPix);
	XDrawLine(dInfo.display, xWin, vData.borderGC, tlX+ii,tlY+height-ii, tlX+width-ii,tlY+height-ii); //bottom

	XDrawLine(dInfo.display, xWin, vData.borderGC, tlX+width-ii,tlY, tlX+width-ii,tlY+height-ii); //right
      };
}

unsigned GxTabManager::SelTab(int xPix, int yPix) const
{
  if(yPix > (height-GX_BORDER_WD) || (yPix < height-25) ) return 0;

  unsigned cNum = 1;
  int cX = GX_BORDER_WD + 5;
  std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
  std::list<GxWinArea*>::const_iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      const GxTabPane* pCPlane = (const GxTabPane*)(*cPlace);
 
      const char *pCName = pCPlane->GetName();
      int nameLen = strlen(pCName);
      unsigned textWidth = 0;

      //hack; I really don't want to re-calculate textX and textY and textWidth
      //every time I go through here
      if(nameLen != 0)
	textWidth = XTextWidth(dInfo.pDefaultFont, pCName, nameLen);

      if( (xPix > cX) && xPix < (cX + textWidth+10) )
	return cNum;

      cX += textWidth+10;
      cNum++;
      cPlace++;
    };

  return 0;
}
