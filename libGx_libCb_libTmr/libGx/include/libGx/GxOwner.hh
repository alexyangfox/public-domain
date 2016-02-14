#ifndef GXOWNER_INCLUDED
#define GXOWNER_INCLUDED

#include <list>

#include <libGx/GxInc.hh>
#include <libGx/GxDisplayInfo.hh>
#include <libGx/GxVolatileData.hh>
#include <libGx/GxWinArea.hh>

class GxCoreWin;
class GxRealOwner;
class GxMapHolder;

class GxOwner
{
public:
  virtual ~GxOwner(void);

  //these virtual void functions are used in child constructors
  virtual Window GetClosestXWin(void) = 0;
  virtual GxDisplayInfo& GetDisplayInfo(void) = 0;
  virtual GxVolatileData& GetVolatileData(void) = 0;
  virtual GxMapHolder* GetClosestMapHolder(void) = 0;
  //this virtual function is called by child distructors
  //It would be far better if this could be a pure virtual function like the
  //above, but if this were to be, as an inherited object is distructed,
  //the class which contains the winmap is deleted which means this function
  //would be called when the v-table is empty.
  //This function is called by GxCoreWin children distructors which in turn
  //are called when they are deleted in this class's distructor
  virtual void UnManageWindow(Window winID);

  virtual void RemoveChild(GxWinArea *pChild);
  void AddChild(GxWinArea *pNewChild); //must add to end of list

  //a child can call this after the user selects it to make sure the owner
  //knows it( owners by default pass this message up the "window tree" untill
  //it reaches a GxTopLevelWin which will match up pChild with an object
  //in its FocusObject List so that when a tab occurs the focus is moved
  //to the correct next object in the GxTopLevelWin's focus list. I could
  //implement the whole Focus thing with a seperate focus manager class
  //and instead of this implement GetClosestFocusManager(). doing that would
  //mean extra memory (an extra * or refrence to a focus manager) for each
  //object that could accpet focus, but it would be faster because this
  //pChild message would not have to travel up the owner tree. would be
  //more overhead at constructor time. This would also expose functionality
  //to individual objects than they should have (like adding and removing focus
  //objects)
  virtual void MoveFocusToChild(GxWinArea *pChild, Time eventTime) = 0;

  //this would add the pChild to both the Child list and the owner child list.
  //benefits of this would be being able to map all of my children at once
  //via XMapSubWindows, rather than mapping each one one at a time with
  //XMapWindow.  The reason this is unimplemented is I don't want expend the
  //memory on a second list of owners and the overhead of managing two child
  //lists at one, or the overhead of RTTI if I maintain only one list.
  //void AddOwnerChild(GxRealOwner *pChild);
  //void RemoveOwnerChild(GxRealOwner *pChild);

protected:
  GxOwner(void);

  //objects in the child list MUST be retained in the same order as they
  //were added to allow sensible geometry management
  //I wish this could be a slist, but I need to add objects to the back.
  //this _could_ be done with a differently implemented slist
  std::list<GxWinArea*> childList;
};

#endif //GXOWNER_INCLUDED
