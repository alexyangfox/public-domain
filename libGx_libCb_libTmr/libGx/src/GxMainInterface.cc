#include <string.h>
#include <errno.h>
#include <assert.h>

#include <iostream>
#include <sstream>

#include <libGx/GxMainInterface.hh>
#include <libGx/GxDisplay.hh>
#include <libGx/GxArguments.hh>

#include "GxDefines.hh"

using namespace std;

GxMainInterface::GxMainInterface(const char *pAppName, int tNumDisplays) :
  numDisplays(tNumDisplays), processEvents(true)
{
  unsigned junkLength = 0; //unused
  GxSetLabel(appName, GX_APP_NAME_LENGTH, pAppName, junkLength);
}

GxMainInterface::~GxMainInterface(void)
{
  //this should not fail.
  if( pMainInterface == this )
    pMainInterface = 0;

  for(unsigned ii = 0; ii < dispVector.size(); ii++)
    {
      GxDisplay *pDisp = dispVector[ii];
      delete pDisp;
      dispVector[ii] = 0;
    };
}

bool GxMainInterface::Initialize(int argc, char** argv)
{
  if( strlen(appName) == 0 && argc > 1) //should not ever happen
    {
      //desperately try and get some sort of application name.
      unsigned junkLength = 0; //unused
      GxSetLabel(appName, GX_APP_NAME_LENGTH, argv[0], junkLength);
    };

  if(numDisplays == -1) //search for the --num-displays argument. it must be present
    {
      if( argc <= 1 )
	{
	  SinkError("Required parameter -num-displays and argument (int ranging from 1 to 10) was not specified on command line");
	  return false;
	};

      numDisplays = 0; //if non-zero we found it.
      for(unsigned ii = 1; ii < (unsigned)argc; ii++)
	{
	  if( string("-num-displays") == argv[ii] )
	    {
	      if( ii+1 == (unsigned)argc )
		{
		  ostringstream err;
		  err << "-num-displays tag found at argument: " << ii << " but is missing subsequent specifier";
		  SinkError( err.str() );
		  return false;
		};

	      char *pTail = 0;
	      numDisplays = strtoul(argv[ii+1], &pTail, 10);
	      if( pTail == argv[ii+1] )
		{
		  ostringstream err;
		  err << "Unable to parse num-displays specified at argument: " << ii+1;
		  SinkError( err.str() );
		  return false;
		};

	      if( numDisplays == 0 || numDisplays > 10 )
		{
		  ostringstream err;
		  err << "Height specified at argument: " << ii+1 << " is out of valid range of 1:10 inclusive.";
		  SinkError( err.str() );
		  numDisplays = 0;
		  return false;
		};
	    };
	};

      if( numDisplays == 0 )
	{
	  SinkError("Required parameter -num-displays and argument (int ranging from 1 to 10) was not specified on command line");
	  return false;
	};
    };
  
  if( numDisplays == 0 )
    {
      dispVector.resize(1);
      GxArguments args;
      if( !ParseDisplayArgs(-1, args, argc, argv) ) return false;
      dispVector[0] = new GxDisplay(*this, args);
      return true;
    };

  assert(numDisplays > 0);
  //also reached by having numDisplays == -1 and having a valid -num-displays argument be found
  dispVector.resize(numDisplays);
  for(unsigned ii = 0; ii < (unsigned)numDisplays; ii++)
    dispVector[ii] = 0;

  for(unsigned ii = 0; ii < (unsigned)numDisplays; ii++)
    {
      GxArguments args;
      if( !ParseDisplayArgs(ii, args, argc, argv) ) return false;

      dispVector[ii] = new GxDisplay(*this, args);
    };

  return true;
}

bool GxMainInterface::OpenAllocateAll(void)
{
  assert( !pMainInterface );
  pMainInterface = this;

  for(unsigned ii = 0; ii < dispVector.size(); ii++)
    {
      GxDisplay *pDisp = dispVector[ii];
      if( !pDisp->OpenAllocate() ) return false;
    };

  XSetErrorHandler( GxMainInterface::ErrorHandler );

  return true;
}

void GxMainInterface::EventStep(void)
{
  fd_set read_set;
  FD_ZERO(&read_set);

  int maxFD = -1;
  for(unsigned ii = 0; ii < dispVector.size(); ii++)
    {
      int xFD = ConnectionNumber( dispVector[ii]->XDisp() );
      if( xFD > maxFD) maxFD = xFD;
      FD_SET( xFD, &read_set); 
    };
  
  //only set the timeout if we have timers to expire
  struct timeval *pTime = 0;
  struct timeval timeout;
  if( tMaster.GetNextTimerTime(timeout) )
    pTime = &timeout;
  
  int selRes = select(maxFD+1,  &read_set,  0, 0, pTime);
  if(selRes == -1)
    if(errno != EINTR) //EINTR is not necessarly a problem
      {//hackish. should try to save application
	cerr << "select error in GxMainInterface::EventLoop. fatal error." << endl;
      };
  
  for(unsigned ii = 0; ii < dispVector.size(); ii++)
    {
      Display *pXDisp = dispVector[ii]->XDisp();
      if( FD_ISSET( ConnectionNumber(pXDisp), &read_set ) )
	{
	  while( XEventsQueued(pXDisp, QueuedAfterFlush) )//we had an event
	    {
	      XEvent event;
	      XNextEvent(pXDisp, &event);
	      dispVector[ii]->HandleEvent(event); //here is where processEvents might get set false
	    };
	}
    };
  
  //even if we did not have a timeout in select, we will check for one here that
  //could have expired while we were processing the event
  //if(selRes == 0) //we had a timeout
  tMaster.CheckAndExecExpired();
  
  //we can easily have commands waiting to be sent to the server unless we flush each
  //display connection. if we don't all the events left in the send que will apear
  //never to have been made (the server won't see them).
  FlushDisplays();
}


void GxMainInterface::EventLoop(void)
{
  processEvents = true;

  FlushDisplays();

  while(processEvents)
    EventStep();
}

void GxMainInterface::EndEventLoop(void)
{
  processEvents = false;
}

void GxMainInterface::FlushDisplays(void)
{
  for(unsigned ii = 0; ii < dispVector.size(); ii++)
    XFlush( dispVector[ii]->XDisp() );
}

bool GxMainInterface::ProcessEvents(void) const
{
  return processEvents;
}

void GxMainInterface::ActivateTimer(TmrTimer &rTimer)
{
  rTimer.Activate(tMaster);
}

TmrTimerMaster & GxMainInterface::GetTimerMaster()
{
  return tMaster;
}

const char * GxMainInterface::GetAppName(void)
{
  return appName;
}

const char * GxMainInterface::GetClassName(void)
{
  return appName;
}

bool GxMainInterface::ParseDisplayArgs(int dispNum, GxArguments &rDispArgs, int argc, char** argv)
{
  //the value of dispNum here has a different meaning. if less than zero we search for the
  //DISPLAY environment variable
  ostringstream displayID1, displayID2, syncID1, syncID2, widthID, heightID;
  displayID1 << "-d";
  displayID2 << "--display";
  syncID1 << "-g";
  syncID2 << "--sync";
  widthID << "--width";
  heightID << "--height";
  
  if( dispNum >= 0)
    {
      displayID1 << dispNum;
      displayID2 << dispNum;
      syncID1 << dispNum;
      syncID2 << dispNum;
      widthID << dispNum;
      heightID << dispNum;
    }
  
  string dispName; //if non-zero length, the display has been named
  bool syncFound = false;
  unsigned width = 0; //if non-zero a valid value has been found
  unsigned height = 0; //if non-zero a valid value has been found

  if(argc >= 2)
    for(int ii = 1; ii < argc; ii++)
      {
	if( ( displayID1.str() == argv[ii] || displayID2.str() == argv[ii] ) && dispName.empty() )
	  {
	    if( ii+1 == argc )
	      {
		ostringstream err;
		err << "Display tag found at argument: " << ii << " but is missing subsequent specifier";
		SinkError( err.str() );
		return false;
	      }
	    
	    dispName = argv[ii+1];
	    argv[ii][0] = '\0';
	    argv[ii+1][0] = '\0';
	    ii++; //skip over display argument
	  }else
	    if( ( syncID1.str() == argv[ii] || syncID2.str() == argv[ii] ) && !syncFound )
	      {
		syncFound = true;
		argv[ii][0] = '\0';
	      }else
		if( widthID.str() == argv[ii] && width == 0 )
		  {
		    if( ii+1 == argc )
		      {
			ostringstream err;
			err << "Width tag found at argument: " << ii << " but is missing subsequent specifier";
			SinkError( err.str() );
			return false;
		      }
		    char *pTail = 0;
		    width = strtoul(argv[ii+1], &pTail, 10);
		    if( pTail == argv[ii+1] )
		      {
			ostringstream err;
			err << "Unable to parse width specified at argument: " << ii+1;
			SinkError( err.str() );
			return false;
		      };
		    if( !TestWidth(width) )
		      {
			ostringstream err;
			err << "Width specified at argument: " << ii+1 << " is out of range";
			SinkError( err.str() );
			width = 0;
			return false;
		      };
		    
		    argv[ii][0] = '\0';
		    argv[ii+1][0] = '\0';
		    ii++; //skip over argument
		  }else
		    if( heightID.str() == argv[ii] && height == 0)
		      {
			if( ii+1 == argc )
			  {
			    ostringstream err;
			    err << "Height tag found at argument: " << ii << " but is missing subsequent specifier";
			    SinkError( err.str() );
			    return false;
			  };
			
			char *pTail = 0;
			height = strtoul(argv[ii+1], &pTail, 10);
			if( pTail == argv[ii+1] )
			  {
			    ostringstream err;
			    err << "Unable to parse height specified at argument: " << ii+1;
			    SinkError( err.str() );
			    return false;
			  };
			
			if( !TestHeight(width) )
			  {
			    ostringstream err;
			    err << "Height specified at argument: " << ii+1 << " is out of range";
			    SinkError( err.str() );
			    width = 0;
			    return false;
			  };
			
			argv[ii][0] = '\0';
			argv[ii+1][0] = '\0';
			ii++; //skip over argument
		      };
      };

  if(dispName.empty() && dispNum < 0)
    {
      char *pEnvName = getenv("DISPLAY");
      if(pEnvName)
	dispName = pEnvName;
    };

  if( dispName.empty() )
    {
      SinkError("No display name specified or found in DISPLAY environmental variable");
      return false;
    };

  rDispArgs.displayName = dispName;
  rDispArgs.dispSync = syncFound;
  rDispArgs.mainWinWidth = width;
  rDispArgs.mainWinHeight = height;
  return true;
}

bool GxMainInterface::TestWidth(unsigned width)
{
  if( width < 400 || width > 4000 )
    return false;
  else
    return true;
}

bool GxMainInterface::TestHeight(unsigned height)
{
  if( height < 400 || height > 4000 )
    return false;
  else
    return true;
}

void GxMainInterface::SinkError(const std::string &rError)
{
  cerr << rError << endl;
}

void GxMainInterface::LocalErrorCallback(Display *pErrDisplay, const XErrorEvent &rEvent)
{
  //get the name of the GxDisplay's network connection that is connected to the error's display
  string dispName("<unknown>");
  for(unsigned ii = 0; ii < dispVector.size(); ii++)
    if( dispVector[ii]->XDisp() == pErrDisplay )
      {
	dispName = dispVector[ii]->GetDispName();
	break;
      };

  SinkError("Got x error from display: " + dispName);
  char eBuffer[1024];
  XGetErrorText(pErrDisplay, rEvent.error_code, eBuffer, 1024);
  SinkError( string(eBuffer) );
  SinkError( string("request that failed was: ") + request_text[rEvent.request_code] );
}

// ************************ start static stuff ********************

int GxMainInterface::ErrorHandler(Display *pErrDisplay, XErrorEvent *pError)
{
  if( !pMainInterface ) return 0; //should not happen, buy why die by default
  //assert( pMainInterface ) //hack? this seems a bit harsh

  pMainInterface->LocalErrorCallback(pErrDisplay, *pError);
  return 0;
}

GxMainInterface * GxMainInterface::pMainInterface = NULL;

const char* GxMainInterface::request_text[] = {
  "",
  "X_CreateWindow",
  "X_ChangeWindowAttributes",
  "X_GetWindowAttributes",
  "X_DestroyWindow",
  "X_DestroySubwindows",
  "X_ChangeSaveSet",
  "X_ReparentWindow",
  "X_MapWindow",
  "X_MapSubwindows",
  "X_UnmapWindow",
  "X_UnmapSubwindows",
  "X_ConfigureWindow",
  "X_CirculateWindow",
  "X_GetGeometry",
  "X_QueryTree",
  "X_InternAtom",
  "X_GetAtomName",
  "X_ChangeProperty",
  "X_DeleteProperty",
  "X_GetProperty",
  "X_ListProperties",
  "X_SetSelectionOwner",
  "X_GetSelectionOwner",
  "X_ConvertSelection",
  "X_SendEvent",
  "X_GrabPointer",
  "X_UngrabPointer",
  "X_GrabButton",
  "X_UngrabButton",
  "X_ChangeActivePointerGrab",
  "X_GrabKeyboard",
  "X_UngrabKeyboard",
  "X_GrabKey",
  "X_UngrabKey",
  "X_AllowEvents",
  "X_GrabServer",
  "X_UngrabServer",
  "X_QueryPointer",
  "X_GetMotionEvents",
  "X_TranslateCoords",
  "X_WarpPointer",
  "X_SetInputFocus",
  "X_GetInputFocus",
  "X_QueryKeymap",
  "X_OpenFont",
  "X_CloseFont",
  "X_QueryFont",
  "X_QueryTextExtents",
  "X_ListFonts",
  "X_ListFontsWithInfo",
  "X_SetFontPath",
  "X_GetFontPath",
  "X_CreatePixmap",
  "X_FreePixmap",
  "X_CreateGC",
  "X_ChangeGC",
  "X_CopyGC",
  "X_SetDashes",
  "X_SetClipRectangles",
  "X_FreeGC",
  "X_ClearArea",
  "X_CopyArea",
  "X_CopyPlane",
  "X_PolyPoint",
  "X_PolyLine",
  "X_PolySegment",
  "X_PolyRectangle",
  "X_PolyArc",
  "X_FillPoly",
  "X_PolyFillRectangle",
  "X_PolyFillArc",
  "X_PutImage",
  "X_GetImage",
  "X_PolyText8",
  "X_PolyText16",
  "X_ImageText8",
  "X_ImageText16",
  "X_CreateColormap",
  "X_FreeColormap",
  "X_CopyColormapAndFree",
  "X_InstallColormap",
  "X_UninstallColormap",
  "X_ListInstalledColormaps",
  "X_AllocColor",
  "X_AllocNamedColor",
  "X_AllocColorCells",
  "X_AllocColorPlanes",
  "X_FreeColors",
  "X_StoreColors",
  "X_StoreNamedColor",
  "X_QueryColors",
  "X_LookupColor",
  "X_CreateCursor",
  "X_CreateGlyphCursor",
  "X_FreeCursor",
  "X_RecolorCursor",
  "X_QueryBestSize",
  "X_QueryExtension",
  "X_ListExtensions",
  "X_ChangeKeyboardMapping",
  "X_GetKeyboardMapping",
  "X_ChangeKeyboardControl",
  "X_GetKeyboardControl",
  "X_Bell",
  "X_ChangePointerControl",
  "X_GetPointerControl",
  "X_SetScreenSaver",
  "X_GetScreenSaver",
  "X_ChangeHosts",
  "X_ListHosts",
  "X_SetAccessControl",
  "X_SetCloseDownMode",
  "X_KillClient",
  "X_RotateProperties",
  "X_ForceScreenSaver",
  "X_SetPointerMapping",
  "X_GetPointerMapping",
  "X_SetModifierMapping",
  "X_GetModifierMapping",
  "X_NoOperation" };
