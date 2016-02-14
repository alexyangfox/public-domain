#ifndef GXTABMANAGER_INCLUDED
#define GXTABMANAGER_INCLUDED

#include <libGx/GxNoFocusButtonBase.hh>

// class GxTabButton : public GxNoFocusButtonBase
// {
// public:
//   GxTabButton(GxRealOwner *pOwner);
//   virtual ~GxTabButton(void);

// protected:
//   virtual void DoAction(void);
//   virtual void DrawButton(void);
// };

class GxTabManager : public GxOwnerWin
{
public:
  GxTabManager(GxRealOwner *pOwner);
  virtual ~GxTabManager(void);

  virtual void SetActiveNum(unsigned newActiveNum);

  virtual void PlaceChildren(void); //we place all our children as if they were the only one.
  //virtual void CreateChildren(void); //we create all our children
  virtual void DisplayChildren(void); //we only display the active pane


  void HandleEvent(const XEvent &rEvent);
  CbOneFO<unsigned> tabChangeCB; //is called with the newly active id.

protected:
  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);
  unsigned activeNum; //numbers start at 1 -> 0 is an invalid id.
  void DrawInternal(void);
  void DrawBottomTab(unsigned tlX, unsigned tlY, unsigned width, unsigned height, bool active);
  unsigned SelTab(int xPix, int yPix) const;
};

#endif //GXTABMANAGER_INCLUDED
