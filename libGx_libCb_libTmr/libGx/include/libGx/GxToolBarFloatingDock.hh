#ifndef GXTOOLBARFLOATINGDOCK_INLCUDED
#define GXTOOLBARFLOATINGDOCK_INLCUDED

#include <libGx/GxToolBar.hh>
#include <libGx/GxTopLevelWin.hh>
#include <libGx/GxToolBarCplxDockCore.hh>

/*
  the toolbar floating dock is a child of the toplevel window which handles either
  a single or group of GxToolBars this is not a transient window. In win32, I have
  always seen these decorated slightly differently, I don't know if this is a good idea,
  or even if the ICCCM supports such a thing
*/
class GxToolBarManager;
//?GxToobarFreeDock?
class GxToolBarFloatingDock : public GxTopLevelWin, public GxToolBarCplxDockCore
{
public:
  GxToolBarFloatingDock(GxDisplay *pOwner, GxToolBarManager &rTManager);
  virtual ~GxToolBarFloatingDock(void);

  void AddToolBar(GxToolBar *pNewToolBar);
  void RemoveToolBar(GxToolBar *pToolBar);

  bool DockUsed(void); //returns true if we have toolbars.

  void GetGeom(int &rXRoot, int &rYRoot, UINT &rNumButtonsRow);

  //overload
  virtual Window GetDockBaseWindow(void) const;

  //size myself based on by toolbar
  virtual void Place(void);
  virtual void Create(void); //done to grab buttons

  virtual void HandleEvent(const XEvent &rEvent);

protected:
  virtual void DeleteWindow(void);

  virtual void GetWMProperties(XTextProperty &winName, XTextProperty &iconName,
			       XSizeHints &rSizeHints, XWMHints &rWMHints,
			       std::list<Atom> &rWMProtocolList);

  virtual void GetWindowData(XSetWindowAttributes &winAttrib,
			     ULINT &valueMask);

  //start GxToolBarCplxDockCore overloads
  virtual void ResizeAndPlaceDock(void);
  virtual void EraseDock(void);
  virtual void DrawDock(void);
  virtual GxRealOwner& GetToolBarWinOwner(void);
  //end GxToolBarCplxDockCore overloads
};

#endif //GXTOOLBARFLOATINGDOCK_INLCUDED
