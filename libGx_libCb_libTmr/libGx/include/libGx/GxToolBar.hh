#ifndef GXTOOLBAR_INCLUDED
#define GXTOOLBAR_INCLUDED

#include <list>
#include <libCb/CbCallback.hh>

#include <libGx/GxRealOwner.hh>

typedef UINT GX_TOOLBAR_LOCATION;
const GX_TOOLBAR_LOCATION GX_TOOLBAR_HIDDEN = 0;
const GX_TOOLBAR_LOCATION GX_TOOLBAR_ROOT_WIN = 65535;

class GxToolBarButton;
class GxToolBarButtonWin;
class GxToolBarManager;

//a GxToolBar can be moved and relocated to another GxToolBarManager, or to the rootwindow
//_if_ it works together with a GxToolBarManager.
//the only time pManager should be NULL is if the toolBar belongs to a simple dock (no relocation allowed)
class GxToolBar
{
public:
  GxToolBar(GxToolBarManager *pTManager, const char *pLabel = 0);
  ~GxToolBar(void);

 //the label is used for the toolbar popup menu.
  void Label(const char *pLabel);
  const char* Label(void) const;

  //we fill rWinList with the button windows
  void AllocateButtonWindows(GxRealOwner *pOwner, std::list<GxToolBarButtonWin*> &rWinList);
  void PlaceButtons(std::list<GxToolBarButtonWin*> &rButtonWinList, bool vert, int x, int y) const;

  void SetDesDock(GX_TOOLBAR_LOCATION tDesDock);
  GX_TOOLBAR_LOCATION GetDesDock(void) const;

  //The dock positions only make sense to the dock the toolbar is currently located in
  void SetDesDockPlace(unsigned dockRow, unsigned rowPlace);
  void GetDesDockPlace(unsigned &rDockRow, unsigned &rRowPlace) const;

  //called by the toolbarButton's in their constructor
  void AddButton(GxToolBarButton *pNewButton);
  void RemoveButton(const GxToolBarButton *pButton);

  void DesiredSize(UINT &rWidth, UINT &rHeight);

  //returns true if I am currently displayed whether in a GxToolBarWin or
  //in a GxToolBarManager
  //bool Displayed(void);

  //returns the length of all of the buttons of the toolbar set end to end
  UINT BarLength(void) const;

  //called when the display state changes. hackish. called by the tool bar manager. the argument is the new
  //display state. this is so that external tools can track which toolbars are visible.
  //?hack?
  CbOneFO<bool> displayChangeCB;

private:
  //the application interface to the entire toolbar subsystem is through
  //GxToolBar's (and GxToolBarManagers to a lesser extent).  If the app
  //wants a toolbar hidden, it is hidden by a function call to that toolbar,
  //resulting in the distrcution of 
  //its owner (if it is a toplevel win) or the change in geometry of the 
  //GxToolBarManager, all seamlessly to the application (and the programmer).
  GxToolBarManager *pManager;

  bool displayed;
  //because all libGx widgets share the same visual, we only need to create
  //the images in the toolbar buttons once.
  bool imagesCreated;

  std::list<GxToolBarButton*> buttonList;

  //none of this information is kept updated continuously
  GX_TOOLBAR_LOCATION tbPlace;
  UINT numButtonsRow;
  int placeX, placeY;
  unsigned dockRow, rowPlace;

  char toolBarName[GX_DEFAULT_LABEL_LEN];
};

#endif //GXTOOLBAR_INCLUDED
