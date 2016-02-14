#ifndef GXTOOLBARSIMPLEDOCK_INCLUDED
#define GXTOOLBARSIMPLEDOCK_INCLUDED

#include <list>

#include <libGx/GxInc.hh>
#include <libGx/GxOwnerWin.hh>
#include <libGx/GxToolBar.hh>

//a toolbar dock which does not require a toolbar manager.  toolbars cannot be removed or modified.
//a bar which organizes its buttons in a row
//some buttons groups (organized by the programmer) only lets one
//remain depressed at a time; this functionality has not been added
//as of yet. hack. add button groups.

class GxToolBarButtonWin;

class GxToolBarSimpleDock : public GxOwnerWin
{
public:
  GxToolBarSimpleDock(GxRealOwner *pOwner);
  virtual ~GxToolBarSimpleDock(void);

  void SetToolBar(GxToolBar* pCBar);

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  void SetVertical(bool nVert);

  virtual void PlaceChildren(void);
  //virtual void Create(void);

  void HandleEvent(const XEvent &rEvent);

protected:
  void GetWindowData(XSetWindowAttributes &winAttributes, ULINT &valueMask);
  void ClearButtonWinList(void);

  GxToolBar* pToolBar;
  std::list<GxToolBarButtonWin*> buttonWinList;
  //if true draw things vertically
  bool vertical;
};


#endif //GXTOOLBARSIMPLEDOCK_INCLUDED
