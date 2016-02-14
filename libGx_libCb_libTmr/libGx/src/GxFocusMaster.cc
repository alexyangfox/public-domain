#include <libGx/GxFocusMaster.hh>

GxFocusMaster::GxFocusMaster(void) :
  haveFocus(false), pCFocusObject(0)
{}

GxFocusMaster::~GxFocusMaster(void)
{}

void GxFocusMaster::AddFocusObject(GxWinArea *pObject)
{
  if(!pObject) return;

  focusList.push_back(pObject);
}

void GxFocusMaster::RemoveFocusObject(GxWinArea *pWinArea, Time eventTime)
{
  if(!pWinArea) return;

  std::list<GxWinArea*>::iterator cPlace = focusList.begin();
  std::list<GxWinArea*>::iterator cEnd = focusList.end();
  while(cPlace != cEnd)
    {
      if(*cPlace == pWinArea)
	{
	  cPlace = focusList.erase(cPlace);
	  //give focus to the next place in the list.
	  //?hack. do we want to do this? -> nope I think not
	  /*
	  if(cPlace == cEnd)
	    {
	      if(focusList.empty() )
		{
		  //?do what?
		}else
		  {
		    (*(focusList.front()))->AcceptFocus(eventTime);
		  }
	    }else
	      (*cPlace)->AcceptFocus(eventTime);
	  */
	  if(pCFocusObject == *cPlace)
	    {
	      pCFocusObject = 0;
	      haveFocus = false;
	    };
	  return;
	};
      cPlace++;
    };
}

void GxFocusMaster::MoveFocusToObject(GxWinArea *pWinArea, Time eventTime)
{
  if(!pWinArea) return;

#ifdef LIBGX_DEBUG_BUILD
  std::cout << "GxFocusMaster::MoveFocusToObject unimplemented" << std::endl;
#endif //LIBGX_DEBUG_BUILD
}

void GxFocusMaster::RegainFocus(Time eventTime)
{
#ifdef LIBGX_DEBUG_BUILD
  std::cout << "GxFocusMaster::RegainFocus" << std::endl;
#endif //LIBGX_DEBUG_BUILD

  haveFocus = false; //should do nothing
  if( focusList.empty() )
    {
      pCFocusObject = 0; //should be null already
      return;
    }; //nothing to do

  if(pCFocusObject)
    {
      //hackish. we are verifying that pCFocusObject is in the list
      //before transfering control to it.
      std::list<GxWinArea*>::iterator cPlace = focusList.begin();
      std::list<GxWinArea*>::iterator cEnd = focusList.end();
      while(cPlace != cEnd)
	{
	  if(*cPlace == pCFocusObject)
	    {
	      pCFocusObject->AcceptFocus(eventTime);
	      haveFocus = true;
	      return;
	    };

	  cPlace++;
	};
    };

  //it should not have been set.
  //we will reset it now
  //pCFocusObject = 0;
  //haveFocus = false;

  //set it to be correct.
  pCFocusObject = focusList.front(); //we know this is valid from check above
  haveFocus = true;
  pCFocusObject->AcceptFocus(eventTime);
  return;
}

void GxFocusMaster::TransferFocus(Time eventTime)
{
  if( focusList.empty() )
    {
      pCFocusObject = 0; //should be null already
      return;
    }; //nothing to do

  /* we regularly lose focus. that does not mean we should reset pCFocusObject
  if(!haveFocus)
    {
      //pCFocusObject should be null.
      pCFocusObject = focusList.front();
      haveFocus = true;
      pCFocusObject->AcceptFocus(eventTime);
      return;
    };
  */

  //if we're here we will be moving the focus unless there is only one element
  //in the focus list and that element already has the focus. for this latter
  //case we don't want to AcceptFocus() unless we have to
  std::list<GxWinArea*>::iterator cPlace = focusList.begin();
  std::list<GxWinArea*>::iterator cEnd = focusList.end();
  while(cPlace != cEnd)
    {
      if(*cPlace == pCFocusObject)
	{
	  if( (++cPlace) == cEnd )
	    {
	      (focusList.front())->AcceptFocus(eventTime);
	      pCFocusObject = focusList.front();
	    }else
	      {
		(*cPlace)->AcceptFocus(eventTime); //remember we incremented it above
		pCFocusObject = *cPlace;
	      };
	  return;
	};
      cPlace++;
    };
}
