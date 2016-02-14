#include <string.h>

#include <libGx/GxDisplayInfo.hh>

#include "GxDefines.hh"

GxDisplayInfo::GxDisplayInfo(GxMainInterface &rTMainInterface, GxDisplay &rTGxDisplay ) :
  rMainInterface(rTMainInterface), rGxDisplay(rTGxDisplay), pDefaultFont(0), pMenuFont(0)
{}

GxDisplayInfo::~GxDisplayInfo(void)
{}

void GxDisplayInfo::FillComplete(Display *pTXDisplay, int tScreen,
				 Window tRootWin,
				 const XVisualInfo &rVisualInfo, Colormap tCMap,
				 ULINT bgPix, ULINT rPix,
				 ULINT lbPix, ULINT dbPix,
				 ULINT tLabelTextPix, ULINT tUnActiveLabelTextPix,
				 ULINT tTextPix, ULINT tSelectedTextPix,
				 ULINT tSelectedTextBackgroundPix,
				 ULINT tToolTipBGPix, ULINT tPercentBarPix,
				 XFontStruct *pTDefaultFont,
				 XFontStruct *pTMenuFont,
				 Atom tWMProtocols, Atom tWMTakeFocus, Atom tDeleteWin)


{
  display = pTXDisplay;
  screenNum = tScreen;
  rootWin = tRootWin;
  //pVisual = pTVisual;
  memcpy(&cVisualInfo, &rVisualInfo, sizeof(XVisualInfo));
  cMap = tCMap;

  blackPix = BlackPixel(display, screenNum);
  whitePix = WhitePixel(display, screenNum);

  backgroundPix = bgPix;
  recessedPix = rPix;
  lightBorderPix = lbPix;
  darkBorderPix = dbPix;

  labelTextPix = tLabelTextPix;
  unActiveLabelTextPix = tUnActiveLabelTextPix;

  textPix = tTextPix;
  selectedTextPix = tSelectedTextPix;
  selectedTextBackgroundPix = tSelectedTextBackgroundPix;

  toolTipBGPix = tToolTipBGPix;
  percentBarPix = tPercentBarPix;

  gxBorderWidth = GX_BORDER_WD;

  pDefaultFont = pTDefaultFont;
  pMenuFont = pTMenuFont;

  wmProtocols = tWMProtocols;
  takeFocus = tWMTakeFocus;
  deleteWindow = tDeleteWin;
}
