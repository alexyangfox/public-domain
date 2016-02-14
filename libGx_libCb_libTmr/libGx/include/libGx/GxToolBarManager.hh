#ifndef GXTOOLBARMANAGER_INCLUDED
#define GXTOOLBARMANAGER_INCLUDED

#include <list>

#include <libGx/GxInc.hh>
#include <libGx/GxDisplay.hh>
#include <libGx/GxToolBarFloatingDock.hh>
#include <libGx/GxToolBar.hh>
#include <libGx/GxToolBarDock.hh>

//this class serves to manage the toolbars for a single app over a single display
class GxToolBarManager
{
public:
  GxToolBarManager(GxDisplay *pOwner);
  virtual ~GxToolBarManager(void);

  //called by the GxToolBarDock constructor
  void AddToolBarDock(GxToolBarDock *pDock);
  void RemoveToolBarDock(GxToolBarDock *pDock);

  //called by the dock itself!
  void DeleteToolBarFloatingDock(GxToolBarFloatingDock *pDock);

  void DisplayToolBar(bool show, GxToolBar *pToolBar);
  void DisplayToolBar(bool show, GxToolBar *pToolBar, GxToolBarDock *pDock);

  //these are called by the toolbar itself
  bool ToolBarShown(GxToolBar *pToolBar); //hackish. should be const.
  //this resets the state of the toolbar to reflect where it was last positioned
  //uses StripToolBarDockState() will call the toolbar's display changeCB
  void HideToolBar(GxToolBar *pToolBar);

  //called by the current owner of the toolbar
  //(a GxToolBarFloatingDock or a GxToolBarDock) in response to a buttonpress event
  //results in a display grab and local event loop
  void MoveToolBar(GxToolBar *pToolBar, GxToolBarFloatingDock *pTWStart,
		   GxToolBarDock *pTDStart);

  void SetGroupWindow(Window xID);
  //will return none if set. if this is set all toolbars managed by this
  //toolbar manager will be minimized together
  Window GetGroupWindow(void);

  void RegisterToolBar(GxToolBar &rToolBar);
  void UnRegisterToolBar(GxToolBar &rToolBar); //remove it from my list and make sure it is not displayed in any docks
  //sort the toolbars according to their toolbar dock & their toolbar rows & row locations
  //then add them to their respective docks
  void OrganizeAndPlaceToolBars(void);

private:
  std::list<GxToolBar*> tbList; //we don't own these toolbars

  GxDisplay *pDisp;

  //when we move a toolbar, we grab all events from the display.
  void HandleGrabbedEvents(const XEvent &rEvent);
  GxToolBar *pToolBarMoving; //the toolbar being actively moved
  unsigned phantomDockRow, phantomRowPlace;
  GxEventHandlerID pushedHandlerID;

  //when we move a toolbar, these hold the object the toolbar starts
  //out belonging to
  GxToolBarFloatingDock *pFDStart;
  GxToolBarDock *pDStart;

  void MoveToolBar(GxToolBar *pToolBar, Window win, int xPix, int yPix);

  bool PresentDock(GxToolBar *pToolBar, std::list<GxToolBarDock*>::iterator &objToRemove);
  bool PresentFloatingDock(GxToolBar *pToolBar, std::list<GxToolBarFloatingDock*>::iterator &objToRemove);

  void AddToolBarToNewFloatingDock(GxToolBar *pToolBar);

  //if returns true either pDestDock or pDestFloating dock will have been modified to point to the dock.
  //this modifies both to be null first.
  bool SelectDockByWindow(Window win, GxToolBarDock **pDestDock, GxToolBarFloatingDock **pDestFloatingDock);

  void LeaveActiveDock(void);
  void PossibleEnterDock(Window win);

  void DrawToolBarOutline(int x, int y); //in root window coordinates.
  void EraseToolBarOutline(void);
  void DoOutlineDraw(int x, int y) const;
  int lastX, lastY;
  bool outlineDrawn;
  unsigned toolBarLength;
  unsigned toolBarVertical;

  //if we are dragging a toolbar, and we currently have the pointer over a dock,
  //this is the pointer to that dock.
  GxToolBarCplxDockCore *pActiveDock;

  Window gWin; //not really used anymore

  std::list<GxToolBarFloatingDock*> floatingDockList;
  std::list<GxToolBarDock*> dockList;
};

#endif //GXTOOLBARMANAGER_INCLUDED
