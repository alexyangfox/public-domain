#ifndef GXDISPLAYINFO_INCLUDED
#define GXDISPLAYINFO_INCLUDED

#include <libGx/GxInc.hh>

/*
  one of these must be alocated for each display/screen combination
  (usually just one per aplication)
  MUST NOT CHANGE INTERNAL STUFF AFTER CREATION ?const?
*/

class GxDisplay;
class GxMainInterface;

/* this class does not own any of its contents */
class GxDisplayInfo
{
private:
  //only one of these should exist per display so lets make it more difficult to copy.
  //this is not implemented.
  GxDisplayInfo(const GxDisplayInfo &rhs);
public:
  GxDisplayInfo(GxMainInterface &rTMainInterface, GxDisplay &rTGxDisplay);
  ~GxDisplayInfo(void);

  //fills in most of the members that are not available at time of construction
  void FillComplete(Display *pTXDisplay, int tScreen,
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
		    Atom wmProtocols, Atom takeFocus, Atom tDeleteWin);

  GxMainInterface &rMainInterface;
  //one reason this is good to be here is if a widget goes into an internal event loop,
  //it can send unused display events through the display. It could SendEventsUp if it were
  //an owner win, but using this is more generic.
  GxDisplay &rGxDisplay;

  Display* display;
  int screenNum;
  Window rootWin;

  //Visual *pVisual;
  XVisualInfo cVisualInfo; //the visual info we use to create all our windows with.
  /*
    //I think visual is a pointer to a structure allocated and maintained by Xlib. we just copy the XVisualInfo
    //structure so if I'm wrong, this is broken.
    typedef struct {
    Visual *visual;
    VisualID visualid;
    int screen;
    unsigned int depth;
    int class;
    unsigned long red_mask;
    unsigned long green_mask;
    unsigned long blue_mask;
    int colormap_size;
    int bits_per_rgb;
    } XVisualInfo;
  */

  Colormap cMap;
  ULINT blackPix;
  ULINT whitePix;
  ULINT backgroundPix;
  ULINT recessedPix;
  ULINT lightBorderPix;
  ULINT darkBorderPix;

  //the text color of any labeling text; inside buttons, menu items, etc.
  ULINT labelTextPix;
  //the text color of any "widget" which is not to respond to user input
  ULINT unActiveLabelTextPix;

  //the text and related colors in GxEditWin && GxTextWindow
  ULINT textPix;
  ULINT selectedTextPix;
  ULINT selectedTextBackgroundPix;

  //the text of a tooltip background
  ULINT toolTipBGPix;

  //the color of the bar in the GxPercentBar
  ULINT percentBarPix;
  
  unsigned gxBorderWidth; //set to be the compiled default. for use by clients

  //should have a menubar font, a label font, a general text font
  //perhaps a menuitem font?

  XFontStruct *pDefaultFont;
  //hack; most/all of the menu information should be in a seperate object
  //alocated when a menuObject is created and deleted when the last menu
  //object is deleted. However all of the information is dependant on which
  //display the MenuObject is on, so for right now I will leave all of the
  //stuff here.
  XFontStruct *pMenuFont;

  //I probably need to have several XFontStruct*'s here to simplifiy the life
  //of aplication writers who just need a few extra fonts

  int travKeyCode; //the traversal keycode
  Cursor defaultCursor; //this exists particularly for pointer grabs that need a cursor. it is an arrow.
  Cursor vDividerCursor; //used for the GxVDivider

  Atom wmProtocols; //WM_PROTOCOLS
  Atom takeFocus; //WM_TAKE_FOCUS
  Atom deleteWindow; //WM_DELETE_WINDOW
};

#endif //GXDISPLAYINFO_INCLUDED
