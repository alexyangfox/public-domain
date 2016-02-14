#include <string.h>

#include <libGx/GxTabPane.hh>

#include <libGx/GxTabManager.hh>

#include <libGx/GxGeomControl.hh> //hackish?

GxTabPane::GxTabPane(GxTabManager *pOwner) :
  GxOwnerWin(pOwner)
{
  SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT, GX_FLOW_UP) );
}

GxTabPane::~GxTabPane(void)
{}

void GxTabPane::Hide(void)
{
  if( Created() )
    XUnmapWindow(dInfo.display, xWin);
}

void GxTabPane::SetName(const char *pName)
{
  strncpy(tabName, pName, GX_DEFAULT_LABEL_LEN-1);
  tabName[GX_DEFAULT_LABEL_LEN-1] = '\0';
}

const char* GxTabPane::GetName(void) const
{
  return tabName;
}






