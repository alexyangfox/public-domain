#ifndef GXLISTTREE_INCLUDED
#define GXLISTTREE_INCLUDED

#include <libGx/GxScrolledWin.hh>

class GxListTree : public GxAppScrolledWin
{
public:
  GxListTree(GxRealOwner *pOwner);
  virtual ~GxListTree(void);

  void DrawList(void) const;
  void SelectPress(int yClick);
  void SelectRelease(int yClick);
  void SelectMotion(int yMotion);

protected:

  class ListPane : public GxWin //forms the clip window
  {
  public:
    ListPane(GxListTree *pOwner);
    virtual ~ListPane(void);
    //clears the window minus the border
    virtual void Clear(void);
    void HandleEvent(const XEvent &rEvent);
  protected:
    void HandleGrabbedEvents(const XEvent &rEvent);
    GxEventHandlerID pushedHandlerID;

    void GetWindowData(XSetWindowAttributes &winAttributes, ULINT &valueMask);
  };
  
  ListPane listPane;
};

#endif //GXLISTTREE_INCLUDED
