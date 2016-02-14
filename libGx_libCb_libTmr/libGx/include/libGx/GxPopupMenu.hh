#ifndef GXPOPUPMENU_INCLUDED
#define GXPOPUPMENU_INCLUDED

#include <libGx/GxRootTransient.hh>
#include <libGx/GxMenuItemOwner.hh>
#include <libGx/GxSubMapHolder.hh>
#include <libGx/GxMenuItems.hh>

//**** this depends on the fact that only menu items will be added to it *****

class GxPopupMenu : public GxRootTransient, public GxMenuItemOwner, public GxSubMapHolder
{
public:
  GxPopupMenu(GxRealOwner *pOwner);
  virtual ~GxPopupMenu(void);

  virtual void Place(void);
  //these coordinates are our 0,0 wrt the root window. There must be
  //a better way of obtaining them; see GxMenuBar.cc
  //this must have been placed and dipslayed
  void Activate(int xRoot, int yRoot); //this must be called with no buttons pressed
  void DeActivate(void);

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;
  virtual void PlaceChildren(void);

  // ************* start GxMenuItemOwner overloads *******************
  virtual bool GetButtonHeldMode(void) const;
  virtual void SetButtonHeldMode(bool newMode);
  virtual void EndMenuEventGrab(void);
  virtual GxRealOwner & GetRealOwnerObject(void);
  // *********** end GxMenuItemOwner overloads *****************
  //stuff for GxSubMapHolder
  GxMapHolder* GetClosestMapHolder(void);
  void UnManageWindow(Window winID);

  virtual void HandleEvent(const XEvent &rEvent);
  virtual void HandleGrabbedEvents(const XEvent &rEvent);
protected:
  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);

  bool buttonHeldMode;
  //if non-null we are in the 'internal' event loop where we filter all events going into the app via
  //an event handler registered with our display
  GxEventHandlerID eventHandlerID;
  virtual void StartMenuEventGrab(void);
  virtual void Draw(void);
};

#endif //GXPOPUPMENU_INCLUDED
