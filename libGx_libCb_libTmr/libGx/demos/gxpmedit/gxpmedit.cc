#include <libGx/GxMainInterface.hh>
#include <libGx/GxDisplay.hh>
#include <X11/xpm.h>

#include "GxpmCore.hh"
#include "PreviewWin.hh"
#include "ColorWin.hh"
#include "GxpmGui.hh"

int main(int argc, char ** argv)
{
  GxMainInterface mainInt("gxpmedit");
  if( !mainInt.Initialize(argc, argv) ) return -1;
  if( !mainInt.OpenAllocateAll() ) return -2;

  GxDisplayInfo &rInfo = mainInt.dispVector[0]->GetDisplayInfo();
  ColorWin::AllocDefaultColors(rInfo);

  appXData xDat;
  xDat.pDisplay = rInfo.display;
  xDat.whitePix = rInfo.whitePix;
  xDat.pVis = rInfo.cVisualInfo.visual;
  xDat.cmap = rInfo.cMap;
  xDat.xdepth = rInfo.cVisualInfo.depth;

  GxpmCore gxpmCore(xDat);
  GxpmGui gui(mainInt.dispVector[0], &gxpmCore);

  mainInt.EventLoop();
  
  return 0;
};
