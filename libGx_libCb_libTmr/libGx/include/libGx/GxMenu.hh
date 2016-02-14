#ifndef GXMENU_INCLUDED
#define GXMENU_INCLUDED

#include <libCb/CbCallback.hh>

#include <libGx/GxInc.hh>
#include <libGx/GxOwnerWin.hh>
#include <libGx/GxMenuItemOwner.hh>
#include <libGx/GxRootTransient.hh>
#include <libGx/GxGeomControl.hh>

// hack. this must be tested with activated and deactivated menus on the same menu bar.

class GxMenu;

class GxMenuPane : public GxRootTransient
{
public:
  GxMenuPane(GxMenu *pOwner);
  virtual ~GxMenuPane(void);

  //hack menu panes are children of the root window, so overloading Place
  //to do nothing might be a good idea 
  //void Place(int &lX, int &rX, int &tY, int &bY);
  virtual void PlaceChildren(void);
  void Draw(void);
  void HandleEvent(const XEvent &rEvent);

protected:
  //we only need expose events
  void GetWindowData(XSetWindowAttributes &winAttributes, ULINT &valueMask);
};

class GxMenuBar;

class GxMenu : public GxCoreOwnerWin, public GxMenuItemOwner
{
public:
  GxMenu(GxMenuBar *pMenuBar, const char* pLabel = 0);
  virtual ~GxMenu(void);

  void SetLabel(const char* pLabel);
  void StackRight(bool tStackRight);

  //can this menu be used
  void Active(bool nActive);
  bool Active(void) const;

  //these coordinates are our 0,0 wrt the root window. There must be
  //a better way of obtaining them; see GxMenuBar.cc
  void Select(int xRoot, int yRoot);
  void DeSelect(void);

  // ************* start GxMenuItemOwner overloads *******************
  virtual bool GetButtonHeldMode(void) const; //redirects to menu bar
  virtual void SetButtonHeldMode(bool newMode); //redirects to menu bar
  virtual void EndMenuEventGrab(void); //redirects to menu bar
  virtual GxRealOwner & GetRealOwnerObject(void); //returns the menu pane
  // *********** end GxMenuItemOwner overloads *****************

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  void HandleEvent(const XEvent &rEvent);

  CbVoidFO activateCB; //called just before mapping the menu pane
  CbVoidFO deactivateCB; //called just after unmapping the menu pane
  
protected:
  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);
  void DrawMenu(void);

private:
  char label[GX_SHORT_LABEL_LEN];
  unsigned labelLen;

  bool selected;
  bool active;
  bool stackRight;

  GxMenuPane menuPane;
};

#endif //GXMENU_INCLUDED
