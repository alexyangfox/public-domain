#ifndef GXTOPLEVELWIN_INCLUDED
#define GXTOPLEVELWIN_INCLUDED

#include <list>

#include <libGx/GxOwner.hh>
#include <libGx/GxDisplay.hh>
#include <libGx/GxCoreOwnerWin.hh>
#include <libGx/GxFocusMaster.hh>

class GxTopLevelWin : public GxFocusMaster, public GxCoreOwnerWin
{
public:
  virtual ~GxTopLevelWin(void);

  //these are inherited from GxFocusMaster
  //void AddFocusObject(GxWinArea *pObject);
  //void RemoveFocusObject(GxWinArea *pObject, Time eventTime);
  virtual void MoveFocusToChild(GxWinArea *pChild, Time eventTime);

  virtual void Place(void);
  //this is overloaded to call SetWMProperties
  virtual void Create(void);

  //can only be set after Create()
  virtual void SetTitle(const char *pTitle);

  //we need to request and handle Configure Events, also FocusEvents
  //also ClientMessage events (for WM_TAKE_FOCUS)
  virtual void HandleEvent(const XEvent &rEvent);

protected:
  GxTopLevelWin(GxOwner *pOwner);

  //only called if we add ourselves to the WM_DELETE_WINDOW protocol
  virtual void DeleteWindow(void);

  //returns RootWindow; overload of GxCoreWin function
  Window GetParentWindow(void);

  //does several things by default.
  // sets the win-name and the icon-name to be that of the application
  // sets WMHints.input to be true
  // adds all protocols we want to involve ourselves with in rWMProtolList,
  // registers ourself in the WM_TAKE_FOCUS protocol only by default
  virtual void GetWMProperties(XTextProperty &winName, XTextProperty &iconName,
			       XSizeHints &rSizeHints, XWMHints &rWMHints,
			       std::list<Atom> &rWMProtocolList);

  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);
};


#endif //GXTOPLEVELWIN_INCLUDED
