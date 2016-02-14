#include <assert.h>
#include <errno.h>

#include <libGx/GxDisplay.hh>

//hack; this is the number of GxDefaultColors we allocate for
//the library, this should be only used in this class
#define NUM_SYS_COLORS 6

using namespace std;

GxDisplay::GxDisplay(GxMainInterface &rTMainInt, const GxArguments &rArgs) :
  gxArgs(rArgs), rMainInt(rTMainInt), pFirstColorCont(NULL), pDisplay(NULL), DInfo(rTMainInt, *this), VData()
{
  pColorMatrix = NULL;
  colorMatrixSize = 0;

  GxCentralColor cColor;
  // ************* background color (color 0) ******************
  cColor.AddColor("grey78");
  cColor.AddColor("grey79");
  cColor.AddColor("grey77");
  cColor.AddColor("grey80");
  cColor.AddColor("grey76");
  cColor.AddColor("grey81");
  cColor.AddColor("grey82");
  
  AddCentralColor(cColor);
  cColor.ClearColorList();//get ready for next use

  // ************* recessed color  (color 1) ******************
  cColor.AddColor("grey70");
  cColor.AddColor("grey69");
  cColor.AddColor("grey68");
  cColor.AddColor("grey67");
  cColor.AddColor("grey66");
  cColor.AddColor("grey65");
  cColor.AddColor("grey64");
  cColor.AddColor("grey63");
  cColor.AddColor("grey62");
  cColor.AddColor("grey61");
  cColor.AddColor("grey60");

  AddCentralColor(cColor);
  cColor.ClearColorList();//get ready for next use

  // ************* light border color (color 2) ******************
  cColor.AddColor("grey96");
  cColor.AddColor("grey97");
  cColor.AddColor("grey98");
  cColor.AddColor("grey99");
  cColor.AddColor("grey95");
  cColor.AddColor("grey94");
  cColor.AddColor("grey93");
  cColor.AddColor("grey92");
  cColor.AddColor("grey100");

  AddCentralColor(cColor);
  cColor.ClearColorList();//get ready for next use

  // ************* dark border color (color 3) ******************
  cColor.AddColor("grey47");
  cColor.AddColor("grey46");
  cColor.AddColor("grey48");
  cColor.AddColor("grey45");
  cColor.AddColor("grey49");
  cColor.AddColor("grey44");
  cColor.AddColor("grey50");
  cColor.AddColor("grey43");
  cColor.AddColor("grey42");

  AddCentralColor(cColor);
  cColor.ClearColorList();//get ready for next use

  // ************* tool tip background color (color 4) ******************
  cColor.AddColor("yellow");
  cColor.AddColor("gold");
  cColor.AddColor("LightGoldenrod");
  cColor.AddColor("goldenrod");
  cColor.AddColor("wheat");
  cColor.AddColor("yellow1");
  cColor.AddColor("yellow2");
  cColor.AddColor("yellow3");
  cColor.AddColor("yellow4");

  AddCentralColor(cColor);
  cColor.ClearColorList();//get ready for next use

  // ************* percent bar color (color 5) ******************
  cColor.AddColor("Blue");
  cColor.AddColor("SkyBlue");
  cColor.AddColor("medium blue");
  cColor.AddColor("navy");
  cColor.AddColor("steel blue");
  cColor.AddColor("pale turquoise");
  cColor.AddColor("DarkTurquoise");
  cColor.AddColor("medium aquamarine");
  cColor.AddColor("CadetBlue");
  cColor.AddColor("LightBlue");

  AddCentralColor(cColor);
  //cColor.ClearColorList();
}

GxDisplay::~GxDisplay(void)
{
  //DestroyChildren();
  
  if(pColorMatrix)
    {
      delete [] pColorMatrix;
      pColorMatrix = 0;
    };
  
  while(pFirstColorCont)
    {
      CentralColorCont *pNewFront = pFirstColorCont->pNextCont;

      delete pFirstColorCont->pColor;
      pFirstColorCont->pColor = 0;

      delete pFirstColorCont;
      pFirstColorCont = 0;

      pFirstColorCont = pNewFront;
    };

  VData.VDFreeAll(DInfo);

  if( DInfo.pDefaultFont )
    {
      XFreeFont(DInfo.display, DInfo.pDefaultFont);
      DInfo.pDefaultFont = 0;
    };
      
  if( DInfo.pMenuFont )
    {
      if( menuFontUnique )
	XFreeFont(DInfo.display, DInfo.pMenuFont);
      DInfo.pMenuFont = 0;
    };

  if(pDisplay)
    {
      //this should not happen
      XCloseDisplay(pDisplay);
      pDisplay = 0;
      DInfo.display = 0;
    };
}

const std::string& GxDisplay::GetDispName(void) const
{
  return gxArgs.displayName;
}

bool GxDisplay::OpenAllocate(void)
{
  pDisplay = XOpenDisplay( gxArgs.displayName.c_str() );
  if(!pDisplay)
    {
      rMainInt.SinkError( string("Failed to open Display: ") + GetDispName() );
      return false;
    };

  if(gxArgs.dispSync)
    {
      rMainInt.SinkError( string("Forcing requested sync on display: ") + GetDispName() );
      XSynchronize(pDisplay, true);
    };

  screenNum = DefaultScreen(pDisplay);
  rootWin = RootWindow(pDisplay, screenNum);

  Visual *pVis = DefaultVisual(pDisplay, screenNum);
  tempMap = DefaultColormap(pDisplay, screenNum);

  int numVisuals;
  long mask = 0;
  XVisualInfo templateVis;
  XVisualInfo *pVisInfo = XGetVisualInfo(pDisplay, mask, &templateVis,
					 &numVisuals);
  if(!pVisInfo) //problem. we should have goton a list of all visuals on the server. failure.
    {
      rMainInt.SinkError( string("Error getting server visual information on display: ") + GetDispName() );
      return false;
    };
  unsigned visualToUseNum = 0;

#ifdef LIBGX_DEBUG_BUILD
  cout << "number of visuals: " << numVisuals << endl;

    const char *pVisNames[6] = {"StaticGrey",
    "GreyScale",
    "StaticColor",
    "PseudoColor",
    "TrueColor",
    "DirectColor"};

#endif //LIBGX_DEBUG_BUILD

  for(int ii = 0; ii < numVisuals; ii++)
    {
#ifdef LIBGX_DEBUG_BUILD
      cout << "visual type: " << pVisNames[ pVisInfo[ii].c_class ]
	   << " depth: " << pVisInfo[ii].depth << endl;
#endif //LIBGX_DEBUG_BUILD
      if(pVisInfo[ii].visual == pVis)
	{
	  visualToUseNum = ii;
#ifdef LIBGX_DEBUG_BUILD
	  cout << "\tthis is the default visual" << endl;
#endif //LIBGX_DEBUG_BUILD
	};
    };

  XStandardColormap *pStdColormap = 0;
  int numMaps;

  int res = XGetRGBColormaps(pDisplay, rootWin, &pStdColormap, &numMaps,
			     XA_RGB_DEFAULT_MAP);
#ifdef LIBGX_DEBUG_BUILD
  cout << "RGB res: " << res << endl;
#endif //LIBGX_DEBUG_BUILD

  if(pStdColormap && res != 0)
    XFree(pStdColormap);

  //central colors adding should be here
  //XColor color;

  if(!AllocateCentralColors())
    {
      rMainInt.SinkError( string("Color allocation failed on display: ") + GetDispName() );
      return false;
    };

  pDefaultFont = XLoadQueryFont(pDisplay, "fixed");
  if(!pDefaultFont)
    {
      rMainInt.SinkError( string("Unable to load default font on display: ") + GetDispName() );
      return false;
    };

  pMenuFont = XLoadQueryFont(pDisplay, "-adobe-helvetica-medium-r-normal--12-*");
  if(!pMenuFont)
    {
      rMainInt.SinkError( string("Unable to load menu font so falling back to default on display: ") + GetDispName() );
      pMenuFont = pDefaultFont;
      menuFontUnique = false;
    }else
      menuFontUnique = true;

  //getting atoms
  Atom wmProtocols = XInternAtom(pDisplay, "WM_PROTOCOLS", false);
  Atom takeFocus = XInternAtom(pDisplay, "WM_TAKE_FOCUS", false);
  Atom deleteWindow = XInternAtom(pDisplay, "WM_DELETE_WINDOW", false);

 const unsigned char nullBuffer = '\0';
 for(unsigned ii = 0; ii < 8; ii++)
   {
     Atom cAtom;
     switch(ii)
       {
       case 0:
	 cAtom = XA_CUT_BUFFER0;
	 break;
       case 1:
	 cAtom = XA_CUT_BUFFER1;
	 break;
       case 2:
	 cAtom = XA_CUT_BUFFER2;
	 break;
       case 3:
	 cAtom = XA_CUT_BUFFER3;
	 break;
       case 4:
	 cAtom = XA_CUT_BUFFER4;
	 break;
       case 5:
	 cAtom = XA_CUT_BUFFER5;
	 break;
       case 6:
	 cAtom = XA_CUT_BUFFER6;
	 break;
       default: //case 7
	 cAtom = XA_CUT_BUFFER7;
	 break;
       };

     if( !XChangeProperty(pDisplay, rootWin, cAtom, XA_STRING, 8, PropModeAppend, &nullBuffer, 0) )
       {
	 rMainInt.SinkError( string("Error initializing cut buffer on display: ") + GetDispName() );
       };
   };

  //hack should seperatly load other colors
  DInfo.FillComplete(pDisplay, screenNum, rootWin,
		     pVisInfo[visualToUseNum], tempMap,
		     pColorMatrix[0].pixel, pColorMatrix[1].pixel,
		     pColorMatrix[2].pixel, pColorMatrix[3].pixel,
		     BlackPixel(pDisplay, screenNum), pColorMatrix[3].pixel,
		     BlackPixel(pDisplay, screenNum), WhitePixel(pDisplay, screenNum), pColorMatrix[5].pixel,
		     pColorMatrix[4].pixel, pColorMatrix[5].pixel,
		     pDefaultFont, pMenuFont,
		     wmProtocols, takeFocus, deleteWindow);

  //we have copied the relevant structure from the list. we know the pointer pVisInfo
  //is non-null as we checked it above
  XFree(pVisInfo);
  pVisInfo = 0;

  VData.VDAllocAll(DInfo);

  DInfo.travKeyCode = XKeysymToKeycode(pDisplay, XK_Tab);
  DInfo.defaultCursor = XCreateFontCursor(pDisplay, XC_left_ptr);
  DInfo.vDividerCursor = XCreateFontCursor(pDisplay, XC_sb_h_double_arrow);

  return true;
}

Display* GxDisplay::XDisp(void)
{
  return pDisplay;
}


void GxDisplay::PushCutText(const std::string &rText)
{
  if( rText.empty() ) return;

  Atom aVec[8];
  aVec[0] = XA_CUT_BUFFER0;
  aVec[1] = XA_CUT_BUFFER1;
  aVec[2] = XA_CUT_BUFFER2;
  aVec[3] = XA_CUT_BUFFER3;
  aVec[4] = XA_CUT_BUFFER4;
  aVec[5] = XA_CUT_BUFFER5;
  aVec[6] = XA_CUT_BUFFER6;
  aVec[7] = XA_CUT_BUFFER7;

  if( !XRotateWindowProperties(DInfo.display, DInfo.rootWin, aVec, 8, 1) )
    {
      rMainInt.SinkError( string("Error rotating cut buffers on display: ") + GetDispName() );
    };
  
  XStoreBytes(DInfo.display, rText.c_str(), rText.size() );
}

void GxDisplay::PullCutText(std::string &rText, unsigned numChars)
{
  rText.clear();
  if(numChars == 0) return;

  int nBytes = 0;
  char *pChars = XFetchBytes(DInfo.display, &nBytes);
  
  int finalNBytes = (nBytes < numChars) ? nBytes : numChars;
  rText = string(pChars, finalNBytes);
}

void GxDisplay::PushEventHandler(const CbOneBase<const XEvent&> &rEVHandler,
				 GxEventHandlerID &rNewID)
{
  //make a class local copy
  CbOneBase<const XEvent&> *pCB = rEVHandler.Clone();
  eventStack.push_front(pCB);
  rNewID = (GxEventHandlerID)pCB;
}

void GxDisplay::RemoveEventHandler(const GxEventHandlerID &rID)
{
  if( eventStack.empty() ) return;

  list<CbOneBase<const XEvent&>*>::iterator cPlace = eventStack.begin();
  list<CbOneBase<const XEvent&>*>::iterator cEnd = eventStack.end();
  while(cPlace != cEnd)
    {
      CbOneBase<const XEvent&> *pCCB = *cPlace;
      if( ((GxEventHandlerID)pCCB) == rID)
	{
	  cPlace = eventStack.erase(cPlace);
	  delete pCCB;
	  pCCB = 0;
	  return;
	};
      cPlace++;
    };
}

void GxDisplay::HandleSafeLoopEvent(const XEvent &rEvent)
{
  mapHolder.SendXEventLocally(rEvent);
}

void GxDisplay::HandleEvent(const XEvent &rEvent)
{
  //calls the registered event handler.
  if( !eventStack.empty() )
    {
      CbOneBase<const XEvent&>* pCCB = eventStack.front();
      pCCB->DoCallback(rEvent);
    }else
    mapHolder.SendXEventLocally(rEvent);
}

Window GxDisplay::GetMainAppWin(void)
{
  return None;
}

Window GxDisplay::GetClosestXWin(void)
{
  return rootWin;
}

GxMapHolder* GxDisplay::GetClosestMapHolder(void)
{
  return &mapHolder;
}

void GxDisplay::UnManageWindow(Window winID)
{
  mapHolder.UnManageWin(winID);
}

void GxDisplay::MoveFocusToChild(GxWinArea *pChild, Time eventTime)
{}

GxDisplayInfo& GxDisplay::GetDisplayInfo(void)
{
  return DInfo;
}

GxVolatileData& GxDisplay::GetVolatileData(void)
{
  return VData;
}

void GxDisplay::AddCentralColor(const GxCentralColor &rNewColor)
{
  GxCentralColor *pNewColor = rNewColor.Clone();

  if(!pFirstColorCont)
    {
      pFirstColorCont = new CentralColorCont(pNewColor);
      return;
    };

  CentralColorCont *pTempCont = pFirstColorCont;

  while(pTempCont->pNextCont)
    pTempCont = pTempCont->pNextCont;

  pTempCont->pNextCont = new CentralColorCont(pNewColor);
}

ULINT GxDisplay::GetCentralColorPix(int colorNum)
{
  if(colorNum + NUM_SYS_COLORS > colorMatrixSize)
    {
      rMainInt.SinkError("quering for out of range central color");
#ifdef LIBGX_DEBUG_BUILD
      assert(false);
#endif //LIBGX_DEBUG_BUILD
      return DInfo.blackPix;
    };

  return (pColorMatrix[colorNum + NUM_SYS_COLORS]).pixel;
}

//this algorithm is shared with libImg's ImgXDrawData
bool GxDisplay::GetTrueColorMapInfo(GxTrueColorMapInfo &rInfo)
{
  if( DInfo.cVisualInfo.c_class != TrueColor) return false;

#ifdef LIBGX_DEBUG_BUILD
  cout << hex
       << DInfo.cVisualInfo.red_mask << '\n'
       << DInfo.cVisualInfo.green_mask << '\n'
       << DInfo.cVisualInfo.blue_mask << '\n' << dec;
#endif //LIBGX_DEBUG_BUILD

  unsigned redOffset = 0;
  for(unsigned ii = 0; ii < 32; ii++)
    {
      if( DInfo.cVisualInfo.red_mask&(1<<ii) )
	{
	  rInfo.red_mult = 1<<ii;
	  redOffset = ii;
	  break;
	};
    };
  rInfo.red_max = DInfo.cVisualInfo.red_mask>>redOffset;

  unsigned greenOffset = 0;
  for(unsigned ii = 0; ii < 32; ii++)
    {
      if( DInfo.cVisualInfo.green_mask&(1<<ii) )
	{
	  rInfo.green_mult = 1<<ii;
	  greenOffset = ii;
	  break;
	};
    };
  rInfo.green_max = DInfo.cVisualInfo.green_mask>>greenOffset;

  unsigned blueOffset = 0;
  for(unsigned ii = 0; ii < 32; ii++)
    {
      if( DInfo.cVisualInfo.blue_mask&(1<<ii) )
	{
	  rInfo.blue_mult = 1<<ii;
	  blueOffset = ii;
	  break;
	};
    };
  rInfo.blue_max = DInfo.cVisualInfo.blue_mask>>blueOffset;

#ifdef LIBGX_DEBUG_BUILD
  cout << rInfo.red_max << " " << redOffset << '\n'
       << rInfo.green_max << " " << greenOffset<< '\n'
       << rInfo.blue_max << " " << blueOffset << endl;
#endif //LIBGX_DEBUG_BUILD

  return true;
}

bool GxDisplay::AllocateCentralColors(void)
{
  CentralColorCont *pTempCont = pFirstColorCont;
  int numColors = 0;
  while(pTempCont)
    {
      if( !pTempCont->pColor->Allocate(rMainInt, *this, tempMap) )
	return false;

      numColors++;
      pTempCont = pTempCont->pNextCont;
    };

  //put the XColors into a matrix and delete the list of colors
  pColorMatrix = new XColor[numColors];
  colorMatrixSize = numColors;

  pTempCont = pFirstColorCont;
  for(int i = 0; i < numColors; i++)
    {
      pColorMatrix[i] = pTempCont->pColor->GetColor();

      pTempCont = pTempCont->pNextCont;
    };

  return true;
}

// ********************* GxCentralColorCont ***************************

GxDisplay::CentralColorCont::CentralColorCont(GxCentralColor *pTColor) :
  pColor(pTColor), pNextCont(NULL)
{}

GxDisplay::CentralColorCont::~CentralColorCont(void)
{}
