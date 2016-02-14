#ifndef GXFOCUSBUTTONBASE_INCLUDED
#define GXFOCUSBUTTONBASE_INCLUDED

#include <X11/keysym.h>
#include <libCb/CbCallback.hh>

#include <libGx/GxWin.hh>


class GxFocusButtonBase : public GxWin
{
protected:
  GxFocusButtonBase(GxRealOwner *pOwner);
public:
  virtual ~GxFocusButtonBase(void);

  //this compares the new active state to the previous active state and does notthing
  //if no change. this makes this an inexpensive function if no change.
  void SetActive(bool nActive);

  virtual bool AcceptFocus(Time eventTime);

  //this should not have to be overloaded, if it does, perhaps inheriting
  //from this class is the wrong choice
  void HandleEvent(const XEvent &rEvent);
protected:
  void HandleGrabbedEvents(const XEvent &rEvent);

  //the button will have to redraw itself(if it wants) from this
  //BUT BEFORE MAKING ANY CALLBACK (IMAGINE A QUIT BUTTON)
  virtual void DoAction(void) = 0;
  virtual void DrawButton(void) = 0;

  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);

  bool haveFocus;

  //so you can de-activeate a button and it will no longer respond to
  //user input.
  //button should draw itself differently if this is false so the user
  //knows it cannot be used
  bool active;
  //the pointer is over the button and the user has pushed the button
  bool pressed;
  bool inside; //if the mouse pointer is over the button

private:
  GxEventHandlerID pushedHandlerID;
};

#endif //GXFOCUSBUTTONBASE_INCLUDED
