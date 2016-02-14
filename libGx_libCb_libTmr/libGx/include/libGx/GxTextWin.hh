#ifndef GXTEXTWIN_INCLUDED
#define GXTEXTWIN_INCLUDED

#include <list>

#include <libGx/GxInc.hh>
#include <libGx/GxOwnerWin.hh>
#include <libGx/GxScrolledWin.hh>
#include <libGx/GxScrollBar.hh>
#include <libGx/GxGeomControl.hh>
#include <libGx/GxKeyHandler.hh>
#include <libGx/GxCursor.hh>

#include <list>
#include <string>
#include <sstream>
#include <ctype.h>
#include <string.h>

// ********************************** ATTENTION ******************************
//
// this is one of the places that make a debug build library have to be linked
// with probrams compiled with the LIBGX_DEBUG_BUILD flag set
//
// ***************************************************************************

/*
#ifdef LIBGX_DEBUG_BUILD
const unsigned int GXTWUNIT = 12;
#else
const unsigned int GXTWUNIT = 1024; //?should this be bigger?
#endif
*/
const unsigned int GXTWUNIT = 12; //hack. revert.

//a class which can hold a 'huge' element of text.
//unlimited in size (except for availiable memory)
//holds text in a linked list of char matrices.
/*
  TextPlace's represent the place where a new character would be inserted.
  a character that is in the TextPlace's place is moved to the next place.
  if an insertion place's subPlace is zero a new character is inserted
  at the end of the previous's TextUnits text. (same thing)
  it is invalid for a TextPlace's subPlace to be greater or equal to the
  length of the TextUnits valid text.
  A GxTextHunk may have many TextUnits, or it may have none.
*/
class GxTextHunk
{
public:
  GxTextHunk(void);
  ~GxTextHunk(void);

  //deletes all the text in this GxTextHunk
  void Clear(void);

  //hack? should we interit from GxDLList instead
  class TextUnit
  {
  public:
    TextUnit(void);
    TextUnit(const TextUnit &rTUnit);
    ~TextUnit(void);
    
    char text[GXTWUNIT]; //never null terminated
    unsigned int length; //end char is absolute max is GXTWUNIT
  };

  //this may become invalid after the TextHunk is edited
  class TextPlace
  {
  public:
    TextPlace(const std::list<TextUnit>::iterator &rlPlace, unsigned int tPlace);
    TextPlace(const TextPlace &rPlace);
    TextPlace(void);
    ~TextPlace(void);
    const TextPlace & operator=(const TextPlace &rPlace);
    friend bool operator==(const TextPlace &lhs, const TextPlace &rhs);
    //should this have an increment operator

    bool Valid(void) const;
    void Invalidate(void);

    bool valid;
    std::list<TextUnit>::iterator listPlace;
    unsigned int subPlace;
  };

  //at end of buffer or after rPlace
  void AppendText(const char *pText, unsigned numChars);
  void AppendText(TextPlace &rPlace, const char *pText, unsigned numChars);
  //at begining of buffer or before rPlace
  void PrependText(const char *pText, unsigned numChars);
  void PrependText(TextPlace &rPlace, const char *pText, unsigned numChars);

  //this will increment TextPlace to the next location if it can.
  //if it cannot increment no change is made.
  bool IncrementTextPlace(TextPlace &rPlace);
  //this will decrement TextPlace to the previous location if it can.
  //if it cannot decrement no change is made.
  bool DecrementTextPlace(TextPlace &rPlace);

  void GetPlaceSpaced(const TextPlace &rCPlace, int spacing,
		      TextPlace &rNewPlace);

  TextPlace GetBeginPlace(void);
  TextPlace GetEndPlace(void); //end of the entire buffer

  //after this should rStartPlace == rEndPlace
  void DeleteTextBetween(TextPlace &rStartPlace,
			 TextPlace &rEndPlace);
  void GetTextBetween(const TextPlace &rStartPlace,
		      const TextPlace &rEndPlace,
		      std::string &rText);

  const GxTextHunk::TextUnit &GetTextUnit(const TextPlace &rPlace);
  char* GetCharPointer(const TextPlace &rPlace);
  char GetChar(const TextPlace &rPlace) const;
  bool Empty(void); /*no content */
  bool IsCharPlace(const TextPlace &rPlace) const; /* ?hack? */

  bool BothPlacesInSameTextUnit(const TextPlace &rPlaceOne,
				const TextPlace &rPlaceTwo) const;
#ifdef LIBGX_DEBUG_BUILD
  //the following are friend functions because if they were member functions the
  //class layout would change between debug and non-debug builds
  //this validates whether or not a place is valid acording to the very
  //complex rules needed by this class.  This is too expensive to compile
  //into the non-debug library.
  //if rPlace is allowed to be the last non-derefrenceable position and still valid
  friend bool PlaceValid(const GxTextHunk &rHunk, const TextPlace &rPlace);
  friend bool Valid(const GxTextHunk &rHunk); //verifies that the given GxTextHunk's state is valid
#endif //LIBGX_DEBUG_BUILD
  bool InFirstUnit(const TextPlace &rPlace) const;
  bool InLastUnit(const TextPlace &rPlace) const;
  bool Dereferenceable(const TextPlace &rPlace) const;
  bool Equal(const TextPlace &rPlaceOne, const TextPlace &rPlaceTwo) const;
  void GetText(std::ostream &rOut) const;
  void SetText(std::istream &rIn);
 
  //the GxDLList uses the copy constructor of the TextUnit(s) which are pretty
  //simple so this should be pretty fast. must check for successful
  //insert/delete
  std::list<TextUnit> textList;
protected:
  //rPlace is reset to the end of the region removed, or to the start of
  //the region removed if the TextUnit is the last in textList.
  bool RemoveTextUnit(TextPlace &rPlace);

};

/* as a necessary concession for speed, the implementation of GxTextWin is
   nightmarishly complex. Basicly Format()ing the text builds a display list
   in the form of GxTextWin::Line's. Each line in turn consists of
   DisplayChunks. Display chunks consist of a character * for the start of
   the string to display and the number of characters to display, as well
   as the starting x value of the text to be rendered at.
*/

class GxTextWin : public GxAppScrolledWin, protected GxKeyHandler
{
public:
  GxTextWin(GxRealOwner *pOwner);
  virtual ~GxTextWin(void);

  //hack? I'm not sure if scrollEnd is good or not. it is not efficient.
  void SetText(const char *pText, bool scrollEnd = false);
  void GetText(std::string &rText) const;
  //this invalidates all internal state relating to the windowText
  //and returns a reference to the textHunk. for the window to be used
  //this must be Released. this lets the user parse/edit the text without
  //making a copy. rCPlace is set to the cursor place. if the cursor place
  //is not set, rCPlace is set to !Valid()
  GxTextHunk& GrabTextHunk(void);
  GxTextHunk& GrabTextHunk(GxTextHunk::TextPlace &rCursorPlace, unsigned &rCurrentTopLine);
  //reformats and displays. ok to call without grabbing first.
  void ReleaseTextHunk(void); //sets cursor to top place and results window viewing top left of text
  void ReleaseTextHunk(GxTextHunk::TextPlace &rCursorPlace, unsigned lastCurrentTopLine);

  void Wrap(bool wrapStat);
  bool Wrap(void) const;

  void Editable(bool editStat);
  bool Editable(void) const;

  //void Spacing(int tSpacing);
  //bool Spacing(void) const;

  //called whenever the user modifies text in the window.
  //hack. not hooked up.
  CbVoidFO modifyCB;

  virtual bool AcceptFocus(Time eventTime);
  virtual void Resize(UINT tWidth, UINT tHeight);
  virtual void Place(int &lX, int &rX, int &tY, int &bY);
  virtual void Create(void);
protected:
  class DrawInfo
  {
  public:
    DrawInfo(GxDisplayInfo &rDInfo, GxVolatileData &rVData);
    ~DrawInfo(void);

    GxDisplayInfo &dInfo;
    GxVolatileData &vData;
    Window xWin;
  };

  class DisplayChunk
  {
  public:
    DisplayChunk(const GxTextHunk::TextPlace &rPlace, int xPixStart);
    DisplayChunk(const GxTextWin::DisplayChunk &rChunk);
    ~DisplayChunk(void);

    void Display(DrawInfo &lInfo, int xOffset, int lineYPixel) const;
    //offsets are relative to the startPlace
    void Display(DrawInfo &lInfo, int xOffset, int lineYPixel,
		 int startOffset, int endOffset) const;
    //for back lookups
    //the place of the first char in this DisplayChunk
    GxTextHunk::TextPlace startPlace;

    //this was broken. set one past end of chunk-> might not be valid.
    //void GetEndPlace(GxTextHunk::TextPlace &rEndPlace); //must compute end Place
    inline const char* StartPtr(void) const
    {
      const char *pStart = (*(startPlace.listPlace)).text + startPlace.subPlace;
      return pStart;
    }
    //this is the number of characters actually displayed. there might be more characters than
    //this considered to be on the line. (like a trailing \n)
    unsigned numChars; //might be 0 if the only char is a '\n'

    //set assuming the line starts at 0. is Positive. used to calculate
    //horizontal position within the DisplayChunk.
    //?hack? Should this be stored as a ?unsigned? width of this DisplayChunk, and the xStartPixel
    //be passed in in all functions?
    int xPixel;
  };

  class Line
  {
  public:
    Line(void);
    ~Line(void);

    void Display(DrawInfo &lInfo, int xOffset, int lineYPix) const;
    void ClearHighlight(void);
    DisplayChunk& GetLastChunk(void); //hack? how do I verify the validity of the reference?
    
    //we will be using a copy constructor here. slowish but much less hairy
    //than handling DisplayChunk*'s (for now);
    //a lines's dcList may not be empty.
    std::list<DisplayChunk> dcList;
    //this is highlight start and highlight end.
    //if hlStart is Valid() hlEnd is assumed to be valid.
    GxTextHunk::TextPlace hlStart, hlEnd;

    //it is tacky adding this to every line, perhaps a better implementation would be to have a composite
    //iterator that held the line number and its place in the line list.
    unsigned lineNum; //is zero for the first line.
    unsigned lineLength; //used in cursor position lookups. last visible char position.
  };

  class CCoord
  {
  public:
    CCoord(void);
    CCoord(int tX, int tY);
    CCoord(int tX, int tY, Time eventTime);
    CCoord(const CCoord &rhs);
    ~CCoord(void);

    const CCoord & operator=(const CCoord &rhs);

    int xC, yC;
    Time time;
  };

  class DisplayWindow : public GxWin
  {
  public:
    DisplayWindow(GxRealOwner *pOwner);
    virtual ~DisplayWindow(void);

    void ClearAndDisplay(void);
    void DrawBorder(void);

    virtual void Create(void);
    virtual void HandleEvent(const XEvent &rEvent);

    //public so the GxTextWin can scoll this
    //the width of the window which can be drawn into
    unsigned GetTextAreaWidth(void) const;
    unsigned GetTextAreaHeight(void) const;

    GxCursor cursor;

    bool freezeEvents;

    CbOneFO<const XKeyEvent&> keyEventCB;
    CbVoidFO exposeCB;
    //the int is 1 for #1 press; 2 for #1 release; 3 for #2 press; 4 for #2 release 
    CbTwoFO<const CCoord&, int> clickCB;
    CbOneFO<const CCoord&> motionCB;
    CbOneFO<bool> focusCB;
    DrawInfo lInfo;
  protected:
    virtual void GetWindowData(XSetWindowAttributes &winAttrib,
			       ULINT &valueMask);
  };

  virtual void HScrollCallback(GxFraction rHScrollFr);
  virtual void VScrollCallback(GxFraction rVScrollFr);
  virtual void ScrollLeft(void);
  virtual void ScrollRight(void);
  virtual void ScrollUp(void);
  virtual void ScrollDown(void);
  
  // ****************** start GxKeyHandler overloads ******************
  virtual void HandleKeyString(const char *pBuffer, unsigned long buffLen);
  virtual void KeyLeft(void);
  virtual void KeyRight(void);
  virtual void KeyUp(void);
  virtual void KeyDown(void);
  virtual void KeyDelete(void);
  virtual void KeyBackspace(void);
  virtual void KeyEnter(void);
  // ****************** end GxKeyHandler overloads ******************

  //for key up/down/left/right makes the cursor (presumed repositioned) visible, redraws the window if needed
  void PosKeyUtility(void);

  void DrawDisplayWin(void);
  void HandleClick(const CCoord &rClickCoord, int type);
  void HandleClickDragMotion(const CCoord &rMotionCoord);
  void HandleFocusChange(bool focusStat);

  //creates the display list, and places the cursor. uses the below two functions.
  //viewCursor == make the cursor visible, if false view the top line.
  //oldTopLineNum is used in viewing the cursor it is used only if viewCursor and no lines exist.
  void Format(bool viewCursor = false, unsigned oldTopLineNum = 0);

  //these two return the actual line length in pixels (for setting scrollbars)
  unsigned FormatWrappedLine(GxTextWin::Line *pCLine,
			     const GxTextHunk::TextPlace &lineStart,
			     GxTextHunk::TextPlace &nextLineStart);
  unsigned FormatUnWrappedLine(GxTextWin::Line *pCLine,
			       const GxTextHunk::TextPlace &lineStart,
			       GxTextHunk::TextPlace &nextLineStart);

  void MakeCursorVisible(unsigned oldTopLine); //sets selectStartTextPlace to something valid no matter what
  void SetScrollBarFractions(void); //after repositioning the cursor and start lines this must be called.

  //this uses the data in the lineList
  bool SetCursorPixPos(void);
  //the line pos is irrespictive of the lineStartXPixel, it is just the sum of the line character widths before the
  //cursor. this does not include the lineStartXPixel offset. pure character spacings from 0.
  void GetCursorInfo(std::list<Line*>::iterator &rCursorLinePos, unsigned &rCursorLineHPixPos);
  bool FindCursorPlaceInLineText(Line *pLine, unsigned &rCursorLineHPixPos) const;
  //given a line and a cursor x position _in pixels_ (not offsets), this returns the _best_ cursor position
  void GetBestCursorPlace(Line *pLine, int xPos, GxTextHunk::TextPlace &rPlace);

  //given a y value this finds the best 'displayed' line (note the line must be on the screen)
  //this could be const except it returns a non-const iterator
  void GetLinePlace(int yPix, std::list<Line*>::iterator &rLinePlace);

  //the width in pixels in the current font of the given character
  short GetCharWidth(unsigned cChar) const; //unsigned for future 16bit fonts

  //called everytime the contents of the textHunk are going to be modified. leaves the class in an invalid state.
  void ClearLines(void);

  void SetTextGrabbed(bool newStat); //talks to the display window too.

  bool textGrabbed;
  bool editable;
  bool wrap;
  bool haveFocus;
  UINT lineHeight; //calculated in constructor

  int lineStartXPixel; //negative ok. set by Format and HScrollCallback
  UINT longestLine;   //must be determined by format.  This is the longest line in all of the text. in pixel coordinates.
  std::list<Line*>::iterator startingDrawnLinePlace; //the first line drawn in the window. set by Format and VScrollCallback

  // this is the info used in text selection/highlighting
  bool textSelected;
  std::list<Line*>::iterator selectStartLineListPlace;
  GxTextHunk::TextPlace selectStartTextPlace; //better be on the above line.
  //int selectStartLineNum;
  // end text selection/highlighting variables

  GxTextHunk windowText;
  GxTextHunk::TextPlace cursorPos; //as long as we have a valid windowText this is valid too

  std::list<Line*> lineList; //the line list should never be empty (unless the windowText has been grabbed)
#ifdef LIBGX_DEBUG_BUILD
  friend void CheckValid(GxTextWin &rWin); //verifies that all of this classes state is consistant and valid. asserts on error
  friend void CheckLinePlaceValid(GxTextWin &rWin, std::list<Line*>::iterator desPlace, bool endOk); //same as above
#endif //LIBGX_DEBUG_BUILD

  DisplayWindow dispWin;
  //this is just here for convience. we are a ghost, so don't have one by default.
  GxDisplayInfo &dInfo;
};

#endif //GXTEXTWIN_INCLUDED
