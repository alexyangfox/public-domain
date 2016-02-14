#ifndef GXFOCUSMASTER_INCLUDED
#define GXFOCUSMASTER_INCLUDED

#include <list>

#include <libGx/GxInc.hh>
#include <libGx/GxWinArea.hh>

class GxFocusMaster
{
public:
  GxFocusMaster(void);
  virtual ~GxFocusMaster(void);

  //both of these are the responsibility of the user
  virtual void AddFocusObject(GxWinArea *pObject);
  //really of the user ?wtf did I mean?
  //should this be removed?
  virtual void RemoveFocusObject(GxWinArea *pWinArea, Time eventTime);

protected:
  //only updates focus if pChild is really in our list.
  virtual void MoveFocusToObject(GxWinArea *pWinArea, Time eventTime);

  void RegainFocus(Time eventTime); //try to give focus to pCFocusObject
  void TransferFocus(Time eventTime); //moves focus to next in list
  std::list<GxWinArea*> focusList; //hackish. should be slist.
  bool haveFocus; //set if an object in the list actually has the focus
  //the object we think should have the focus. and the object we will try to give the focus when we
  //regain the focus if we have lost it
  GxWinArea* pCFocusObject;
};

#endif //GXFOCUSMASTER_INCLUDED
