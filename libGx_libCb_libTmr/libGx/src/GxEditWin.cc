#include <list>
#include <iostream>

#include <libGx/GxDisplay.hh>
#include <libGx/GxMainInterface.hh>

#include <libGx/GxEditWin.hh>

#include "GxDefines.hh"

using namespace std;

GxEditWin::GxEditWin(GxRealOwner *pOwner) :
  GxWin(pOwner),
  pushedHandlerID(NULL_EVENT_HANDLER_ID),
  asPos(AS_INSIDE), inhibitAS(true),
  asTimer(10, CbVoidMember<GxEditWin>(this, &GxEditWin::AutoScrollCB) ),
  dc_c1_time(0), dc_state(DC_P1),
  haveFocus(false),
  numCharsVisible(25),
  editable(true),
  active(true),
  currentSize(0),
  startPlace(0),
  cPosition(0)
{
  selectedStart = 0;
  numSelected = 0;

  height = 20;
  //we want fonts to be vertically centered in my window
  textY = height - ((height - (dInfo.pDefaultFont->ascent +
			       dInfo.pDefaultFont->descent))/2)
    - dInfo.pDefaultFont->descent;
  text[0] = '\0';
}

GxEditWin::~GxEditWin(void)
{}

void GxEditWin::SetNumVisibleChars(UINT newNum)
{
  numCharsVisible = newNum;
}

const char* GxEditWin::GetText(void)
{
  //we do not maintain the string as being null terminaged, but
  //some functions may require it, so we do it now.
  text[currentSize] = '\0';
  return text;
}

void GxEditWin::SetText(const char* pNewText)
{
  GxSetLabel(text, GX_EDIT_WIN_SIZE, pNewText, currentSize);
  cPosition = currentSize; //might be 0.
  startPlace = 0;

  if(Created())
    {
      ClearTextArea();
      Draw();
    };
}

void GxEditWin::SetText(const char* pNewText, int numChars)
{
  string newString;
  if(pNewText)
    newString = string(pNewText, numChars);

  GxSetLabel(text, GX_EDIT_WIN_SIZE, newString.c_str(), currentSize);
  cPosition = currentSize; //might be 0.
  startPlace = 0;

  if(Created())
    {
      ClearTextArea();
      Draw();
    };
}

void GxEditWin::SetEditable(bool editStatus)
{
  editable = editStatus;
}

void GxEditWin::SetActive(bool tActiveState)
{
  bool oldActive = active;
  active = tActiveState;
  if(active != oldActive)
    {
      ClearTextArea();
      Draw();
    };
}

bool GxEditWin::AcceptFocus(Time eventTime)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxEditWin::AcceptFocus" << endl;
#endif //LIBGX_DEBUG_BUILD

  if(!Created() || !editable)
    return false;
  else
    {
#ifdef LIBGX_DEBUG_BUILD
      cout << "GxEditWin::AcceptFocus made XSetInputFocus call with time: " << eventTime << endl;
#endif //LIBGX_DEBUG_BUILD
      XSetInputFocus(dInfo.display, xWin, RevertToParent, eventTime);
      return true;
    };
}

void GxEditWin::HandleEvent(const XEvent &rEvent)
{
  bool doDraw = false;
  int motionPlace = 0;
  switch(rEvent.type)
    {
    case Expose:
      Draw();
      break;
    case KeyPress:
      HandleKeyEvent(rEvent.xkey);
      break;
    case ButtonPress:
      if( rEvent.xbutton.button == 2)
	{
	  b2Pressed = true;
	  dInfo.rGxDisplay.PushEventHandler( CbOneMember<const XEvent&, GxEditWin>
					     (this, &GxEditWin::HandleGrabbedEvents),
					     pushedHandlerID );
	  ClearSelection();
	  Draw();
	  break;
	};

      if( rEvent.xbutton.button == 1)
	{
	  if(dc_state == DC_R1)
	    {
	      if( rEvent.xbutton.time > dc_c1_time + GX_DC_TIME ||
		  rEvent.xbutton.time < dc_c1_time ) //rollover
		{
		  dc_state = DC_P1;
		  dc_c1_time = rEvent.xbutton.time;
		  //cout << "resetting dc to p1" << endl;
		}else
		{
		  dc_state = DC_P2;
		  //cout << "moving dc to state p2" << endl;
		};
	    }else
	    {
	      //cout << "starting dc at p1" << endl;
	      dc_state = DC_P1;
	      dc_c1_time = rEvent.xbutton.time;
	    };
	  AcceptFocus(rEvent.xbutton.time);
	  dInfo.rMainInterface.ActivateTimer(asTimer);
	  inhibitAS = false;
	  asPos = AS_INSIDE; //logically true (hack? Event via XSendEvent?)
	  //hack; shouldn't this be part of AcceptFocus?
	  if(editable)
	    pWinAreaOwner->MoveFocusToChild(this, rEvent.xbutton.time); 
	  pressPlace = LookUpPlace(rEvent.xbutton.x);
	  if(pressPlace != cPosition)
	    {
	      DrawEraseCursor(false);
	      cPosition = pressPlace;
	      doDraw = true;
	    };
	  if(numSelected)
	    {
	      ClearSelection();
	      doDraw = true;
	    };
	  if(doDraw)
	    Draw();
	};
      break;
    case FocusIn:
#ifdef LIBGX_DEBUG_BUILD
      cout << "GxEditWin got FocusIn event" << endl;
#endif //LIBGX_DEBUG_BUILD
      if(haveFocus || !editable) break;
      haveFocus = true;
      Draw();
      break;
    case FocusOut:
#ifdef LIBGX_DEBUG_BUILD
      cout << "GxEditWin got FocusOut event" << endl;
#endif //LIBGX_DEBUG_BUILD
      //we should not have children, but handle it gracefully
      if(!haveFocus) break;
      haveFocus = false;
      ClearTextArea();
      Draw();
      break;
    case ButtonRelease:
      if( rEvent.xbutton.button == 1)
	{
	  asTimer.DeActivate();
	  inhibitAS = true;
	  if(dc_state == DC_P2)
	    {
	      if( rEvent.xbutton.time > dc_c1_time + GX_DC_TIME ||
		  rEvent.xbutton.time < dc_c1_time ) //rollover
		{
		  //cout << "resetting state to P1 on an timed out release" << endl;
		  dc_state = DC_P1;
		}else
		{
		  //cout << "doing double click action!!" << endl;
		  selectedStart = 0;
		  numSelected = currentSize;
		  if(numSelected)
		    dInfo.rGxDisplay.PushCutText( string(&text[selectedStart] , numSelected) );
		  ClearTextArea();
		  Draw();
		  dc_state = DC_R1;
		};
	    }else
	    {
	      if(numSelected)
		dInfo.rGxDisplay.PushCutText( string(&text[selectedStart] , numSelected) );
	      //cout << "setting state to R1 after assuming state == DC_P1" << endl;
	      dc_state = DC_R1;
	    };
	};
      break;
    case MotionNotify:
      if( rEvent.xmotion.x < 0 )
	asPos = AS_LEFT;
      else
	if( rEvent.xmotion.x > width )
	  asPos = AS_RIGHT;
	else
	  {
	    asPos = AS_INSIDE;
	    motionPlace = LookUpPlace(rEvent.xmotion.x);
	    AdjustSelection(motionPlace);
	    cPosition = motionPlace;
	    ClearTextArea();
	    Draw();
	  };
      break;
    default:
      break;
    };
}

void GxEditWin::HandleGrabbedEvents(const XEvent &rEvent)
{
  int availChars = GX_EDIT_WIN_SIZE - currentSize;
  string pasteBuffer;
  switch(rEvent.type)
    {
    case Expose:
      dInfo.rGxDisplay.HandleSafeLoopEvent(rEvent);
      break;
    case LeaveNotify:
      if(rEvent.xcrossing.window == xWin )
	b2Pressed = false;
      break;
    case EnterNotify:
      if(rEvent.xcrossing.window == xWin )
	b2Pressed = true;
      break;
    case ButtonRelease:
      if(rEvent.xbutton.button != 2) break;
      //if we are here the correct button was released
      //no matter what else we remove this event handler
      dInfo.rGxDisplay.RemoveEventHandler(pushedHandlerID);
      pushedHandlerID = NULL_EVENT_HANDLER_ID;
      if(b2Pressed)
	{
	  dInfo.rGxDisplay.PullCutText(pasteBuffer, availChars);
	  //set the cursor position
	  cPosition = LookUpPlace(rEvent.xbutton.x);
	  HandleKeyString(pasteBuffer.c_str(), pasteBuffer.size() );
	};
    default: 
      break;
    };
}

UINT GxEditWin::GetDesiredWidth(void) const
{
  return (UINT)numCharsVisible*(dInfo.pDefaultFont->max_bounds.width) + 6;
}

UINT GxEditWin::GetDesiredHeight(void) const
{
  return (UINT)(dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent +
		2*GX_BORDER_WD + 4);
}

void GxEditWin::GetWindowData(XSetWindowAttributes &winAttributes,
			      ULINT &valueMask)
{
  winAttributes.background_pixel = dInfo.whitePix;
  winAttributes.event_mask = KeyPressMask | ExposureMask | ButtonPressMask |
    ButtonReleaseMask | Button1MotionMask | FocusChangeMask | EnterWindowMask | LeaveWindowMask;
  valueMask |= CWBackPixel | CWEventMask;
}

void GxEditWin::AutoScrollCB(void)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxEditWin::AutoScrollCB: " << asPos << endl;
#endif //LIBGX_DEBUG_BUILD

  int endPlace = 0;

  switch(asPos)
    {
    case AS_LEFT:
      if( startPlace > 0)
	{
	  startPlace--;
	  AdjustSelection(startPlace);
	  cPosition = startPlace;
	  ClearTextArea();
	  Draw();
	};
      break;
    case AS_RIGHT:
      endPlace = LookUpPlace(width - GX_BORDER_WD);
      if( endPlace < currentSize )
	{
	  startPlace++;
	  cPosition = endPlace;
	  AdjustSelection(endPlace);
	  ClearTextArea();
	  Draw();
	};
      break;
    default:
      break;
    };

  if( !inhibitAS )
    dInfo.rMainInterface.ActivateTimer(asTimer);
}

void GxEditWin::Draw(void)
{
  if( !Created() ) return;

  if( AdjustStartPlace() ) //hack? seems to be a bad place to do this.
    ClearTextArea();

  int localNumSelected = numSelected;

#ifdef LIBGX_DEBUG_BUILD
  if(selectedStart + localNumSelected > currentSize)
    {
      cerr << "error debug fixup of fragment size" << endl;
      localNumSelected = currentSize - selectedStart;
      if(localNumSelected < 0)
	localNumSelected = 0;

      cerr << "fixup: numSelected was: " << numSelected << " localNumSelected: " << localNumSelected << endl;
    };
#endif //LIBGX_DEBUG_BUILD

  //first create the fragement list.

  list<DrawFragment> dfList;
  DrawFragment dFragment;  //we will reuse the dFragment on the stack as a temporary

  dFragment.startPlace = 0;
  dFragment.numChars = currentSize;

  if(localNumSelected)
    {
      if(selectedStart == 0) //the first part of the text is highlighted
	{
	  dFragment.highlighted = true;
	  dFragment.numChars = localNumSelected;
	  dfList.push_back(dFragment);

	  //add the unhightlighed fragment now if it exists
	  if(currentSize > localNumSelected)
	    {
	      dFragment.Reset();
	      dFragment.startPlace = localNumSelected;
	      dFragment.numChars = currentSize - localNumSelected;
	      dfList.push_back(dFragment);
	    };
	}else
	  {
	    //the first part of the string is not highlighted then we have a highlighed section
	    //that may continue to the end of the string
	    dFragment.numChars = selectedStart;
	    dfList.push_back(dFragment);

	    //add the hightlighted fragment.
	    dFragment.Reset();
	    dFragment.startPlace = selectedStart;
	    dFragment.numChars = localNumSelected;
	    dFragment.highlighted = true;
	    dfList.push_back(dFragment);

	    if(selectedStart + localNumSelected < currentSize) //we have more text after the selected text
	      {
		dFragment.Reset();
		dFragment.startPlace = selectedStart + localNumSelected;
		dFragment.numChars = currentSize - (selectedStart + localNumSelected);
		dfList.push_back(dFragment);
	      };
	  };
    }else //nothing is selected.
      {
	dFragment.numChars = currentSize;
	dfList.push_back(dFragment);
      };

  //draw our fragment list.
  XSetFont(dInfo.display, vData.textGC, dInfo.pDefaultFont->fid);

  list<DrawFragment>::iterator cPlace = dfList.begin();
  list<DrawFragment>::iterator cEnd   = dfList.end();
  //  cout << "fragment list size is: " << dfList.size() << endl;

  int cPixPlace = GX_BORDER_WD;
  while(cPlace != cEnd)
    {
      if( startPlace > ( (*cPlace).startPlace + (*cPlace).numChars) )
	{
	  cPlace++;
	  continue;
	};
      //if here we know we will draw _something_ from this fragment
      int localStartPlace = (*cPlace).startPlace;
      if( startPlace > (*cPlace).startPlace)
	localStartPlace = startPlace;
      int localNumChars = (*cPlace).numChars - (localStartPlace-(*cPlace).startPlace);
      //cout << "fragment numChars: " << (*cPlace).numChars << " localNumChars: " << localNumChars << endl;

      //hack? we could only compute this if we know we are going to draw the next fragment.
      unsigned tWidth = XTextWidth(dInfo.pDefaultFont, &text[localStartPlace], localNumChars);
      if( (*cPlace).highlighted )
	{
	  //cout << "drawing highlighted fragment" << endl;
	  //draw the highlight rectangle color
	  XSetForeground(dInfo.display, vData.textGC, dInfo.selectedTextBackgroundPix);
	  XFillRectangle(dInfo.display, xWin, vData.textGC, cPixPlace, textY-dInfo.pDefaultFont->ascent,
			 tWidth, dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent);
	  //setup to draw the highlighted text;
	  XSetForeground(dInfo.display, vData.textGC, dInfo.selectedTextPix);
	}else
	  {
	    //cout << "drawing normal fragment" << endl;
	    if(active)
	      XSetForeground(dInfo.display, vData.textGC, dInfo.blackPix);
	    else
	      XSetForeground(dInfo.display, vData.textGC, dInfo.darkBorderPix); //darkborderPix ->hack
	  };

      XDrawString(dInfo.display, xWin, vData.textGC, cPixPlace, textY, &text[localStartPlace], localNumChars);

      cPixPlace += tWidth;
      if(cPixPlace >= width) break; //no point in drawing more text.
      cPlace++;
    };

  //must draw the border after the text; otherwise text would be drawn
  //over the border.
  Draw3dBorder(0,0, width,height, false);

  DrawEraseCursor(true);

  //restore text color ?hack?
  XSetForeground(dInfo.display, vData.textGC, dInfo.blackPix);

  if(haveFocus) //draw the border around the window to specify focus
    {
      XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
      XDrawRectangle(dInfo.display, xWin, vData.borderGC, 0,0,
		     width-1,height-1);
    };
}

void GxEditWin::DrawEraseCursor(bool draw)
{
  if(!editable || !haveFocus) return;

  //location of the cursor in pixels
  int cPixPosition = LookUpPixel(cPosition);

  //top and bottom bars are 5 pixels
  //drawn at (font->ascent) and (font->descent)
  //with a vertical line drawn from midpoint to midpoint

  if(draw)
    XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
  else
    XSetForeground(dInfo.display, vData.borderGC, dInfo.whitePix);

  //body of cursor
  XDrawLine(dInfo.display, xWin, vData.borderGC,
	    cPixPosition, textY + dInfo.pDefaultFont->descent,
	    cPixPosition, textY - dInfo.pDefaultFont->ascent);
  //top of cursor
  XDrawLine(dInfo.display, xWin, vData.borderGC,
	    cPixPosition - 2, textY - dInfo.pDefaultFont->ascent,
	    cPixPosition + 2, textY - dInfo.pDefaultFont->ascent);
  //bottom of Cursor
  XDrawLine(dInfo.display, xWin, vData.borderGC,
	    cPixPosition - 2, textY + dInfo.pDefaultFont->descent,
	    cPixPosition + 2, textY + dInfo.pDefaultFont->descent);
}

void GxEditWin::ClearTextArea(void)
{
  if( !Created() ) return;
  XClearArea(dInfo.display, xWin, GX_BORDER_WD, GX_BORDER_WD,
	     width - 2*GX_BORDER_WD, height - 2*GX_BORDER_WD, false);
}

// ************* start GxKeyHandler overloads ***************
void GxEditWin::HandleKeyString(const char *pBuffer, unsigned long buffLen)
{
  if(buffLen == 0) return;

  DeleteSelection();

  if(buffLen + currentSize > GX_EDIT_WIN_SIZE)
    {
      XBell(dInfo.display, 100);
#ifdef LIBGX_DEBUG_BUILD
      cerr << "in GxEditWin::HandleKeyEvent and we are out of space" << endl;
#endif //LIBGX_DEBUG_BUILD
      return;
    };
	  
  //copy buffLen char's in pBuffer to my buffer
  //we make absolutly sure that cPosition <= currentSize
  //because nothing else makes sense
  if(cPosition > currentSize) //should not happen
    cPosition = currentSize;
      
  if(cPosition == currentSize)
    {
      memcpy(&text[cPosition], pBuffer, buffLen);
      //we don't need need to erase the entire window if we're just adding
      //characters to the end of the string
      DrawEraseCursor(false);
    }else
      {
	//make space for copying text from pBuffer into text
	memmove(&text[cPosition+buffLen], &text[cPosition],
		currentSize-cPosition);
	//now copy from pBuffer into text
	memcpy(&text[cPosition], pBuffer, buffLen);
	//if we are adding text in the middle of the string we have
	//to erase the characters we are overwriting. The easiest way
	//to do this is to Clear the window. Of course no need to delete
	//the cursor if we're doing this
	XClearWindow(dInfo.display, xWin);
      };

  cPosition += buffLen;
  currentSize += buffLen;
  modifyCB();
  Draw();
}

void GxEditWin::MoveCursorLineStart(void)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxEditWin::MoveCursorLineStart" << endl;
#endif //LIBGX_DEBUG_BUILD

  ClearSelection();

  cPosition = 0;

  AdjustStartPlace();
  ClearTextArea();
  Draw();
}

void GxEditWin::MoveCursorLineEnd(void)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxEditWin::MoveCursorLineEnd" << endl;
#endif //LIBGX_DEBUG_BUILD

  ClearSelection();

  cPosition = currentSize;

  AdjustStartPlace();
  ClearTextArea();
  Draw();
}

void GxEditWin::KillTextToLineEnd(void)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxEditWin::KillTextToLineEnd" << endl;
#endif //LIBGX_DEBUG_BUILD

  ClearSelection();
  
  currentSize = cPosition;

  AdjustStartPlace();
  ClearTextArea();
  Draw();
  modifyCB();
}

void GxEditWin::KeyLeft(void)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxEditWin::KeyLeft" << endl;
#endif //LIBGX_DEBUG_BUILD
  if(cPosition > 0)
    {
      DrawEraseCursor(false);
      cPosition--;
      DrawEraseCursor(true);
      Draw(); //hack. needed because of ugly cursor handling
    };
}

void GxEditWin::KeyRight(void)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << " GxEditWin::KeyRight" << endl;
#endif //LIBGX_DEBUG_BUILD
  if(cPosition < currentSize)
    {
      DrawEraseCursor(false);
      cPosition++;
      DrawEraseCursor(true);
      Draw(); //hack. needed because of ugly cursor handling
    };
}

void GxEditWin::KeyDelete(void)
{
  //remove the character imediatly to the right of the cursor
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxEditWin::KeyDelete" << endl;
  assert(cPosition <= currentSize);
#endif //LIBGX_DEBUG_BUILD

  if(numSelected)
    {
      DeleteSelection();
      return;
    };
  
  //we should not ever have cPosition > currentSize
  if(cPosition == currentSize) return;

  //if there is only one char to the right of cPosition just shorten the
  //string length by one.
  if(cPosition + 1 == currentSize)
    {
      currentSize--;
      XClearWindow(dInfo.display, xWin);
      //this is probably not necessary
      modifyCB();
      Draw();
      return;
    };

  //if we're here we have chars on the right of the cursor to move.
  memmove(&text[cPosition], &text[cPosition+1], currentSize-cPosition-1);
  currentSize--;
  XClearWindow(dInfo.display, xWin);
  //this is probably not necessary
  modifyCB();
  Draw();
}

void GxEditWin::KeyBackspace(void)
{
  if(numSelected)
    {
      DeleteSelection();
      return;
    };

  //remove the character immediatly to the left of the cursor
  //if there is not a char to the right of the cursor, just shorten the
  //length by one. But be careful not to shorten the length to less than 0.
  if(cPosition == 0) return;

  if(cPosition == currentSize)
    {
      currentSize--;
      cPosition--;
      XClearWindow(dInfo.display, xWin);
      modifyCB();
      Draw();
      return;
    };

  //if we're here we have chars to the right of the cursor to move
  memmove(&text[cPosition-1], &text[cPosition], currentSize - cPosition);
  currentSize--;
  cPosition--;
  XClearWindow(dInfo.display, xWin);
  modifyCB();
  Draw();
}

void GxEditWin::KeyEnter(void)
{
  enterCB();
}
// ************* end GxKeyHandler overloads ***************

void GxEditWin::AdjustSelection(int eventStrPos)
{
  if(eventStrPos < pressPlace)
    {
      selectedStart = eventStrPos;
      numSelected = pressPlace-eventStrPos;
    }else
    {
      selectedStart = pressPlace;
      numSelected = eventStrPos-pressPlace;
    };
}

int GxEditWin::LookUpPlace(int xEv)
{
  //this should return start or end of the *displayed* string
  if(xEv <= GX_BORDER_WD) return startPlace;

  int totalX = GX_BORDER_WD; //the total widths of all chars untill ii;
  //the total width of all chars untill half way through the char before ii;
  int lastCharMid = GX_BORDER_WD;
  int ii = startPlace;
  while(ii < currentSize)
    {
      unsigned int charID = (unsigned int)text[ii];
      if( (charID < dInfo.pDefaultFont->min_char_or_byte2) ||
	  (charID > dInfo.pDefaultFont->max_char_or_byte2) )
	{
#ifdef LIBGX_DEBUG_BUILD
	  cout << "char does not exist in this font!" << endl;
#endif //LIBGX_DEBUG_BUILD
	  ii++;
	  continue; //don't lookup a char that does not exist
	};

      short int cWidth = (dInfo.pDefaultFont->per_char[charID]).width;
      if( (xEv > lastCharMid) && (xEv <= totalX + cWidth/2) ) 
	{
	  //this is the correct spot
	  return ii;
	}else
	  {
	    lastCharMid = totalX + cWidth/2;
	    totalX += cWidth;
	    if(totalX > (LINT)(width - GX_BORDER_WD) )
	      return ii;
	  };

      ii++;
    };
  //if we have not returned by now the end of the string is before the end
  //of the window and the user clicked to the right of the end.
  return ii;
}

int GxEditWin::LookUpPixel(int strPosition)
{
  if(strPosition < startPlace) return GX_BORDER_WD;
  
  int currentX = GX_BORDER_WD;
  int i = startPlace;
  while(i < currentSize)
    {
      if( i == strPosition )
	{
	  //this is the correct spot
	  return currentX;
	};

      unsigned int charID = (unsigned int)text[i];
      if( (charID < dInfo.pDefaultFont->min_char_or_byte2) ||
	  (charID > dInfo.pDefaultFont->max_char_or_byte2) )
	{
#ifdef LIBGX_DEBUG_BUILD
	  cout << "char does not exist in this font!" << endl;
#endif //LIBGX_DEBUG_BUILD
	}else
	  currentX += (dInfo.pDefaultFont->per_char[charID]).width;

      //      if(currentX > LINT(width - 3)
      
      i++;
    };

  return currentX;
}

bool GxEditWin::AdjustStartPlace(void)
{
  //if cPosition is in a location that puts it beyond the visable area
  //of the string, adjusts start place just enough to make the cursor visable.

  if(cPosition < startPlace)
    {
      startPlace = cPosition;
      return true;
    };

  //figure out how many characters can be displayed in this window given
  //our current width. the numCharsVisible in our header is an idealization.
  //hack; this calculation is too crude
  int numCharsVis =
    (width - 2*GX_BORDER_WD)/(dInfo.pDefaultFont->max_bounds.width);

  if(cPosition > startPlace+numCharsVis)
    {
      startPlace = cPosition - numCharsVis;
      return true;
    };

  return false;
}

void GxEditWin::ClearSelection(void)
{
  selectedStart = 0;
  numSelected = 0;
  ClearTextArea();
  Draw();
}

void GxEditWin::DeleteSelection(void)
{
  if(!numSelected) return;

  memmove( &(text[selectedStart]), &(text[selectedStart+numSelected]), numSelected);
  currentSize -= numSelected;
  cPosition = selectedStart;
  numSelected = 0;
  selectedStart = 0;
  ClearTextArea();
  Draw();
}

// ******************** DrawFragment ***********************

GxEditWin::DrawFragment::DrawFragment(void) :
  startPlace(0), numChars(0), highlighted(false)
{}

GxEditWin::DrawFragment::DrawFragment(const GxEditWin::DrawFragment &rhs) :
  startPlace(rhs.startPlace), numChars(rhs.numChars), highlighted(rhs.highlighted)
{}

GxEditWin::DrawFragment::~DrawFragment(void)
{}

const GxEditWin::DrawFragment& GxEditWin::DrawFragment::operator=(const GxEditWin::DrawFragment &rhs)
{
  startPlace = rhs.startPlace;
  numChars = rhs.numChars;
  highlighted = rhs.highlighted;

  return *this;
}

void GxEditWin::DrawFragment::Reset(void)
{
  startPlace = 0;
  numChars = 0;
  highlighted = false;
}

