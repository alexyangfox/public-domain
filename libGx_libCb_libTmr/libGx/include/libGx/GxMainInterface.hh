#ifndef GXMAININTERFACE_INCLUDED
#define GXMAININTERFACE_INCLUDED

/*
  an x app may have connections to several displays.  This class
   handles main() level activities like timers and command line
   arguments.  Events are handled at the individual GxDisplay level,
   as each has an independant file descriptor to the x server

   there should normally only be one of these per application

   there are several ways this class can be used. the first is the
   simplest and most straightforward, the use of only a single display
   connection. For this use the GxMainInterface(appName, 0) or
   GxMainInterface(appName) constructor.  The class will search the
   command line for un-suffexed display parameters like display and
   width. The dispVector will be sized only for a single
   connection. invalid display parameter specifications will be
   ignored

   if GxManInterface(appName, -1) is constructed, the number of
   displays to be connected to will be specified by the command line
   parameter --num-displays followed by an int that is arbritrarly
   constrained to be between 1 and 10 display parameters must be
   suffexed by an int like --display0 and --width0.  The DISPLAY
   environment variable is not examined.  the display0 through
   display<n> names must be fully specified.

   if GxMainInterface(appName, n) where n is from 1 to 10 is called
   then the parameter --num-displays is not looked for and like the
   second case the display0 through display<n> names must be fully
   specified. the DISPLAY environmental variable is not examined.

   redunant display parameters specified on the command line will be
   ignored (and are passed on to the application)
*/

#include <vector>

#include <libTmr/TmrTimer.hh>
#include <libTmr/TmrTimerMaster.hh>

#include <libGx/GxErrorSink.hh>
#include <libGx/GxInc.hh>

class GxDisplay;
class GxArguments;

const unsigned GX_APP_NAME_LENGTH = 512;

class GxMainInterface : public GxErrorSink
{
public:
  //the app name is needed for certain WM needs. used for applicaion
  //name atom and application class atom. By X conventions, the name should
  //have its first letter capitalized, and start with X.
  GxMainInterface(const char *pAppName, int tNumDisplays = 0);
  virtual ~GxMainInterface(void);


  //this is the way applications open and initialize all the displays.
  //this function first calls ParseDisplayArgs (see below) internally.
  //it then sizes and populates the dispVector with valid displays.
  //Initialize does not call OpenAllocateAll to allow the user to add colors
  //to the displays to be allocated
  virtual bool Initialize(int argc, char** argv);

  /*
    utility function that opens all x connections and allocates all
    necessary resources on all displays this then calls OpenAllocate
    on all the GxDisplays if any GxDisplay::OpenAllocate() fails, we
    return false.
  */
  virtual bool OpenAllocateAll(void);

  virtual void EventStep(void); //the meat of the event loop
  virtual void EventLoop(void); //this calls the individual display's HandleEvent internally
  virtual void EndEventLoop(void);
  virtual void FlushDisplays(void);
  virtual bool ProcessEvents(void) const; //usefull for ending an external event loop

  void ActivateTimer(TmrTimer &rTimer);
  //I don't like providing this, but it is useful.
  TmrTimerMaster & GetTimerMaster();

  // ******** all functions below this probably won't be used by app programmers ************
  const char * GetAppName(void);
  const char * GetClassName(void);

  // ************** GxErrorSink overloads *********************
  //default error reporting facility for all of libGx
  virtual void SinkError(const std::string &rError);

  //this was deliberately left public to allow the app programmer to not use the built in EventLoop()
  //by immediately hacking a specialized event loop into main.
  //it turns out having this public is increadibly convienient.
  std::vector<GxDisplay*> dispVector;

protected:

  //the default application limits for main window width and height as specified on cmd line are 400 to 4000
  //(4000 is probably going to be much to low a few years from now )
  //overload for special needs.
  virtual bool TestWidth(unsigned width);
  virtual bool TestHeight(unsigned height);

  char appName[GX_APP_NAME_LENGTH];
  int numDisplays; //directly from constructor, possibly modified by Initialize()

  bool processEvents;
  TmrTimerMaster tMaster;

  //this _modifies_ argv to strip libGx argumetns from it by setting arg[n][0] = '\0' if
  //it considers arg[n] to be a x related parameter.
  //if displayNum == -1 then the function looks for non-numbered arguments only
  //and examines the DISPLAY enviromental variable.
  //all found parameters are filled into rProgArgs
  bool ParseDisplayArgs(int dispNum, GxArguments &rDispArgs, int argc, char** argv);

  //the static XError handler jumps to this via pMainInterface
  void LocalErrorCallback(Display *pErrDisplay, const XErrorEvent &rEvent);

  // ************************ start static stuff ********************
  static int ErrorHandler(Display *pErDisplay, XErrorEvent *pError);
  static GxMainInterface * pMainInterface; //set non-null in first constructor, but only once. asserts if non-null
  static const char *request_text[];
};

#endif //GXMAININTERFACE_INCLUDED
