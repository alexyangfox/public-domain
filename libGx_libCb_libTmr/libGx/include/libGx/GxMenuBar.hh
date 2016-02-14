#ifndef GXMENUBAR_INCLUDED
#define GXMENUBAR_INCLUDED

#include <list>

#include <libCb/CbCallback.hh>

#include <libGx/GxOwnerWin.hh>
#include <libGx/GxSubMapHolder.hh>
#include <libGx/GxGeomControl.hh>

class GxMenu;

class GxMenuBar : public GxCoreOwnerWin, public GxSubMapHolder
{
public:
  GxMenuBar(GxRealOwner *pOwner);
  //this class could be overloaded to add a vendor specific pixmap
  //as decoration if desired. Just overload DrawBar
  virtual ~GxMenuBar(void);

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  //overloaded to start placing children beyond the border
  virtual void PlaceChildren(void);

  GxMapHolder* GetClosestMapHolder(void);
  void UnManageWindow(Window winID);

  //we want Expose and BPressEvents & BReleaseEvents for server lock.
  void GetWindowData(XSetWindowAttributes &winAttributes, ULINT &valueMask);

  void HandleEvent(const XEvent &rEvent);

  //in lue of an internal event loop, this pushes a callback onto the display that delivers
  //all Display events here for this class to filter.
  void GrabbedDisplayHandleEvent(const XEvent &rEvent);

  void MenuButtonPress(GxMenu *pMenu);
  void MenuButtonRelease(void);
  bool SelectMenuBecauseEnter(GxMenu *pMenu);

  void EndMenuEventGrab(void);
  //returns true for ButtonHeld false otherwise
  bool GetMenuMode(void);
  void SetMenuMode(bool newMode);
  //so the menu's can know if we are in the event loop
  bool InMenuLoop(void);

  //hackish. this adds functions to call when we finish processing an event in MenuEventLoop.
  //functions are called in the order they are added. this cannot tolerate a null pointer
  void AddPostEventCB(CbVoidBase *pVoidBaseCB);
protected:
  virtual void DrawBar(void);
  std::list<CbVoidBase*> postCBList;

private:

  bool buttonHeld;
  GxMenu *pActiveMenu;
  //if non-null we are in the 'internal' event loop where we filter all events going into the app via
  //an event handler registered with our display
  GxEventHandlerID eventHandlerID;
};

#endif //GXMENUBAR_INCLUDED
