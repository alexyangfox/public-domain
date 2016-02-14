#include <libGx/GxToolBar.hh>

#include <libGx/GxToolBarButton.hh>
#include <libGx/GxToolBarManager.hh>

#include "GxDefines.hh"

using namespace std;

GxToolBar::GxToolBar(GxToolBarManager *pTManager, const char *pLabel) :
  pManager(pTManager), displayed(false), imagesCreated(false), tbPlace(GX_TOOLBAR_HIDDEN),
  numButtonsRow(10000), placeX(0), placeY(0), dockRow(0), rowPlace(0)
{
  toolBarName[0] = '\0';
  Label(pLabel);

  if(pManager)
    pManager->RegisterToolBar(*this);
}

GxToolBar::~GxToolBar(void)
{
  if(pManager)
    pManager->UnRegisterToolBar(*this);
}

void GxToolBar::Label(const char *pLabel)
{
  unsigned junkLen;
  GxSetLabel(toolBarName, GX_DEFAULT_LABEL_LEN, pLabel, junkLen);
}

const char* GxToolBar::Label(void) const
{
  return toolBarName;
}

void GxToolBar::AllocateButtonWindows(GxRealOwner *pOwner, std::list<GxToolBarButtonWin*> &rWinList)
{
  list<GxToolBarButton*>::iterator cPlace = buttonList.begin();
  list<GxToolBarButton*>::iterator cEnd = buttonList.end();
  while(cPlace != cEnd)
    {
      GxToolBarButtonWin *pWin = new GxToolBarButtonWin(pOwner, *(*cPlace));
      rWinList.push_back(pWin);

      cPlace++;
    };
}

void GxToolBar::SetDesDock(GX_TOOLBAR_LOCATION tDesDock)
{
  tbPlace = tDesDock;
}

GX_TOOLBAR_LOCATION GxToolBar::GetDesDock(void) const
{
  return tbPlace;
}

void GxToolBar::SetDesDockPlace(unsigned tDockRow, unsigned tRowPlace)
{
  dockRow = tDockRow;
  rowPlace = tRowPlace;
}

void GxToolBar::GetDesDockPlace(unsigned &rDockRow, unsigned &rRowPlace) const
{
  rDockRow = dockRow;
  rRowPlace = rowPlace;
}

void GxToolBar::PlaceButtons(std::list<GxToolBarButtonWin*> &rButtonWinList, bool vert, int x, int y) const
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxToolBar::PlaceButtons" << endl;
#endif //LIBGX_DEBUG_BUILD

  int cX = x;
  int cY = y;

  list<GxToolBarButtonWin*>::iterator cPlace = rButtonWinList.begin();
  list<GxToolBarButtonWin*>::iterator cEnd = rButtonWinList.end();
  while(cPlace != cEnd)
    {
      GxToolBarButtonWin *pWin = *cPlace;

      pWin->Move(cX, cY);
      pWin->Resize(GX_TOOLBAR_BUTTON_SIZE, GX_TOOLBAR_BUTTON_SIZE);
      if(vert)
	cY += GX_TOOLBAR_BUTTON_SIZE;
      else
	cX += GX_TOOLBAR_BUTTON_SIZE;
      cPlace++;
    };
}

void GxToolBar::AddButton(GxToolBarButton *pNewButton)
{
  buttonList.push_back(pNewButton);
}

void GxToolBar::RemoveButton(const GxToolBarButton *pButton)
{
  list<GxToolBarButton*>::iterator cPlace = buttonList.begin();
  list<GxToolBarButton*>::iterator cEnd = buttonList.end();
  while(cPlace != cEnd)
    {
      if(*cPlace == pButton)
	cPlace = buttonList.erase(cPlace);
      else
	cPlace++;
    };

}

void GxToolBar::DesiredSize(UINT &rWidth, UINT &rHeight)
{
  rHeight = GX_TOOLBAR_BUTTON_SIZE + 2*GX_TOOLBAR_GAP;
  rWidth = 2*GX_TOOLBAR_GAP + buttonList.size()*GX_TOOLBAR_BUTTON_SIZE;
}

/*
bool GxToolBar::Displayed(void)
{
  return displayed;
}
*/

UINT GxToolBar::BarLength(void) const
{
  //should not ever happen; but if it does, we want to see something is wrong
  if( buttonList.empty() ) return GX_BORDER_WD;

  return buttonList.size()*GX_TOOLBAR_BUTTON_SIZE;
}
