#ifndef GXPOPUPWIN_INCLUDED
#define GXPOPUPWIN_INCLUDED

#include <libGx/GxInc.hh>
#include <libGx/GxDisplay.hh>
#include <libGx/GxSubMapHolder.hh>
#include <libGx/GxTopLevelWin.hh>

class GxPopupWin : public GxTopLevelWin
{
public:
  GxPopupWin(GxTopLevelWin *pOwner);
  virtual ~GxPopupWin(void);

  //we overload this again to set transient hints. This calls
  //GxTopLevelWin::Create so GetWMProperties is _still_ called (as it should)
  virtual void Create(void);

  //this is overloaded to set my x and y so my window is centered relative
  //to my parent.  the window manager just uses this as a hint.
  //this is done now because this should be called after we have been created.
  virtual void Display(void);

  //overloads of GxCoreOwnerWin
  GxMapHolder* GetClosestMapHolder(void);
  void UnManageWindow(Window winID);

  void StartNonblockingDialog(void);//pushes an event handler onto the display's list
  void EndNonblockingDialog(void); //pops an event handler
  //called from EndNonblockingDialog
  CbVoidFO nbDialogDone;

  //does a blocking event loop untill processEvents is set true
  void EventLoop(void);

protected:
  //handle an event which may or may not be destined to this window.
  virtual void HandleExternalEvent(const XEvent &rEvent);

  //so we will be in our own mapHolder
  virtual GxMapHolder * GetMapHolder(void);

  //if eventHandlerID is not NULL_EVENT_HANDLER_ID  we are doing a
  //non-blocking dialog
  GxEventHandlerID eventHandlerID;
  bool processEvents;
  GxSubMapHolder mapHolder;
};

#endif //GXPOPUPWIN_INCLUDED

