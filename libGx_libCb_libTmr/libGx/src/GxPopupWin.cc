#include <libGx/GxMainInterface.hh>
#include <libGx/GxDisplay.hh>

#include <libGx/GxPopupWin.hh>

GxPopupWin::GxPopupWin(GxTopLevelWin *pOwner) :
  GxTopLevelWin(pOwner), eventHandlerID(NULL_EVENT_HANDLER_ID),
  processEvents(true), mapHolder(pOwner->GetClosestMapHolder() )
{}

GxPopupWin::~GxPopupWin(void)
{
  //hack? should we do this or just call EndNonblockingDialog
  if(eventHandlerID != NULL_EVENT_HANDLER_ID)
    dInfo.rGxDisplay.RemoveEventHandler(eventHandlerID);
}

void GxPopupWin::Create(void)
{
  GxTopLevelWin::Create();
  XSetTransientForHint(dInfo.display, xWin, pWinAreaOwner->GetClosestXWin());
}

void GxPopupWin::Display(void)
{
  int pX, pY;
  ((GxTopLevelWin*)pWinAreaOwner)->GetPosition(pX, pY);
  UINT pWidth, pHeight;
  ((GxTopLevelWin*)pWinAreaOwner)->GetSize(pWidth, pHeight);

  int nX = pX + (((int)pWidth - (int)width)/2);
  int nY = pY + (((int)pHeight - (int)height)/2);
  Move(nX, nY);

  GxTopLevelWin::Display();
}

GxMapHolder* GxPopupWin::GetClosestMapHolder(void)
{
  return &mapHolder;
}

void GxPopupWin::UnManageWindow(Window winID)
{
  mapHolder.UnManageWin(winID);
}

void GxPopupWin::StartNonblockingDialog(void)
{
  dInfo.rGxDisplay.PushEventHandler( CbOneMember<const XEvent&, GxPopupWin>
				      (this, &GxPopupWin::HandleExternalEvent),
				      eventHandlerID);
}

void GxPopupWin::EndNonblockingDialog(void)
{
  dInfo.rGxDisplay.RemoveEventHandler(eventHandlerID);
  eventHandlerID = NULL_EVENT_HANDLER_ID;
  nbDialogDone();
}

void GxPopupWin::EventLoop(void)
{
  StartNonblockingDialog();

  dInfo.rMainInterface.FlushDisplays();

  while(processEvents)
    dInfo.rMainInterface.EventStep();

  EndNonblockingDialog();
  processEvents = true;
}

void GxPopupWin::HandleExternalEvent(const XEvent &rEvent)
{
  if( mapHolder.SendXEventLocally(rEvent) ) return;

  /*
    Because we are a popup we must eat certain events destined to
    other application windows like ButtonPress and ButtonReleasees,
    but some must make it to other parts of the application; like
    expose events. this is what limits a users interaction with other
    parts of an application while a popup window is displayed on the
    screen
  */

  switch(rEvent.type)
    {
    case Expose:
      mapHolder.SendXEventUp(rEvent);
      break;
    case ConfigureNotify:
      /*
	the win manager will resize windows regardless of where
	we are in the event loop. we must catch these resizes
	nand make sure the width and height are updated in the
	GxWinArea class (as well as repostioning everything)
      */
      mapHolder.SendXEventUp(rEvent);
      break;	      
    case SelectionRequest:
      //these are needed to allow an application to respond to others for
      //data transfer, etc. while in a popup.
      mapHolder.SendXEventUp(rEvent);
      break;
      //FocusIn FocusOut //we should not send these up
    default:
      break;//eat the event
    };
}

GxMapHolder * GxPopupWin::GetMapHolder(void)
{
  return &mapHolder;
}
