#include <libGx/GxVolatileData.hh>

GxVolatileData::GxVolatileData(void) :
  allocated(false)
{}

GxVolatileData::~GxVolatileData(void)
{}

void GxVolatileData::VDAllocAll(GxDisplayInfo &rInfo)
{
  ULINT valuemask;
  XGCValues values;

  valuemask = GCFont | GCForeground;
  values.font = rInfo.pMenuFont->fid;
  values.foreground = rInfo.labelTextPix;
  menuGC = XCreateGC(rInfo.display, rInfo.rootWin, valuemask, &values);

  valuemask = GCLineWidth | GCForeground;
  values.foreground = rInfo.blackPix; //just to set something sane
  values.line_width = 0;
  borderGC = XCreateGC(rInfo.display, rInfo.rootWin, valuemask, &values);
  //XSetLineAttributes(dInfo.display, vData.borderGC, 0, LineSolid, CapButt, JoinMiter);

  valuemask = GCForeground | GCFont;
  values.foreground = rInfo.blackPix; 
  values.font = rInfo.pDefaultFont->fid;
  textGC = XCreateGC(rInfo.display, rInfo.rootWin, valuemask, &values);

  allocated = true;
}
void GxVolatileData::VDFreeAll(GxDisplayInfo &rInfo)
{
  if(!allocated) return;

  XFreeGC(rInfo.display, menuGC);
  XFreeGC(rInfo.display, borderGC);
  XFreeGC(rInfo.display, textGC);
  allocated = false;
}
