#ifndef GXEDITWIN_INCLUDED
#define GXEDITWIN_INCLUDED

//a class which allows the user to edit a single line of text

#include <string.h>
#include <assert.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <libCb/CbCallback.hh>
#include <libTmr/TmrTimer.hh>

#include <libGx/GxInc.hh>
#include <libGx/GxWin.hh>
#include <libGx/GxCursor.hh>
#include <libGx/GxKeyHandler.hh>

/*
  minimal editing functionality for this class:

  select text with left mouse button
  make sure pointer never becomes invisible when entering text
  delete selected text with delete or backspace key
  entry of text deletes any highlighed text
  move cursor along string with left & right arrow keys
  Ctrl-A move cursor to front of line (include autoscroll to view cursor)
  Ctrl-E move cursor to end of line (include autoscroll to view cursor)
  Ctrl-K kill to end of line
  selecting text with pointer should autoscroll highlight if text extends beyond edge of box.
  double click selects all text in window (i.e. prepare for delete key)
  copy text by highlighting via X selection
  paste text with center mouse button via X selection
*/

//number of characters which can fit inside a GxEditWin
const unsigned GX_EDIT_WIN_SIZE = 1024;

class GxEditWin : public GxWin, public GxKeyHandler
{
public:
  GxEditWin(GxRealOwner *pOwner);
  ~GxEditWin(void);

  //this is only used in GetDesiredWidth
  void SetNumVisibleChars(UINT newNum);

  //not constant because it forces a null termination.
  //there is a c++ path around this, but it is not implemented
  const char* GetText(void);
  //to clear the text in the window, call SetText with an empty string
  void SetText(const char* pNewText);
  void SetText(const char* pNewText, int numChars);

  //if editStatus is false the editwin is output only and can only be changed
  //by the SetText above. If edit status is true the editwin behaves normally.
  //The default status is TRUE
  void SetEditable(bool editStatus);
  void SetActive(bool tActiveState);

  virtual bool AcceptFocus(Time eventTime);

  void HandleEvent(const XEvent &rEvent);

  //called when the enter key is hit in the editwin
  CbVoidFO enterCB;
  //called anytime the string is modified by the user
  CbVoidFO modifyCB;

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

protected:

  void HandleGrabbedEvents(const XEvent &rEvent); //for button 2

  void GetWindowData(XSetWindowAttributes &winAttributes, ULINT &valueMask);
  GxEventHandlerID pushedHandlerID;

  enum AS_POS{AS_INSIDE, AS_LEFT, AS_RIGHT};
  AS_POS asPos;
  //called by timer whenever button1 is pressed in window
  bool inhibitAS;
  void AutoScrollCB(void);
  TmrTimer asTimer;

  Time dc_c1_time; //time of the initial click for a possible double click
  enum DC_STATE{DC_P1, DC_R1, DC_P2}; //DC_R2;
  DC_STATE dc_state;

  bool b2Pressed; //button 2 pressed. used in private event loop.

  void Draw(void);
  void DrawEraseCursor(bool draw);
  //clears the window except the border
  void ClearTextArea(void);

  //this goes from pixels to string position.
  //this must return a valid space in the string
  //0 is before the first char.
  //only returns visible string positions.
  int LookUpPlace(int xEv);
  //this goes from string position to pixels
  int LookUpPixel(int strPosition);

  void AdjustSelection(int eventStrPos);

  //returns true if it does
  bool AdjustStartPlace(void);
  void ClearSelection(void);
  void DeleteSelection(void);

  // ************* start GxKeyHandler overloads ***************
  virtual void HandleKeyString(const char *pBuffer, unsigned long buffLen);
  virtual void MoveCursorLineStart(void);
  virtual void MoveCursorLineEnd(void);
  virtual void KillTextToLineEnd(void);
  virtual void KeyLeft(void);
  virtual void KeyRight(void);
  virtual void KeyDelete(void);
  virtual void KeyBackspace(void);
  virtual void KeyEnter(void);
  // ************* end GxKeyHandler overloads ***************

  bool haveFocus;

  //where we start drawing the text
  int textY;

  //this is used in Size to determine what my width should be
  UINT numCharsVisible;

  //hack; shouldn't I be using UINT's here- nothing should be negative.
  bool editable;
  bool active;
  char text[GX_EDIT_WIN_SIZE]; // NOT null terminated
  unsigned currentSize; //the number of chars in the text matrix
  //the first character of the visable test
  int startPlace;
  //cursor position inside the string; can be less than or equal currentSize. it is the position
  //within the text matrix that a new character would be inserted into.
  int cPosition;
  //if numSelected == 0 nothing is selected. if nothing selected selectedStart does not have to be sane.
  int selectedStart, numSelected;
  //int cPixPosition; //pixel position of cursor. hack? was it good to remove this?
  //place in the string where the mouse button was pressed
  //used when cutting and pasting
  int pressPlace;
  //only used during the server grab between button press and button release
  int currentPos;

  //a utility class used by the Draw() function. we do not maintain a display list.
  class DrawFragment
  {
  public:
    DrawFragment(void); //the constructor creates a Reset() object
    DrawFragment(const DrawFragment &rhs);
    ~DrawFragment(void);

    const DrawFragment& operator=(const DrawFragment &rhs);

    void Reset(void); //sets startPlace and endPlace to 0 and sets highlighted to false

    int startPlace, numChars;
    bool highlighted;
  };

};

#endif //GXEDITWIN_INCLUDED
