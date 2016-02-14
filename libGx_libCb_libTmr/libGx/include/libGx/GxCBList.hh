#ifndef GXCBLIST_INCLUDED
#define GXCCBLIST_INCLUDED

#include <list>

#include <libCb/CbCallback.hh>

//in many places after we deliver a callback, we need other callbacks to be made
//this handles a list of those callbacks.  These callbacks can be removed from
//the list before the callback list is called. This is important if an object
//to which an event is to be delivered is deleted by the callback that was delivered

//this functionality should probably be moved into libCb

typedef CbVoidBase * GX_CB_ID;
const GX_CB_ID GX_CB_ID_INVALID = 0;

class GxCBList
{
public:
  GxCBList(void);
  virtual ~GxCBList(void);

  void CallCBList(void); //call all callbacks in order

  //we call Clone() on the rBaseCB to allocate an object that we own internally.
  //we return this pointer as a 'handle' to for external objetcts so that they can
  //request that we remove and delete this callback object. we return the ID as
  //a pointer because it is a cheap way to get a unique ID. the calling object
  //should not try to use the ID as a pointer.
  GX_CB_ID AddCB(const CbVoidBase &rBaseCB);
  //when we remove the ID, we delete the object. the owner then has an invalid
  //id (that is actually a pointer). the owner should set the id to GX_CB_ID_INVALID
  void RemoveCB(GX_CB_ID idToRemove);

protected:
    std::list<CbVoidBase*> cbList;
};

#endif //GXCALLBACKCBLIST_INCLUDED
