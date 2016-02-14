#ifndef GXTOOLBARDOCK_INCLUDED
#define GXTOOLBARDOCK_INCLUDED

#include <libGx/GxOwnerWin.hh>
#include <libGx/GxToolBar.hh>
#include <libGx/GxToolBarCplxDockCore.hh>

//we have to be an ownerwin because we want to recieve events
class GxToolBarDock : public GxOwnerWin, public GxToolBarCplxDockCore
{
public:
  //dockID must be non-zero for things to work correctly
  GxToolBarDock(GxRealOwner *pOwner, GxToolBarManager &rTManager, unsigned tDockID);
  virtual ~GxToolBarDock(void);

  //this assumes the toolbar has valid toolbar positions.
  //only should be called by the GxToolBarManager.
  virtual void AddToolBar(GxToolBar *pToolBar);

  virtual void RemoveToolBar(GxToolBar *pToolBar);

  virtual unsigned GetDockID(void);

  //horizontal by default
  void SetVertical(bool newState);

  virtual Window GetDockBaseWindow(void) const;

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  virtual void Place(int &lX, int &rX, int &tY, int &bY);
  virtual void PlaceChildren(void);
  virtual void Create(void);

  virtual void HandleEvent(const XEvent &rEvent);
protected:
  virtual void GetWindowData(XSetWindowAttributes &winAttrib, ULINT &valueMask);
  //start GxToolBarCplxDockCore overloads
  virtual void ResizeAndPlaceDock(void);
  virtual void EraseDock(void);
  virtual void DrawDock(void);
  virtual GxRealOwner& GetToolBarWinOwner(void);
  //end GxToolBarCplxDockCore overloads

  unsigned dockID;
};


#endif //GXTOOLBARDOCK_INCLUDED
