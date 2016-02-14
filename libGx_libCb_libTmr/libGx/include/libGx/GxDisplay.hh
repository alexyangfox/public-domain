#ifndef GXDISPLAY_INCLUDED
#define GXDISPLAY_INCLUDED

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <string>

#include <libCb/CbCallback.hh>

#include <libGx/GxInc.hh>
#include <libGx/GxMainInterface.hh>
#include <libGx/GxOwner.hh>
#include <libGx/GxCoreWinMap.hh>
#include <libGx/GxDisplayInfo.hh>
#include <libGx/GxVolatileData.hh>
#include <libGx/GxMapHolder.hh>
#include <libGx/GxCentralColor.hh>
#include <libGx/GxTrueColorMapInfo.hh>
#include <libGx/GxArguments.hh>

//hackish: we do not yet have any functionality for supporting multiple screens
//this is the only class which derives purely from GxOwner
class GxDisplay : public GxOwner
{
public:
  GxDisplay(GxMainInterface &rTMainInt, const GxArguments &rArgs);
  virtual ~GxDisplay(void);

  Display* XDisp(void);

  //push event handler pushes rEvHandler on top of the event stack and places
  //its id in rNewID. they are used in GxPopupWin. this lets widgets capture all server events
  //during grabs, like for Menu bar interaction etc. calling XNextEvent in widgets is no longer allowed.
  void PushEventHandler(const CbOneBase<const XEvent&> &rEVHandler, GxEventHandlerID &rNewID);
  void RemoveEventHandler(const GxEventHandlerID &rID);
  /*
    a bit hackish. Used by objects that push event handlers above. if
    the object receives events it cannot handle, yet are safe to be
    processed by other windows while still in its 'internal' event
    'loop', it can pass the unhandled event to this
  */
  void HandleSafeLoopEvent(const XEvent &rEvent);

  void HandleEvent(const XEvent &rEvent);

  //what we passed to xlib to open this Display. used for printing errors in GxMainInterface
  const std::string& GetDispName(void) const;

  //?hack? -> actually unimplemented  ?delete?
  Window GetMainAppWin(void);

  //overloads of GxOwner
  Window GetClosestXWin(void); //return the root window
  GxMapHolder *GetClosestMapHolder(void);
  void UnManageWindow(Window winID);
  virtual void MoveFocusToChild(GxWinArea *pChild, Time eventTime);

  GxDisplayInfo& GetDisplayInfo(void);
  GxVolatileData& GetVolatileData(void);

  //this assigns numbers to the Central Colors in the same order as they were added.
  void AddCentralColor(const GxCentralColor &rNewColor);
  //this lets the user get the Central Colors pixel id's after they have been allocated.
  ULINT GetCentralColorPix(int colorNum);
  //this returns false if we don't have a true color visual. this can
  //work only _after_ we have Initialized successfully.
  //hackish. this should be replaced with a generic RGB color interface.
  bool GetTrueColorMapInfo(GxTrueColorMapInfo &rInfo);

  //this is called by GxMainInterface's Initialize, overload for custom/specialty display needs
  virtual bool OpenAllocate(void);

  virtual void PushCutText(const std::string &rText);
  virtual void PullCutText(std::string &rText, unsigned numChars);
protected:
  GxArguments gxArgs;

  //tries to allocate all colors using the colormap tempMap. returns false
  //if any of the colors cannot be allocated
  bool AllocateCentralColors(void);

  GxMainInterface &rMainInt;

  class CentralColorCont
  {
  public:
    CentralColorCont(GxCentralColor *pTColor);
    ~CentralColorCont(void);
    
    GxCentralColor *pColor;
    
    CentralColorCont *pNextCont;
  };

  CentralColorCont *pFirstColorCont;

  std::list<CbOneBase<const XEvent&>*> eventStack;
  //the matrix of Central colors allocated
  XColor *pColorMatrix;
  int colorMatrixSize;

  Display *pDisplay;
  Colormap tempMap;
  int screenNum;
  Window rootWin;

  //we keep this information so we can call XFreeFont
  XFontStruct *pDefaultFont;
  XFontStruct *pMenuFont;
  //if true i.e. pMenuFont is not same as pDefaultFont and
  //will have to be Freed via XFreeFont
  bool menuFontUnique;

  GxDisplayInfo DInfo;
  GxVolatileData VData;

  //  GxCoreWinMap winMap;
  //hack; perhaps we should inherit from this
  GxMapHolder mapHolder;
};

#endif //GXDISPLAY_INCLUDED
