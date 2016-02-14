#ifndef GXTOOLBARCPLXDOCKCORE_INCLUDED
#define GXTOOLBARCPLXDOCKCORE_INCLUDED

#include <libCb/CbCallback.hh>

#include <libGx/GxToolBar.hh>
#include <libGx/GxToolBarButton.hh>
#include <libGx/GxMenuItems.hh>
#include <libGx/GxPopupMenu.hh>

//tool bar dock coordinates:
/*
if a row nubnuber are used in adding a toolbar to mean inserting 
*/

/*
  this class forms most of the implementation of the GxToolBarDock and the GxToolBarFloating Dock.
  it is a bit tricky in that it owns valid widgets (the popup menu and menu item). this object is itself
  not a real owner. in the constructor we are passing a reference to an object that has not fuly been
  constructed yet.
*/

class GxToolBarManager;

class GxToolBarCplxDockCore
{
protected:
  GxToolBarCplxDockCore(GxRealOwner *pOwner, GxToolBarManager &rTBManager);
public:
  virtual ~GxToolBarCplxDockCore(void);

  //used when dragging a toolbar so the toolBarManager knows how to draw the bar outline
  //********* hack?!!! 
  bool Vertical(void) const;
  bool ToolBarUsed(GxToolBar *pToolBar) const;
  virtual void AddToolBar(GxToolBar *pToolBar) = 0;
  /*
    Used by the ToolbarManager when dragging toolbars around to know
    which dock (if any) is under the pointer
    this returns true if win is a window anywhere within this dock.
    this implementation just looks at our GxToolBarButtonWin's.
  */
  virtual bool WindowInDock(Window win) const;
  virtual Window GetDockBaseWindow(void) const = 0; //returns the base window of the dock.

  virtual void GetPhantomPlace(int startX, int startY, int pointerX, int pointerY,
			       unsigned &rDockRow, unsigned &rRowPlace) const;
  virtual bool SetPhantomVisible(unsigned dockRow, unsigned rowPlace, unsigned phantomLength);
  virtual void HidePhantom(void);
protected:
  GxToolBarManager &rManager;

  virtual void ResizeAndPlaceDock(void) = 0;
  virtual void EraseDock(void) = 0;
  virtual void DrawDock(void) = 0;

  /*
    a phantomBar is a rectangle representing a toolbar that is going to
    be drawn in this dock. phantomRow and phantomPlace follow the same
    rules as described with bool AddToolBar()
  */

  class ToolBarHolder
  {
  public:
    ToolBarHolder(void);
    ToolBarHolder(const ToolBarHolder &rhs);
    ~ToolBarHolder(void);
    const ToolBarHolder& operator=(const ToolBarHolder &rhs);

    void CreateAndDisplayButtonWindows(void);
    void FreeButtonWindows(void);

    GxToolBar *pToolBar;
    unsigned phantomLength; //if !pToolbar, this is the length of the phantom bar drawn in this location.
    std::list<GxToolBarButtonWin*> tbWinList; //the buttons created for the toolbar.
  };

  class ToolBarRow
  {
  public:
    ToolBarRow(void);
    ToolBarRow(const ToolBarRow &rhs);
    ~ToolBarRow(void);

    ToolBarRow& operator=(const ToolBarRow &rhs);

    std::list<ToolBarHolder> barHolderList;
  };

  std::list<ToolBarRow> rowList;

  class ToolBarDetails //a utility container class
  {
  public:
    ToolBarDetails(ToolBarHolder &rTHolder);
    ToolBarDetails(const ToolBarDetails &rhs);
    ~ToolBarDetails(void);

    ToolBarDetails& operator=(const ToolBarDetails &rhs);

    ToolBarHolder &rHolder;
    int cX, cY;
    unsigned vertical;
  };

  //returns true if we found the tool bar. if pToolBar is NULL we remove any phantom bar we may find
  virtual bool RemoveBar(const GxToolBar *pToolBar);
  virtual ToolBarHolder& AddBar(GxToolBar &rToolBar); //the caller decides if it is time to Create() the windows
  //if pToolBar is NULL, this uses the phantom* stuff
  virtual ToolBarHolder& AddBarLL(GxToolBar *pToolBar, unsigned phantomRow, unsigned phantomRowPlace, unsigned phantomLen);

  //hackish to stick here. Used by the floating toolbar dock. before it deletes itelf on wm close
  virtual void SetAllToolBarDisplayState(bool newState);

  virtual GxRealOwner& GetToolBarWinOwner(void) = 0;

  //should only ever be one phantom bar in our rowList db, however this checks everywhere.
  //it also removes empty rows.
  virtual void RemovePhantomBar(void);

  //this is used in GetDesiredWidth and GetDesiredHeight. It is implemented
  //using variable names assuming the toolbars are oriented horizontally.
  //hackish. it does not take into account vertical.
  virtual void GetDesiredDockSize(UINT &rDesiredWidth, UINT &rDesiredHeight) const;

  //sets the row & rowplace numbers of toolbars to fit our internaly required (for placing) scheme
  virtual void NumberToolBars(void);

  virtual void PlaceDockElements(int startX, int startY);
  virtual void PlaceDockElement(ToolBarDetails &rDetails, int junk);

  class DrawInfo
  {
  public:
    DrawInfo(const GxDisplayInfo &rTInfo, GxVolatileData &rTVData, Window tXWin);
    const GxDisplayInfo &rInfo;
    GxVolatileData &rVData;
    Window xWin;
  };
	       
  virtual void DrawDock(const GxDisplayInfo &rInfo, GxVolatileData &rVData, Window xWin, int startX, int startY);
  virtual void DrawDockElement(ToolBarDetails &rDetails, DrawInfo drawInfo);

  class SelectInfo
  {
  public:
    int xClick, yClick;
  };

  virtual GxToolBar* SelectToolBar(int startX, int startY, int tXClick, int tYClick);
  virtual void SelectToolBarElement(ToolBarDetails &rDetails, SelectInfo selInfo);
  GxToolBar *pSelectedBar; //a bit hackish to use a 'global' to implement SelectToolBar

  //calls a FO for each element after calculating its proper position
  template<class T>
  void ElementPosCalc(int startX, int startY, CbTwoFO<ToolBarDetails&, T> &rFO, T argTwo);

  bool vertical; //this dock's orientation.

  //called by button 3 over a toolbar in a dock. allows hiding a toolbar
  void DoToolBarMenu(int rootX, int rootY, GxToolBar *pToolBar);
  void HideToolBarMenuCB(bool arg, GxToolBar *pToolBar);

  GxPopupMenu toolbarMenu;
  GxMenuCheckOption hideItem;
};

#endif //GXTOOLBARCPLXDOCKCORE_INCLUDED
