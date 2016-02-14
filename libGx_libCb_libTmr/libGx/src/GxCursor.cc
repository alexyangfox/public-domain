#include <libGx/GxCursor.hh>

GxCursor::GxCursor(const GxDisplayInfo &rTDInfo, GxVolatileData &rTVData) :
  xWin(None), rDInfo(rTDInfo), rVData(rTVData), curXPix(0), textY(0)
{}

GxCursor::~GxCursor(void)
{}

void GxCursor::Draw(bool draw)
{
  //top and bottom bars are 5 pixels
  //drawn at (font->ascent) and (font->descent)
  //with a vertical line drawn from midpoint to midpoint
  if(!draw)
    XSetForeground(rDInfo.display, rVData.borderGC, rDInfo.whitePix);
  else
    XSetForeground(rDInfo.display, rVData.borderGC, rDInfo.blackPix);

  //body of cursor
  XDrawLine(rDInfo.display, xWin, rVData.borderGC,
	    curXPix, textY + rDInfo.pDefaultFont->descent,
	    curXPix, textY - rDInfo.pDefaultFont->ascent);
  //top of cursor
  XDrawLine(rDInfo.display, xWin, rVData.borderGC,
	    curXPix - 2, textY - rDInfo.pDefaultFont->ascent,
	    curXPix + 2, textY - rDInfo.pDefaultFont->ascent);
  //bottom of Cursor
  XDrawLine(rDInfo.display, xWin, rVData.borderGC,
	    curXPix - 2, textY + rDInfo.pDefaultFont->descent,
	    curXPix + 2, textY + rDInfo.pDefaultFont->descent);
}

int GxCursor::XPos(void) const
{
  return curXPix;
}

int GxCursor::TextBaselinePos(void) const
{
  return textY;
}

void GxCursor::SetPos(int tCurXPix, int tTextY)
{
  curXPix = tCurXPix;
  textY = tTextY;

#ifdef LIBGX_DEBUG_BUILD
#include <iostream>
  std::cout << "GxCursor::SetPos curXPix: " << curXPix << " textY:" << textY << std::endl;
#endif //LIBGX_DEBUG_BUILD
}
