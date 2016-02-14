#ifndef GXPULLDOWNMENU_INCLUDED
#define GXPULLDOWNMENU_INCLUDED

#include <libGx/GxInc.hh>
#include <libGx/GxOwnerWin.hh>
#include <libGx/GxArrowButton.hh>
#include <libGx/GxRootTransient.hh>
#include <libGx/GxVLine.hh>
#include <libGx/GxSubMapHolder.hh>
#include <libGx/GxGeomControl.hh>
//psudo hack
#include <libGx/GxList.hh>

// ?should this be pulled into GxPulldownMenu definition?
class GxPulldownItem
{
public:
  GxPulldownItem(const char *pLabel = NULL);
  virtual ~GxPulldownItem(void);

  void SetLabel(const char *pName);
  void Draw(int x, int y, UINT width, UINT height, GxListData &rLData, bool active = true) const;
  UINT GetDesiredHeight(GxListData &rLData) const;
  UINT GetDesiredWidth(GxListData &rLData) const;

  //just calls cb
  void Select(void) const;
  
  CbVoidFO cb;

protected:
  char Label[GX_DEFAULT_LABEL_LEN];
};

/*
  How this class works: User clicks arrow button in the
  GxPulldownMenu.  GxPulldownMenu::DoPulldown is called which maps the
  GxPulldownPane.  GxPulldownPane::StartEventGrab is called which
  starts redirecting all app events to GrabbedDispalyHandleEvent(). In
  this function all events are processed for children of the
  GxPulldownPane.  Expose Events are sent up, however (also focus for
  the time being). Any button events that occur that do not belong to
  the GxPulldownPane or any of its children end the event loop
*/

class GxPulldownMenu;

//?should this be pulled into GxPulldownMenu definition?
//this should inherit from GxList.
class GxPulldownPane : public GxRootTransient
{
public:
  GxPulldownPane(GxRealOwner *pOwner, GxPulldownMenu *pTMenu,
		 GxListData &rLData);
  virtual ~GxPulldownPane(void);
  
  virtual void StartEventGrab(Window clipWin);
  virtual void GrabbedDisplayHandleEvent(const XEvent &rEvent);
  virtual void EndEventGrab(void);

  virtual UINT GetWidestItemWidth(void) const; //needed so the GxPulldownMenu can size its label
  virtual void GetDesiredSize(UINT &rDesiredWidth, UINT &rDesiredHeight) const;

  virtual void Create(void);
  virtual void HandleEvent(const XEvent &rEvent);
  
  virtual GxMapHolder* GetClosestMapHolder(void);

  //I own the list data, but it is managed by the GxPulldownMenu which owns me
  std::list<GxPulldownItem*> itemList;
  const GxPulldownItem* pCurrentItem;
  
protected:
  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);
  
  virtual GxMapHolder * GetMapHolder(void);
  GxSubMapHolder winHolder;
    
  void DrawInterior(void);
    
  void HandleButtonPress(int yClick);
  const GxPulldownItem * SelectItem(int yVal);
  void ButtonPressGrabbedHandleEvent(const XEvent &rEvent);
  const GxPulldownItem *pFItem; //first item selected during button press

  GxPulldownMenu *pMenu;
  GxListData &listData;
  GxEventHandlerID eventHandlerID;
  GxEventHandlerID bPressHandlerID;
};


//hack; need to add scrollbar support; ?could I add a list?
//would be nice, if more expensive than building a custom one.
class GxPulldownMenu : public GxOwnerWin
{
public:
  GxPulldownMenu(GxRealOwner *pOwner);
  virtual ~GxPulldownMenu(void);

  void SetActive(bool nActive);

  //Added items are in many ways considered to be const pointers, but we do provide the interface to
  //delete all list items when we clear the item list, so we cannot take const pointers here.
  void AddItem(GxPulldownItem *pNewItem);
  //I don't know which of the following two interfaces are more usefull.
  //both require list traversals, but these lists are rather short.
  //this looks for a pointer identical to pItemToRemove then removes it from the list.
  void RemoveItem(const GxPulldownItem *pItemToRemove);
  //numbers start from zero
  void RemoveItem(UINT numToRemove);
  void SetNoneSelectedText(const char *pText);
  //if set to zero, will internally calculate. this is in pixels.
  //should probably be an num of avg character width
  void SetDesiredWidth(unsigned value);

  virtual void Create(void);

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  //num to set current, start from one. zero (or an invalid number) clears the selected item
  void SetCurrentItem(UINT numToSetCurrent);
  UINT GetCurrentItem(void) const;
  //clears the current selection
  void ClearCurrentItem(void);

  //totally removes all list items from the menu. it does _not_delete the pointers.
  //if the user wants this to happen, he/she must get the list from GetItemList and delete them
  void ClearItems(void);
  //removes all list items from the menu and deletes the pointers. be careful. many Pulldown menus
  //are constructed from GxPulldownItems that are allocated in big chunks (i.e. are member objects)
  //this is only useful for pulldown menu's whose contents are constructed dynamically.
  void DeleteItems(void);

  //called by my pdPane when current object is changed
  void DrawLabel(bool clear = false);

  void HandleEvent(const XEvent &rEvent);

protected:
  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);
  void DoPulldown(void);

  //this serves double duty, its x window is used for x reporting events
  //outside the application during the pointer grab. It is also used for
  //cliping the normal drawing stuff
  //hackish; it is not really critical to have this anymore
  class ClipWindow : public GxOwnerWin
  {
  public:
    ClipWindow(GxPulldownMenu *pOwner);
    virtual ~ClipWindow(void);

    void HandleEvent(const XEvent &rEvent);
  protected:
    virtual void GetWindowData(XSetWindowAttributes &winAttributes, ULINT &valueMask);
  };

  char label[GX_DEFAULT_LABEL_LEN];

  //a convience decleration (we need a GxListData often)
  GxListData listData;

  //interface widgets
  GxArrowButton AB;
  GxVLine vLine;
  ClipWindow clipWindow;
  GxPulldownPane pdPane;

  unsigned desWidth;
  bool active;
};


#endif //GXPULLDOWNMENU_INCLUDED
