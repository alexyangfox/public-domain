#ifndef GXNEWLIST_INCLUDED
#define GXNEWLIST_INCLUDED

#include <libGx/GxWin.hh>
#include <libGx/GxScrolledWin.hh>

class GxNewListItem
{
public:
  GxNewListItem(void);
  virtual ~GxNewListItem(void);

  bool selected;
};

//in the future this will be client side with a skeleton
//list class too
class GxNewListContents
{
public:
  GxNewListContents(void);
  virtual ~GxNewListContents(void);

  void DrawList(void);
};

//in the future this will be server side
class GxNewList : public GxAppScrolledWin
{
public:
  GxNewList(GxRealOwner *pOwner);
  virtual ~GxNewList(void);

  void SelectMultiple(bool newSelectMultipleStat);

  void DrawList(void);
  void SelectPress(int yPlace);
  void SelectMotion(int yPlace);
  void SelectRelease(int yPlace);

protected:
  class ListWin : public GxWin
  {
  public:
    ListWin(GxNewList *pOwner);
    virtual ~ListWin(void);
    //clears the window minus the border
    virtual void Clear(void);
    void HandleEvent(const XEvent &rEvent);
  protected:
    void HandleGrabbedEvents(const XEvent &rEvent);
    GxEventHandlerID pushedHandlerID;

    void GetWindowData(XSetWindowAttributes &winAttributes, ULINT &valueMask);
  };

  bool selectMultipleStat;
};

#endif //GXNEWLIST_INCLUDED
