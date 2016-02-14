#ifndef GXNOFOCUSBUTTONBASE_INCLUDED
#define GXNOFOCUSBUTTONBASE_INCLUDED

#include <libGx/GxOwnerWin.hh>
#include <libCb/CbCallback.hh>

//hack, we really don't need to inhereit from GxOwnerWin
//(GxWin would be better) but tooltips need to belong to an owner.
class GxNoFocusButtonBase : public GxOwnerWin
{
protected:
  GxNoFocusButtonBase(GxRealOwner *pOwner);
public:
  virtual ~GxNoFocusButtonBase(void);

  void SetActive(bool nActive);

  //this should not have to be overloaded, if it does, perhaps inheriting
  //from this class is the wrong choice
  void HandleEvent(const XEvent &rEvent);

protected:
  void HandleGrabbedEvents(const XEvent &rEvent);

  //the button will have to redraw itself(if it wants) from this
  //BUT BEFORE MAKING ANY CALLBACK (IMAGINE A QUIT BUTTON)
  virtual void DoAction(void) = 0;
  virtual void DrawButton(void) = 0;

  virtual void PointerIn(void);
  virtual void PointerOut(void);

  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);

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

#endif //GXNOFOCUSBUTTONBASE_INCLUDED
