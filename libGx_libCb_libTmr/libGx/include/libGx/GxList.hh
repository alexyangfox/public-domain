#ifndef GXLIST_INCLUDED
#define GXLIST_INCLUDED

#include <libGx/GxWin.hh>
#include <libGx/GxScrolledWin.hh>

/* 
   hack. this is a collection pretty bad code flying in formation.
   must be cleaned up or replaced by GxNewList.
*/

class GxListItem;

class GxListData
{
public:
  GxListData(GxVolatileData &tVData, GxDisplayInfo &tDInfo);
  virtual ~GxListData(void);

  GxVolatileData &vData;
  GxDisplayInfo &dInfo;

  //note: this value is not initally valid. It is valid after
  // the list is Created
  Window win;
};

//the GxList spaces all list items one pixel apart and one pixel at top, at
//bottom at left and at right. LookupListItem, SizeAll, VScrollCallback
//the user should treat this as an opaque class and not try to manipulate
//the GxAppScrolledWin below. ?private inheritance? - but we need the interface
class GxList : public GxAppScrolledWin
{
public:
  GxList(GxRealOwner *pOwner);
  virtual ~GxList(void);

  //NOTE:
  //when we add a list item we don't automaticly redraw the list or adjust the
  //scrollbars or even clear the window because if we are adding a lot of
  //stuff to the list we don't want to waste our time resizing and redrawing
  //the list. Leave that to the application code writer. It keeps things
  //fast and simple.

  //adds the new item before or after all other items
  void AddListItemStart(GxListItem *pNewItem);
  void AddListItemEnd(GxListItem *pNewItem);
  //THE LIST DOES NOT CHECK THE VALIDITY OF pRefItem IT IS ASSUMED TO BE
  //NON-NULL AND TO BE A POINTER TO A LIST ITEM PREVIOUSLY ADDED TO THE LIST.
  void AddListItemAfter(GxListItem *pRefItem, GxListItem *pNewItem);
  void AddListItemBefore(GxListItem *pRefItem, GxListItem *pNewItem);

  //*NOTE* THIS MUST BE SET - IF YOUR LIST DOSEN'T SHOW ANYTHING - YOU FORGOT
  //Sets the top item. calling without an argument sets the first visable
  //item to be the first one in my list of items.
  //hack? should this be automated?
  void SetTopItem(GxListItem *pNewTop = NULL);
  //does not check validity of pSelItem. pSelItem may be null (clears selected item). Does not redraw list.
  void SetSelectedItem(GxListItem *pSelItem);

  //not only removes all items on the list, but it also calls
  //delete on all of the list item pointers. This is called in the destructor (with inhibitRedraw = true).
  //does not redraw or resize anything of course
  void Clear(void);
  //hack; write a demo program for corect list usage (show tricks)
  //this is pretty ugly, but it works remarkably well.
  //pass in a pointer to a list item which is in my
  //list, I will remove it, but not delete it.
  //(that is the resposiblity of the user)
  void RemoveListItem(GxListItem *pItem);

  GxListItem * GetFirstListItem(void);

  //a function for use by the list items
  GxListData& GetListData(void);

  //note: the xwindow in a GxListData is invalid untill Create is called.
  virtual void Create(void);

  //calls SizeAll so we can size the scrollbars
  virtual void PlaceChildren(void);
  //called by button 
  void SelectPress(int yClick);
  void SelectRelease(int yClick);
  //this could be outside the clipwin; have to check; and if so scroll the
  //list slowly selecting as we go.
  void SelectMotion(int yClick);
  //clears the listWindow
  void ClearWindow(void);
  //called by expose events in GxListWin
  void DrawList(void);
  //recalculates maxMinWidth and listHeight and sizes and positions the 
  //sliders within the scrollbars also calls size on all of the list items
  virtual void SizeAll(void);
  virtual void LocateSliders(void);

  //this is more for your debuging than actual use; very slow
  UINT GetNumItems(void);

protected:

  void HScrollCallback(GxFraction nFr);
  void ScrollLeftCB(void);
  void ScrollRightCB(void);
  void VScrollCallback(GxFraction nFr);
  void ScrollUpCB(void);
  void ScrollDownCB(void);

  GxListItem* LookupListItem(int yClick);

  GxListData listData;
  GxListItem *pSelectedItem;
  GxListItem *pFirstItem; //the first item in my list of items.
  GxListItem *pCurrentTop; //the first item currently displayed

  int listX; //where the list is drawn from
  UINT maxMinWidth; //the widest width of my list items; used in scrollbars
  UINT listHeight; //the combined height of my listItems; used in scrollbars

  class ListPane : public GxWin
  {
  public:
    ListPane(GxList *pOwner);
    virtual ~ListPane(void);
    //clears the window minus the border
    virtual void Clear(void);
    void HandleEvent(const XEvent &rEvent);
  protected:
    void HandleGrabbedEvents(const XEvent &rEvent);
    GxEventHandlerID pushedHandlerID;

    void GetWindowData(XSetWindowAttributes &winAttributes, ULINT &valueMask);
  };

  //passed to GxAppScrolledWin as our clip window accessed via pClipWin after
  //we call SetClipWin
  ListPane cWin;
};

class GxListItem
{
  /*
    friend is used to let the GxList manage a list of GxListItems
    without the user or the list having to fool with GxListItem
    containers (Also eliminates the expense of managing
    creating/deleting the containers.)
  */
  friend class GxList;

public:
  GxListItem(void);
  virtual ~GxListItem(void);

  //gives the list item an opertunity to set itemWidth and itemHeight
  //Basically just before the
  //list sizes its scrollbars, Size is called on each of the GxListItems
  //this should set itemWidth and itemHeight to the minimum needed for the
  //contents of the listItem.  After size has been called the maximum width
  //of all of the widest of the listItems is determined, and SetWidth is called
  //on all of the list items with this new width.
  virtual void Size(GxListData &rData);
  //called when the user clicks in the list item
  virtual void SelectCallback(void);
  //the x and y are my upper left corner
  virtual void Draw(int x, int y, const GxListData &rData);
  virtual void SetWidth(UINT newWidth);

  //these pull information from the list item; called by the list
  virtual UINT GetWidth(void);
  virtual UINT GetHeight(void);

  GxListItem * GetPrevItem(void);
  GxListItem * GetNextItem(void);

protected:
  UINT itemWidth;
  UINT itemHeight;

private:
  GxListItem *pPrevItem;
  GxListItem *pNextItem;
};

//a basic list item which displays a text label for each list item and calls
//a GxVoidCallback when selected
const unsigned GX_TEXT_LIST_ITEM_LABEL_LEN = 512;
class GxTextListItem : public GxListItem
{
public:
  GxTextListItem(void);
  virtual ~GxTextListItem(void);

  void SetLabel(const char *pLabel);
  virtual void Size(GxListData &rData);

  virtual void SelectCallback(void);

  virtual void Draw(int x, int y, const GxListData &rData);

  CbVoidFO cb;

protected:
  char label[GX_TEXT_LIST_ITEM_LABEL_LEN];
  unsigned labelLen;
};

#endif //GXLIST_INCLUDED
