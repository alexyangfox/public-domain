//#ifdef LIBGX_DEBUG_BUILD
#include <assert.h>
//#endif //LIBGX_DEBUG_BUILD

#include <libGx/GxTextWin.hh>

#include "GxDefines.hh"

using namespace std;

// ******************** start GxTextHunk ***************************

GxTextHunk::GxTextHunk(void)
{}

GxTextHunk::~GxTextHunk(void)
{}

void GxTextHunk::Clear(void)
{
  textList.clear();
}

void GxTextHunk::AppendText(const char *pText, unsigned numChars)
{
  TextPlace endPlace( GetEndPlace() );
  AppendText(endPlace, pText, numChars);
}

void GxTextHunk::AppendText(TextPlace &rPlace, const char *pText,
			    unsigned numChars)
{
  if(!pText || numChars == 0) return;

#ifdef LIBGX_DEBUG_BUILD
  assert( Valid(*this) );
  assert( PlaceValid(*this, rPlace) );
#endif //LIBGX_DEBUG_BUILD

  unsigned charsToCopy = numChars;
  unsigned charsCopied = 0;
  TextUnit overflowUnit;

  if( textList.end() != rPlace.listPlace )
    {
      //fill the current TextUnit if we can.
      TextUnit &rUnit = *(rPlace.listPlace);
      //possibleSpace is the number of locations we can make avaiable in rUnit
      //for putting characters in.
      unsigned possibleSpace = GXTWUNIT - rPlace.subPlace;
      //space needed is the number of characters we are going to put into
      //the possibleSpace.
      unsigned spaceNeeded =
	(charsToCopy < possibleSpace) ? charsToCopy : possibleSpace;
      unsigned availableSpace = GXTWUNIT - rUnit.length;
      unsigned charsToMove = rUnit.length - rPlace.subPlace;

      unsigned charsToMoveToOverflow = 0;
      if(spaceNeeded > availableSpace)
	charsToMoveToOverflow = spaceNeeded - availableSpace;

      unsigned charsToMoveToEnd = charsToMove - charsToMoveToOverflow;

      if(charsToMoveToOverflow)
	{
	  memcpy(overflowUnit.text,
		 rUnit.text + rUnit.length - charsToMoveToOverflow,
		 charsToMoveToOverflow);
	  overflowUnit.length = charsToMoveToOverflow;
	};

      if(charsToMoveToEnd)
	{
	  memmove(rUnit.text+rPlace.subPlace+spaceNeeded,
		  rUnit.text+rPlace.subPlace,
		  charsToMoveToEnd);
	};

      memcpy(rUnit.text+rPlace.subPlace, pText, spaceNeeded);
      
      rUnit.length += (spaceNeeded-charsToMoveToOverflow);
      charsToCopy -= spaceNeeded;
      charsCopied += spaceNeeded;
      //update rPlace. if we just added enough chars to completly fill
      //the textUnit
      //this may now be incremented to GXTWUNIT (making rPlace invalid)
      rPlace.subPlace += spaceNeeded;

      if(charsCopied == numChars && overflowUnit.length == 0)
	{
	  if(rPlace.subPlace == GXTWUNIT)
	    {
	      rPlace.listPlace++;
	      rPlace.subPlace = 0;
	    };
	  return;
	};

      if(charsCopied == numChars)
	{
	  //insert the overflow _after_ this text unit, but keep
	  //rPlace the same. _unless_ rPlace is now invalid
	  list<TextUnit>::iterator cPlace = rPlace.listPlace;
	  cPlace++;
	  textList.insert(cPlace, overflowUnit);
	  if(rPlace.subPlace == GXTWUNIT)
	    {
	      rPlace.listPlace++;
	      rPlace.subPlace = 0;
	    };
	  return;
	};

      //we know we are going to be adding a new TextUnit which will be added
      //_before_ listPlace -> 
      if( InLastUnit(rPlace) )
	{
	  rPlace.listPlace++;
	  rPlace.subPlace = 0;
	}else
	  {
	    rPlace.listPlace++;
	    rPlace.subPlace = 0;
	  };
    };

  //this must be optimized to fill the last TextUnit in our list.
  while( charsToCopy )
    {
      TextUnit newUnit;
      unsigned numToCopy = (GXTWUNIT > charsToCopy) ? charsToCopy : GXTWUNIT;
      memcpy(newUnit.text, pText+charsCopied, numToCopy);
      newUnit.length = numToCopy;

      charsToCopy -= numToCopy;
      charsCopied += numToCopy;

      textList.insert(rPlace.listPlace, newUnit);
    };

  //a bit of a hack. if an invalid TextPlace is passed in it is treated
  //as the end place. therefore after we add something to the presumably
  //empty text list, we reset rPlace to be the end of the list
  if( !rPlace.Valid() )
    {
      rPlace = GetEndPlace();
    };

  //add the overflowUnit contents after rPlace. do not update rPlace
  //InsertAfter(rPlace, overflowUnit);
  if(overflowUnit.length)
    {
      textList.insert(rPlace.listPlace, overflowUnit);
      rPlace.listPlace--; //we know this is safe from the definition of insert
      rPlace.subPlace = 0;
    };
}


void GxTextHunk::PrependText(const char *pText, unsigned numChars)
{
  TextPlace startPlace( GetBeginPlace() );
  PrependText(startPlace, pText, numChars);
}

void GxTextHunk::PrependText(TextPlace &rPlace, const char *pText,
			     unsigned numChars)
{
  if(!pText || numChars == 0) return;

#ifdef LIBGX_DEBUG_BUILD
  assert( Valid(*this) );
  assert( PlaceValid(*this, rPlace) );

  cerr << "ERROR; GxTextHunk::PrependText is unimplemted" << endl;
#endif //LIBGX_DEBUG_BUILD
}

bool GxTextHunk::IncrementTextPlace(TextPlace &rPlace)
{
#ifdef LIBGX_DEBUG_BUILD
  assert( Valid(*this) );
  assert( PlaceValid(*this, rPlace) );
#endif //LIBGX_DEBUG_BUILD

  list<TextUnit>::const_iterator endPlace = textList.end();
  if( endPlace == rPlace.listPlace ) return false;

  const TextUnit &rUnit = (*rPlace.listPlace);

  if(rPlace.subPlace < (rUnit.length-1))
    {
      rPlace.subPlace++;
    }else //move to next textunit (if it exits)
      {
	rPlace.listPlace++;
	rPlace.subPlace = 0;
      };

  return true;
}

bool GxTextHunk::DecrementTextPlace(TextPlace &rPlace)
{
#ifdef LIBGX_DEBUG_BUILD
  assert( Valid(*this) );
  assert( PlaceValid(*this, rPlace) );
#endif //LIBGX_DEBUG_BUILD

  if(rPlace.subPlace == 0)
    {
      list<TextUnit>::const_iterator beginPlace = textList.begin();
      if( beginPlace == rPlace.listPlace ) return false;
      rPlace.listPlace--;
#ifdef LIBGX_DEBUG_BUILD
      assert( (*rPlace.listPlace).length );
#endif //LIBGX_DEBUG_BUILD
      rPlace.subPlace = (*rPlace.listPlace).length -1;
      return true;
    }else
      {
	rPlace.subPlace--;
	return true;
      };
}

void GxTextHunk::GetPlaceSpaced(const TextPlace &rCPlace, int spacing,
				TextPlace &rNewPlace)
{
#ifdef LIBGX_DEBUG_BUILD
  assert( Valid(*this) );
  assert( PlaceValid(*this, rCPlace) );
#endif //LIBGX_DEBUG_BUILD

  int currentSpacing = 0;
  TextPlace currentPlace(rCPlace);

  while( currentSpacing != spacing )
    {
      if(spacing > 0)
	{
	  if( textList.end() == rCPlace.listPlace)
	    {
	      //we are already at the end. can't increment. we lie here.
	      currentSpacing = spacing;
	      continue;
	    };
	  //we would die without the above check.
	  const TextUnit &rCUnit = *(currentPlace.listPlace);

	  //spacing is positive and currentSpacing is incremented to spacing
	  //so the cast to unsigned is safe
	  unsigned spacesRemaining = (unsigned)(spacing - currentSpacing);
	  if( spacesRemaining >= (rCUnit.length - rCPlace.subPlace) )
	    { //we can move to the next unit we check if the next unit
	      //really exists the next time around
	      (currentPlace.listPlace)++;
	      currentPlace.subPlace = 0;
	      currentSpacing += (rCUnit.length - rCPlace.subPlace);
	    }else //we end on the current unit
	      {
		currentPlace.subPlace += spacesRemaining;
		currentSpacing += spacesRemaining;
	      };
	}else
	  {
	    //spacing is negative and currentSpacing starts at zero and gets
	    //more negative until it is == to spacing. the cast is therefore
	    //safe
	    unsigned spacesRemaining = (unsigned)(currentSpacing - spacing);
	    if( spacesRemaining > rCPlace.subPlace )
	      { //we can move to the next unit
		//if the next unit exists
		if( InFirstUnit(currentPlace) )
		  {
		    //set the current place to be the start of the current
		    //unit and lie by setting currentSpacing to be spacing so
		    //we break from this loop
		    currentPlace.subPlace = 0;
		    //the big lie
		    currentSpacing = spacing;
		  }else
		    {
		      (currentPlace.listPlace)--;
		      TextUnit &rLUnit = *(currentPlace.listPlace);
		      currentPlace.subPlace = rLUnit.length-1;
		      //-1 because we went to the previous TextUnit
		      currentSpacing -= (rCPlace.subPlace + 1);
		    };
	      }else //we end on the current unit
		{
		  currentPlace.subPlace -= spacesRemaining;
		  currentSpacing -= spacesRemaining;
		};
	  };
    };

  rNewPlace = currentPlace;
}

GxTextHunk::TextPlace GxTextHunk::GetBeginPlace(void)
{
  TextPlace cPlace;
  
  cPlace.listPlace = textList.begin();
  cPlace.subPlace = 0;
  cPlace.valid = true;

  return cPlace;
}

GxTextHunk::TextPlace GxTextHunk::GetEndPlace(void)
{
  TextPlace cPlace;

  cPlace.valid = true;
  cPlace.listPlace = textList.end();
  cPlace.subPlace = 0;

  return cPlace;
}

void GxTextHunk::DeleteTextBetween(TextPlace &rStartPlace,
				   TextPlace &rEndPlace)
{
#ifdef LIBGX_DEBUG_BUILD
  assert( Valid(*this) );
  assert( PlaceValid(*this, rStartPlace) );
  assert( PlaceValid(*this, rEndPlace) );
  //assert( Dereferenceable(rStartPlace) ); //we can safely handle this.
  //rEndPlace does not have to be dereferenceable (how else to delete last character in text hunk?)
#endif //LIBGX_DEBUG_BUILD

  TextPlace currentPlace(rStartPlace);
  while(true)
    {
      if( !Dereferenceable(currentPlace) ) return; //can't do next step.
      GxTextHunk::TextUnit &rCUnit = *(currentPlace.listPlace);

      if( BothPlacesInSameTextUnit(currentPlace, rEndPlace) )
	{
	  if( currentPlace.subPlace == rEndPlace.subPlace )
	    {
	      rStartPlace = rEndPlace = currentPlace;
	      return;
	    };
#ifdef LIBGX_DEBUG_BUILD
	  assert( rEndPlace.subPlace > currentPlace.subPlace );
#endif //LIBGX_DEBUG_BUILD

	  if(currentPlace.subPlace == 0)
	    {
	      //we start at the begining of the TextUnit
	      //but we don't end at its end. i.e. we have at least
	      //one charcter left after this fucntion. we have to move
	      //the ending text to the front of the buffer.
	      memmove(rCUnit.text, rCUnit.text+rEndPlace.subPlace,
		      rCUnit.length - rEndPlace.subPlace);
	      rCUnit.length -= rEndPlace.subPlace;
	      //currentPlace.listPlace = //unchanged
	      currentPlace.subPlace = 0;
	      rStartPlace = rEndPlace = currentPlace;
	      return; //done.
	    }else //we don't start at the start of this unit
	      {
		//we are removing text from the middle of this TextUnit
		memmove(rCUnit.text+currentPlace.subPlace,
			rCUnit.text+rEndPlace.subPlace,
			rCUnit.length - rEndPlace.subPlace);
		rCUnit.length = currentPlace.subPlace +
		  (rCUnit.length - rEndPlace.subPlace);
		//currentPlace.listPlace = //unchanged
		//currentPlace.subPlace = //unchanged
		rStartPlace = rEndPlace = currentPlace;
		return; //done.
	      };
	}else // !BothPlacesInSameUnit()
	  {
	    //delete to the end of the current unit. if the unit
	    //is empty, remove it (if possible).
	    if(currentPlace.subPlace == 0)
	      {
		//try to set current place to t
		RemoveTextUnit(currentPlace);
		rStartPlace = rEndPlace = currentPlace;
	      }else
		{
		  //we are clipping the end of this TextUnit off.
		  rCUnit.length = currentPlace.subPlace;
		  (currentPlace.listPlace)++;
		  currentPlace.subPlace = 0;
		  rStartPlace = rEndPlace = currentPlace;
		};

	    //we are not done. we need to go around and look at the next
	    //TextUnit
	  };
    };
}

void GxTextHunk::GetTextBetween(const TextPlace &rStartPlace,
				const TextPlace &rEndPlace,
				string &rText)
{
#ifdef LIBGX_DEBUG_BUILD
  assert( Valid(*this) );
  assert( PlaceValid(*this, rStartPlace) );
  assert( PlaceValid(*this, rEndPlace) );
#endif //LIBGX_DEBUG_BUILD
  rText.clear();
  TextPlace currentPlace(rStartPlace);
  while(true)
    {
      if( !Dereferenceable(currentPlace) ) return; //can't do next step.
      GxTextHunk::TextUnit &rCUnit = *(currentPlace.listPlace);

      if( BothPlacesInSameTextUnit(currentPlace, rEndPlace) )
	{
	  rText +=  string( GetCharPointer(currentPlace),
				 rEndPlace.subPlace - currentPlace.subPlace);
	  break;
	}else
	  {
	    rText += string( GetCharPointer(currentPlace),
			     rCUnit.length - currentPlace.subPlace );
	    currentPlace.listPlace++;
	    currentPlace.subPlace = 0;
	  };
    };
}

const GxTextHunk::TextUnit & GxTextHunk::GetTextUnit(const TextPlace &rPlace)
{
  return *(rPlace.listPlace);
}

char* GxTextHunk::GetCharPointer(const TextPlace &rPlace)
{
  GxTextHunk::TextUnit &rCUnit = *(rPlace.listPlace);
  return rCUnit.text + rPlace.subPlace;
}

char GxTextHunk::GetChar(const TextPlace &rPlace) const
{
#ifdef LIBGX_DEBUG_BUILD
  assert( Dereferenceable(rPlace) );
  assert( IsCharPlace(rPlace) );
#endif //LIBGX_DEBUG_BUILD
  GxTextHunk::TextUnit &rCUnit = *(rPlace.listPlace);
  return rCUnit.text[rPlace.subPlace];
}

bool GxTextHunk::Empty(void)
{
  return textList.empty();
}

bool GxTextHunk::IsCharPlace(const TextPlace &rPlace) const
{
  return true;
}

bool GxTextHunk::BothPlacesInSameTextUnit(const TextPlace &rPlaceOne,
					  const TextPlace &rPlaceTwo) const
{
  if(rPlaceOne.listPlace == rPlaceTwo.listPlace)
    return true;
  else
    return false;
}

bool GxTextHunk::InFirstUnit(const TextPlace &rPlace) const
{

  list<TextUnit>::const_iterator cPlace = rPlace.listPlace;
  list<TextUnit>::const_iterator beginPlace = textList.begin();

  //it could be that we are at the we are also at th end
  if( cPlace == beginPlace )
    return true;
  else
    return false;
}

bool GxTextHunk::InLastUnit(const TextPlace &rPlace) const
{
  list<TextUnit>::const_iterator cPlace = rPlace.listPlace;
  list<TextUnit>::const_iterator endPlace = textList.end();

  if( ++cPlace == endPlace )
    return true;
  else
    return false;
}

bool GxTextHunk::Dereferenceable(const TextPlace &rPlace) const
{
  if( rPlace.listPlace == textList.end() )
    {
#ifdef LIBGX_DEBUG_BUILD
      assert(rPlace.subPlace == 0);
#endif //LIBGX_DEBUG_BUILD
      return false;
    }else
      return true;  
}

bool GxTextHunk::Equal(const TextPlace &rPlaceOne,
		       const TextPlace &rPlaceTwo) const
{
#ifdef LIBGX_DEBUG_BUILD
  assert( Valid(*this) );
  assert( PlaceValid(*this, rPlaceOne) );
  assert( PlaceValid(*this, rPlaceTwo) );
#endif //LIBGX_DEBUG_BUILD

  if(rPlaceTwo.listPlace != rPlaceOne.listPlace)
    return false;
  else
    if(rPlaceOne.subPlace != rPlaceTwo.subPlace)
      return false;

  return true;
}

void GxTextHunk::GetText(ostream &rOut) const
{
  list<TextUnit>::const_iterator cPlace = textList.begin();
  list<TextUnit>::const_iterator cEnd = textList.end();
  while(cPlace != cEnd)
    {
      const TextUnit &rUnit = *cPlace;
      rOut.write(rUnit.text, rUnit.length);
      cPlace++;
    };
}

void GxTextHunk::SetText(istream &rIn)
{
  textList.clear();

  while(rIn)
    {
      char chBuffer[1024];
      rIn.read(chBuffer, 1024);
      if( rIn.gcount() )
	AppendText(chBuffer, rIn.gcount());
    };
}

bool GxTextHunk::RemoveTextUnit(TextPlace &rPlace)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxTextHunk::RemoveTextUnit" << endl;
  assert( Valid(*this) );
  assert( PlaceValid(*this, rPlace) );
  //if below, we are not in a TextUnit
  assert( rPlace.listPlace != textList.end() );
#endif //LIBGX_DEBUG_BUILD

  TextPlace currentPlace(rPlace);
  //GxTextHunk::TextUnit &rCUnit = *(currentPlace.listPlace);

  if( InLastUnit(currentPlace) && InFirstUnit(currentPlace) )
    {
      //we can remove the unit, but we must be careful
      textList.clear(); //might as well; 
      currentPlace.listPlace = textList.end(); //same as begin
      currentPlace.subPlace = 0;
      rPlace = currentPlace;
      return true; //done.
    }else
      { //we can remove this unit.
	if( InLastUnit(currentPlace) )
	  { //we know we are not in the first unit
	    //from above test
	    /*
	    list<TextUnit>::iterator cPlace = currentPlace.listPlace;
	    list<TextUnit>::iterator prevPlace = --(currentPlace.listPlace);
	    textList.erase(cPlace);
	    GxTextHunk::TextUnit &rTUnit = *prevPlace;
	    
	    currentPlace.listPlace = prevPlace; //unnecessary?
	    currentPlace.subPlace = rTUnit.length-1;
	    rPlace = currentPlace;
	    */
	    //set to end
	    currentPlace.listPlace = textList.erase(currentPlace.listPlace);
	    currentPlace.listPlace = textList.end(); //same
	    currentPlace.subPlace = 0;
	    rPlace = currentPlace;
	    return true; //done.
	  }else
	    { //we are somewhere in the middle of the
	      //textList _or_ we are in the FirstUnit
	      //this algorithm works either way.
	      list<TextUnit>::iterator cPlace =
		textList.erase(currentPlace.listPlace);
	      //rCUnit is now an invalid reference
	      currentPlace.listPlace = cPlace;
	      currentPlace.subPlace = 0;
	      rPlace = currentPlace;
	      return true; //done.
	    };
      };
}

// ********************** start GxTextHunk::TextPlace ***************

GxTextHunk::TextPlace::TextPlace(const list<TextUnit>::iterator &rlPlace,
				 unsigned int tPlace) :
  valid(true), listPlace(rlPlace), subPlace(tPlace)
{}

GxTextHunk::TextPlace::TextPlace(const GxTextHunk::TextPlace &rPlace) :
  valid(rPlace.valid), listPlace(rPlace.listPlace),
  subPlace( rPlace.subPlace )
{}

GxTextHunk::TextPlace::TextPlace(void) :
  valid(false), subPlace(0)
{}

GxTextHunk::TextPlace::~TextPlace(void)
{}

const GxTextHunk::TextPlace & GxTextHunk::TextPlace::operator=(const TextPlace &rPlace)
{
  valid = rPlace.valid;
  listPlace = rPlace.listPlace;
  subPlace = rPlace.subPlace;

  return *this;
}

bool GxTextHunk::TextPlace::Valid(void) const
{
  return valid;
}

void GxTextHunk::TextPlace::Invalidate(void)
{
  valid = false;
}

bool operator==(const GxTextHunk::TextPlace &lhs, const GxTextHunk::TextPlace &rhs)
{
  return (lhs.listPlace == rhs.listPlace && lhs.subPlace == rhs.subPlace );
}

// ********************** start GxTextHunk::TextUnit *******************

GxTextHunk::TextUnit::TextUnit(void) :
  length(0)
{}

GxTextHunk::TextUnit::TextUnit(const TextUnit &rTUnit) :
  length(rTUnit.length)
{
  memcpy(text, rTUnit.text, rTUnit.length);
}

GxTextHunk::TextUnit::~TextUnit(void)
{}

// ********************* start GxTextHunk friends *******************************
#ifdef LIBGX_DEBUG_BUILD
bool PlaceValid(const GxTextHunk &rHunk, const GxTextHunk::TextPlace &rPlace)
{
  list<GxTextHunk::TextUnit>::const_iterator endPlace = rHunk.textList.end();

  if( rPlace.listPlace == endPlace )
    {
      //we can't dereference listPlace
      if(rPlace.subPlace != 0)
	return false;
      else
	return true;
    }else
      {
	//be sure the listPlace refers to an object in my list
	list<GxTextHunk::TextUnit>::const_iterator cPlace = rHunk.textList.begin();
	bool listPlaceExists = false;
	while(cPlace != endPlace)
	  {
	    if( &(*(rPlace.listPlace)) == &(*cPlace) )
	      {
		listPlaceExists = true;
		break;
	      };

	    cPlace++;
	  };

	if(!listPlaceExists) return false;

	const GxTextHunk::TextUnit &rTUnit = *(rPlace.listPlace);
	if(rPlace.subPlace >= rTUnit.length)
	  return false;
      };

  return true;
}

bool Valid(const GxTextHunk &rHunk)
{
  list<GxTextHunk::TextUnit>::const_iterator cPlace = rHunk.textList.begin();
  list<GxTextHunk::TextUnit>::const_iterator endPlace = rHunk.textList.end();
  while(cPlace != endPlace)
    {
      const GxTextHunk::TextUnit & rUnit = *cPlace;

      if(rUnit.length > GXTWUNIT || rUnit.length == 0)
	return false;

      cPlace++;
    };

  return true;
}

#endif //LIBGX_DEBUG_BUILD


const unsigned GXTEXTWIN_DEFAULT_XSTART = GX_BORDER_WD+2;
const unsigned PIX_H_MOVE_INC = 30; //uglyish to be a constant.


//*****************************************************************
//*****************************************************************
//*****************************************************************
// ******************** start GxTextWin ***************************
//*****************************************************************
//*****************************************************************
//*****************************************************************

GxTextWin::GxTextWin(GxRealOwner *pOwner) :
  GxAppScrolledWin(pOwner), textGrabbed(false), editable(false),
  wrap(false), haveFocus(false), textSelected(false),
  cursorPos( windowText.GetBeginPlace() ), dispWin(this),
  dInfo( pOwner->GetDisplayInfo() )
{
  dispWin.keyEventCB.Assign( CbOneMember<const XKeyEvent&, GxTextWin>(this, &GxTextWin::HandleKeyEvent) );

  dispWin.exposeCB.Assign( CbVoidMember<GxTextWin>(this,  &GxTextWin::DrawDisplayWin) );

  dispWin.clickCB.Assign( CbTwoMember<const CCoord&, int, GxTextWin>(this,  &GxTextWin::HandleClick) );

  dispWin.motionCB.Assign( CbOneMember<const CCoord&, GxTextWin>(this,  &GxTextWin::HandleClickDragMotion) );

  dispWin.focusCB.Assign( CbOneMember<bool, GxTextWin>(this, &GxTextWin::HandleFocusChange) );

  lineHeight = dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent;

  pClipWin = &dispWin;

  vScrollBar.scrollCB.Assign( CbOneMember< GxFraction, GxTextWin>(this, &GxTextWin::VScrollCallback) );
  vScrollBar.scrollUpCB.Assign( CbVoidMember<GxTextWin>(this, &GxTextWin::ScrollUp) );
  vScrollBar.scrollDownCB.Assign( CbVoidMember<GxTextWin>(this, &GxTextWin::ScrollDown) );

  hScrollBar.scrollCB.Assign( CbOneMember<GxFraction, GxTextWin>(this, &GxTextWin::HScrollCallback) );
  hScrollBar.scrollLeftCB.Assign( CbVoidMember<GxTextWin>(this, &GxTextWin::ScrollLeft) );
  hScrollBar.scrollRightCB.Assign( CbVoidMember<GxTextWin>(this, &GxTextWin::ScrollRight) );

  Format(); //make sure we are valid (i.e. have lines and the first display line is set)

#ifdef LIBGX_DEBUG_BUILD
  CheckValid(*this);
#endif //LIBGX_DEBUG_BUILD
}

GxTextWin::~GxTextWin(void)
{
  ClearLines();
}

void GxTextWin::SetText(const char *pText, bool scrollEnd)
{
  //must be called first because lines have information which will be
  //invalidataed the moment we modify the windowText
  ClearLines();
  vScrollBar.SetBothFractions(GX_MAX_FRACTION, GX_MIN_FRACTION);
  hScrollBar.SetBothFractions(GX_MAX_FRACTION, GX_MIN_FRACTION);

  windowText.Clear();
  windowText.AppendText(pText, strlen(pText));
  cursorPos = windowText.GetBeginPlace();

  Format();
  if(scrollEnd)
    {
      unsigned numLines = lineList.size();
      unsigned numViewableLines = dispWin.GetTextAreaHeight()/lineHeight;
      if(numLines > numViewableLines)
	{
	  startingDrawnLinePlace = lineList.begin();
	  for(unsigned ii = 0; ii < numLines - numViewableLines; ii++)
	    startingDrawnLinePlace++;
	};
      SetScrollBarFractions(); //hackish.
    };

  dispWin.ClearAndDisplay();
  textSelected = false;
  selectStartLineListPlace = lineList.end();
  selectStartTextPlace = windowText.GetEndPlace();

#ifdef LIBGX_DEBUG_BUILD
  CheckValid(*this);
#endif //LIBGX_DEBUG_BUILD
}

void GxTextWin::GetText(string &rText) const
{
  //masive hack
  ostringstream oStr;
  windowText.GetText(oStr);
  rText = oStr.str();
}

GxTextHunk& GxTextWin::GrabTextHunk(void)
{
  SetTextGrabbed(true);
  ClearLines(); //so nothing here refrences the object about to be modified

  return windowText;
}

GxTextHunk& GxTextWin::GrabTextHunk(GxTextHunk::TextPlace &rCursorPlace, unsigned &rCurrentTopLine)
{
#ifdef LIBX_DEBUG_BUILD
  assert(!textGrabbed);
  CheckLinePlaceValid(startingDrawnLinePlace, false);
#endif //LIBGX_DEBUG_BUILD


  SetTextGrabbed(true);
  rCursorPlace = cursorPos;
  rCurrentTopLine = (*startingDrawnLinePlace)->lineNum;
  ClearLines(); //so nothing here refrences the object about to be modified

  return windowText;
}

void GxTextWin::ReleaseTextHunk(void)
{
  SetTextGrabbed(false);
  cursorPos = windowText.GetBeginPlace(); //better than invalidating it.
  Format();

#ifdef LIBGX_DEBUG_BUILD
  CheckValid(*this);
#endif //LIBGX_DEBUG_BUILD

  dispWin.ClearAndDisplay();
}

void GxTextWin::ReleaseTextHunk(GxTextHunk::TextPlace &rCursorPlace, unsigned lastCurrentTopLine)
{
  SetTextGrabbed(false);
  cursorPos = rCursorPlace;
  Format(true, lastCurrentTopLine);

#ifdef LIBGX_DEBUG_BUILD
  CheckValid(*this); //this checks that the cursor pos is valid
#endif //LIBGX_DEBUG_BUILD

  dispWin.ClearAndDisplay();
}

void GxTextWin::Wrap(bool wrapStat)
{
  wrap = wrapStat;
}

bool GxTextWin::Wrap(void) const
{
  return wrap;
}

void GxTextWin::Editable(bool editStat)
{
  editable = editStat;
  if( dispWin.Created() )
    dispWin.ClearAndDisplay();
}

bool GxTextWin::Editable(void) const
{
  return editable;
}

/*
void GxTextWin::Spacing(int tSpacing)
{

}

bool GxTextWin::Spacing(void) const
{

}
*/

bool GxTextWin::AcceptFocus(Time eventTime)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxTextWin::AcceptFocus" << endl;
#endif //LIBGX_DEBUG_BUILD

  if( !dispWin.Created() || !editable)
    return false;
  else
    {
      //hack according to icccm; CurrentTime is invalid, however it is
      //just fine according to XSetInputFocus manpage
      XSetInputFocus(dInfo.display, dispWin.GetWindow(), RevertToParent, eventTime);
      return true;
    };
}

void GxTextWin::Resize(UINT tWidth, UINT tHeight)
{
  GxAppScrolledWin::Resize(tWidth, tHeight);

  if(!textGrabbed && wrap)
    Format();
}

void GxTextWin::Place(int &lX, int &rX, int &tY, int &bY)
{
  GxAppScrolledWin::Place(lX, rX, tY, bY);

  if(!textGrabbed)
    Format();
}

void GxTextWin::Create(void)
{
  GxAppScrolledWin::Create(); //will create our children
}

// ********************* start GxTextWin::DrawInfo ********************

GxTextWin::DrawInfo::DrawInfo(GxDisplayInfo &rDInfo, GxVolatileData &rVData) :
  dInfo(rDInfo), vData(rVData)
{}

GxTextWin::DrawInfo::~DrawInfo(void)
{}
// ********************* end GxTextWin::DrawInfo ********************

// ********************* start DisplayChunk ********************
GxTextWin::DisplayChunk::DisplayChunk(const GxTextHunk::TextPlace &rPlace,
				      int xPixStart) :
  startPlace(rPlace), numChars(0), xPixel(xPixStart)
{}

GxTextWin::DisplayChunk::DisplayChunk(const GxTextWin::DisplayChunk &rChunk) :
  startPlace(rChunk.startPlace), numChars(rChunk.numChars),
  xPixel(rChunk.xPixel)
{}

GxTextWin::DisplayChunk::~DisplayChunk(void)
{}
/*
void GxTextWin::DisplayChunk::GetEndPlace(GxTextHunk::TextPlace &rEndPlace)
{
  rEndPlace.listPlace = startPlace.listPlace;
  rEndPlace.subPlace = startPlace.subPlace + numChars;
}
*/
void GxTextWin::DisplayChunk::Display(DrawInfo &lInfo, int xOffset, int lineYPixel) const
{
  if(numChars == 0) return;

  XDrawString(lInfo.dInfo.display, lInfo.xWin, lInfo.vData.textGC,
	      xOffset + xPixel, lineYPixel + lInfo.dInfo.pDefaultFont->ascent,
	      StartPtr(), numChars);
}

void GxTextWin::DisplayChunk::Display(DrawInfo &lInfo, int xOffset, int lineYPixel,
				      int startOffset, int endOffset) const
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "drawing highlighted chunk" << endl;
  assert( (startOffset <= (int)numChars) && (endOffset <= (int)numChars) &&
	  (endOffset > startOffset) && (startOffset >= 0) && (endOffset >= 0) );
#endif //LIBGX_DEBUG_BUILD
  int xPixOffset = xOffset;
  if(startOffset) //the first part of the string is not highlighted
    {
      //hack? we could only compute this if we know we are going to draw the next fragment.
      XSetForeground(lInfo.dInfo.display, lInfo.vData.textGC, lInfo.dInfo.textPix);
      XDrawString(lInfo.dInfo.display, lInfo.xWin, lInfo.vData.textGC, xPixOffset, lineYPixel,
		  StartPtr(), startOffset);

      xPixOffset += XTextWidth(lInfo.dInfo.pDefaultFont, StartPtr(), startOffset);
    };

  //draw the highlighted section
  //draw the highlight rectangle color
  unsigned tWidth = XTextWidth(lInfo.dInfo.pDefaultFont, StartPtr()+startOffset, endOffset-startOffset);
  XSetForeground(lInfo.dInfo.display, lInfo.vData.textGC, lInfo.dInfo.selectedTextBackgroundPix);
  XFillRectangle(lInfo.dInfo.display, lInfo.xWin, lInfo.vData.textGC, xPixOffset,lineYPixel, tWidth,20);
  xPixOffset += tWidth;
  //setup to draw the highlighted text;
  XSetForeground(lInfo.dInfo.display, lInfo.vData.textGC, lInfo.dInfo.selectedTextPix);
  XDrawString(lInfo.dInfo.display, lInfo.xWin, lInfo.vData.textGC, xPixOffset, lineYPixel,
	      StartPtr()+startOffset, endOffset-startOffset);
  XSetForeground(lInfo.dInfo.display, lInfo.vData.textGC, lInfo.dInfo.textPix);

  //draw the unhighlighted section after the highlight if it exists
  if(endOffset < (int)numChars)
    {
      XSetForeground(lInfo.dInfo.display, lInfo.vData.textGC, lInfo.dInfo.textPix);
      XDrawString(lInfo.dInfo.display, lInfo.xWin, lInfo.vData.textGC, xPixOffset, lineYPixel,
		  StartPtr()+endOffset, numChars-endOffset);
    };
}

// ********************* end DisplayChunk ********************

// *********************** start Line **************************

GxTextWin::Line::Line(void) :
  lineNum(0), lineLength(0)
{}

GxTextWin::Line::~Line(void)
{}

void GxTextWin::Line::ClearHighlight(void)
{
  hlStart.Invalidate();
  hlEnd.Invalidate();
}

GxTextWin::DisplayChunk& GxTextWin::Line::GetLastChunk(void)
{
  assert( !dcList.empty() );

  return dcList.back();
}

void GxTextWin::Line::Display(DrawInfo &lInfo, int xOffset, int lineYPix) const
{
  list<DisplayChunk>::const_iterator cPlace = dcList.begin();
  list<DisplayChunk>::const_iterator cEnd = dcList.end();

  //the no-highlighting-on-this-line routine.
  if( !hlStart.Valid() )
    {
      while(cPlace != cEnd)
	{
	  (*cPlace).Display(lInfo, xOffset, lineYPix);
	  cPlace++;
	};
      return;
    };

#ifdef LIBGX_DEBUG_BUILD
  cout << "starting highlighted line display part" << endl;
#endif //LIBGX_DEBUG_BUILD

  bool highlightStarted = false;
  bool highlightEnded = false;
  while(cPlace != cEnd)
    {
      const DisplayChunk &rChunk = *cPlace;
      if(!highlightStarted)
	{
	  if( &(*rChunk.startPlace.listPlace) ==  &(*hlStart.listPlace) )
	    highlightStarted = true;
	
	  if(highlightStarted)
	    {
	      int startOffset = hlStart.subPlace - rChunk.startPlace.subPlace;
	      int endOffset = rChunk.numChars;
	      //check (and note) if we start and end on the same chunk
	      if( &(rChunk.startPlace.listPlace) ==  &(hlEnd.listPlace) )
		{
		  endOffset = hlEnd.subPlace - rChunk.startPlace.subPlace;
		  highlightEnded = true;
		};
	      rChunk.Display(lInfo, xOffset, lineYPix, startOffset, endOffset);
	    }else
	      rChunk.Display(lInfo, xOffset, lineYPix);
	}else //the highlight has been started
	  if(highlightEnded)
	    {
	      rChunk.Display(lInfo, xOffset, lineYPix);
	    }else
	      {
		if( &(*rChunk.startPlace.listPlace) ==  &(*hlEnd.listPlace) )
		  highlightEnded = true;
	
		if(highlightEnded) //we know we end on this chunk
		  {
		    rChunk.Display(lInfo, xOffset, lineYPix, 0, hlEnd.subPlace - rChunk.startPlace.subPlace);
		  }else //the entire chunk is highlighted
		    rChunk.Display(lInfo, xOffset, lineYPix, 0, rChunk.numChars);
	      };
      cPlace++;
    };
}


// *********************** end Line *****************************

// ********************* start GxTextWin::CCoord *******************
GxTextWin::CCoord::CCoord(void) :
  xC(0), yC(0), time(CurrentTime)
{}

GxTextWin::CCoord::CCoord(int tX, int tY) :
  xC(tX), yC(tY), time(CurrentTime)
{}

GxTextWin::CCoord::CCoord(int tX, int tY, Time eventTime) :
  xC(tX), yC(tY), time(eventTime)
{}


GxTextWin::CCoord::CCoord(const CCoord &rhs) :
  xC(rhs.xC), yC(rhs.yC), time(rhs.time)
{}

GxTextWin::CCoord::~CCoord(void)
{}

const GxTextWin::CCoord & GxTextWin::CCoord::operator=(const GxTextWin::CCoord &rhs)
{
  xC = rhs.xC;
  yC = rhs.yC;

  return *this;
}
// ********************* end GxTextWin::CCoord *******************


// ******************** start GxTextWin::DisplayWindow ********************
GxTextWin::DisplayWindow::DisplayWindow(GxRealOwner *pOwner) :
  GxWin(pOwner), cursor(dInfo, vData), freezeEvents(false), lInfo(dInfo, vData)
{}

GxTextWin::DisplayWindow::~DisplayWindow(void)
{}

void GxTextWin::DisplayWindow::ClearAndDisplay(void)
{
  if( Created() )
    {
      XClearArea(dInfo.display, xWin, GX_BORDER_WD, GX_BORDER_WD,
		 width-2*GX_BORDER_WD, height-2*GX_BORDER_WD, false);
      exposeCB();
    };
}

void GxTextWin::DisplayWindow::DrawBorder(void)
{
  if( !Created() ) return;
  Draw3dBorder(0,0, width,height, false);  
}

void GxTextWin::DisplayWindow::Create(void)
{
  GxWin::Create();
  lInfo.xWin = xWin;
  cursor.xWin = xWin;
}

void GxTextWin::DisplayWindow::HandleEvent(const XEvent &rEvent)
{
  if(freezeEvents) return; //be sure not to touch anything for any reason.

  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      {
	exposeCB();
	return;
      };

  if(rEvent.type == KeyPress)
    {
      keyEventCB(rEvent.xkey);
      return;
    };

  if(rEvent.type == ButtonPress) {
    if(rEvent.xbutton.button == 2) //we got a paste
      {
#ifdef LIBGX_DEBUG_BUILD
	cout << "paste" << endl;
#endif //LIBGX_DEBUG_BUILD
	CCoord clickCoord;
	clickCB( CCoord(rEvent.xbutton.x, rEvent.xbutton.y, rEvent.xbutton.time), 3);
	return;
      }else
	if(rEvent.xbutton.button == 1) //we got a mouse down
	  {
	    //cout << "mouse down" << endl;
	    GxTextHunk::TextPlace selPlace;
	    clickCB( CCoord(rEvent.xbutton.x, rEvent.xbutton.y, rEvent.xbutton.time), 1);
	    return;
	  };
  };

  if(rEvent.type == MotionNotify)
    {
      motionCB( CCoord(rEvent.xmotion.x, rEvent.xmotion.y, rEvent.xmotion.time) );
      return;
    };

  if(rEvent.type == ButtonRelease)
    {
#ifdef LIBGX_DEBUG_BUILD
      cout << "button release" << endl;
#endif //LIBGX_DEBUG_BUILD
    };

  if(rEvent.type == FocusOut)
    {
      focusCB(false);
	//haveFocus = false;
    };
}

unsigned GxTextWin::DisplayWindow::GetTextAreaWidth(void) const
{
  return width - 2*(GXTEXTWIN_DEFAULT_XSTART);
}

unsigned GxTextWin::DisplayWindow::GetTextAreaHeight(void) const
{
  return height - 2*GX_BORDER_WD - 2;
}

void GxTextWin::DisplayWindow::GetWindowData(XSetWindowAttributes &winAttrib,
					     ULINT &valueMask)
{
  GxWin::GetWindowData(winAttrib, valueMask);
  winAttrib.event_mask = ExposureMask | Button1MotionMask | KeyPressMask | ButtonPress | ButtonRelease;
  winAttrib.background_pixel = dInfo.whitePix;

  valueMask |= CWEventMask | CWBackPixel;
}

// ******************** end GxTextWin::DisplayWindow ********************

void GxTextWin::HScrollCallback(GxFraction hScrollFr)
{
  if(textGrabbed) return;

  unsigned viewableWidth = dispWin.GetTextAreaWidth();
  if(viewableWidth >= longestLine)
    {//should not have happened
      hScrollBar.SetBothFractions(GX_MAX_FRACTION, GX_MIN_FRACTION);
      return;
    };

  int newLineStartXPixel =
    - hScrollFr.Convert( (unsigned)(longestLine - viewableWidth) );

  if(newLineStartXPixel == lineStartXPixel) return;

  lineStartXPixel = newLineStartXPixel;
  lineStartXPixel += GXTEXTWIN_DEFAULT_XSTART; //hackish. cut and paste from Format

  if(editable)
    SetCursorPixPos();

  dispWin.ClearAndDisplay();
}

void GxTextWin::VScrollCallback(GxFraction vScrollFr)
{
  if(textGrabbed) return;

  unsigned numViewableLines = dispWin.GetTextAreaHeight()/lineHeight;
  unsigned numLines = lineList.size(); //ugly
  if(numViewableLines >= numLines)
    {
      //shouldn't have gotten a callback; will fix
      vScrollBar.SetBothFractions(GX_MAX_FRACTION, GX_MIN_FRACTION);
      return;
    };

  unsigned newLineStart = vScrollFr.Convert( (unsigned)(numLines - numViewableLines));

  if(newLineStart == (*startingDrawnLinePlace)->lineNum) return;

  //may be negative
  int startLineDist = ((int)newLineStart - (int)((*startingDrawnLinePlace)->lineNum) );
  if(startLineDist < 0)
    {
      for(int ii = 0; ii > startLineDist; ii--)
	{
	  startingDrawnLinePlace--;
	  if(startingDrawnLinePlace == lineList.begin() ) break;
	};
    }else
      for(int ii = 0; ii < startLineDist; ii++)
	{
	  startingDrawnLinePlace++;
	  if(startingDrawnLinePlace == lineList.end() )
	    {
	      startingDrawnLinePlace--;
	      break;
	    };
	};

  if(editable)
    SetCursorPixPos();

  dispWin.ClearAndDisplay();
}

void GxTextWin::ScrollLeft(void)
{
  if(textGrabbed) return;

  if( lineStartXPixel + (int)PIX_H_MOVE_INC > (int)GXTEXTWIN_DEFAULT_XSTART)
    {
      lineStartXPixel = GXTEXTWIN_DEFAULT_XSTART;
    }else
      lineStartXPixel += (int)PIX_H_MOVE_INC;

  SetScrollBarFractions();
  
  if(editable)
    SetCursorPixPos();
  
  dispWin.ClearAndDisplay();
}

void GxTextWin::ScrollRight(void)
{
  if(textGrabbed) return;

  if( lineStartXPixel + (int)longestLine - (int)PIX_H_MOVE_INC < (int)dispWin.GetTextAreaWidth())
    {
      lineStartXPixel = dispWin.GetTextAreaWidth() - (int)longestLine - GXTEXTWIN_DEFAULT_XSTART;
    }else
      lineStartXPixel -= (int)PIX_H_MOVE_INC;

  SetScrollBarFractions();

  if(editable)
    SetCursorPixPos();

  dispWin.ClearAndDisplay();
}

void GxTextWin::ScrollUp(void)
{
  if(textGrabbed) return;
  if(startingDrawnLinePlace == lineList.begin() ) return;

  startingDrawnLinePlace--;
  SetScrollBarFractions();
  if(editable)
    SetCursorPixPos();
  dispWin.ClearAndDisplay();
}

void GxTextWin::ScrollDown(void)
{
  if(textGrabbed) return;

  unsigned numLines = lineList.size();
  unsigned int numViewableLines = dispWin.GetTextAreaHeight()/lineHeight;

  if(numLines < numViewableLines) return;

  unsigned finalTopLineNum = numLines - numViewableLines;
  if( (*startingDrawnLinePlace)->lineNum >= finalTopLineNum) return;

  startingDrawnLinePlace++;
  SetScrollBarFractions();
  if(editable)
    SetCursorPixPos();
  dispWin.ClearAndDisplay();
}

void GxTextWin::Format(bool viewCursor, unsigned tOldTopLineNum)
{
#ifdef LIBGX_DEBUG_BUILD
  assert( !textGrabbed );
  assert( Valid(windowText) );
#endif //LIBGX_DEBUG_BUILD

  //unsigned oldCursorLineNum = ;
  unsigned oldTopLineNum = 0; //the initial format() happens with an empty line list. perhaps the constructor could fix this.
  if(viewCursor)
    {
      if( !lineList.empty() )
	oldTopLineNum = (*startingDrawnLinePlace)->lineNum;
      else
	oldTopLineNum = tOldTopLineNum;
    };

  longestLine = 0;

  ClearLines();
  /*
  if(textGrabbed)
    {
      //we must create a dummy line so that we are valid.
      //asctually this does not make us valid because the line dcList is empty.
      lineList.push_back( new Line() );
      return;
    };
  */
  //using this special case implemetation is hackish. we should have proper edge condition operation
  //in the behavious encoded below.
  if( windowText.Empty() )
    {
      cursorPos = windowText.GetBeginPlace(); //i.e. the end place
      Line *pNewLine = new Line();
      pNewLine->dcList.push_back( DisplayChunk(cursorPos, 0) );
      lineList.push_back( pNewLine );
      startingDrawnLinePlace = lineList.begin();
      lineStartXPixel = GXTEXTWIN_DEFAULT_XSTART;
      SetCursorPixPos();
      return;
    };

  GxTextHunk::TextPlace tPlace( windowText.GetBeginPlace() );
#ifdef LIBGX_DEBUG_BUILD
  assert( PlaceValid(windowText, tPlace) );
#endif //LIBGX_DEBUG_BUILD

  unsigned cLineNum = 0;
  while( windowText.Dereferenceable(tPlace) )
    {
      GxTextHunk::TextPlace lineEnd;
      Line *pCLine = new Line;
      //if(!pCLine) break; //will never happen

      unsigned cLineLen;
      if(wrap)
	cLineLen = FormatWrappedLine(pCLine, tPlace, lineEnd);
      else
	cLineLen = FormatUnWrappedLine(pCLine, tPlace, lineEnd);

      longestLine = (cLineLen > longestLine) ? cLineLen : longestLine;

      pCLine->lineNum = cLineNum;
      cLineNum++;
      pCLine->lineLength = cLineLen; //somewhat hackish to set it here.

      lineList.push_back(pCLine);
      tPlace = lineEnd;
#ifdef LIBGX_DEBUG_BUILD
      assert( PlaceValid(windowText, tPlace) );
#endif //LIBGX_DEBUG_BUILD
    }; 

  //we may need to add an empty line to hold the cursor
  bool extraLineNeeded = windowText.Empty();
  if( !extraLineNeeded ) 
    {
      windowText.DecrementTextPlace(tPlace);
#ifdef LIBGX_DEBUG_BUILD
      assert( windowText.Dereferenceable(tPlace) );
#endif //LIBGX_DEBUG_BUILD
      if( windowText.GetChar(tPlace) == '\n' )
	extraLineNeeded = true;
      windowText.IncrementTextPlace(tPlace);
    };

  if(extraLineNeeded)
    {
#ifdef LIBGX_DEBUG_BUILD
      //not really so hackish. we must have a valid line to put the cursor on at all times.
      cout << "tripped hack. we added line num: " << cLineNum << endl;
#endif //LIBGX_DEBUG_BUILD

      Line *pCLine = new Line;
      pCLine->lineNum = cLineNum;
      cLineNum++;
      DisplayChunk cursorChunk(tPlace, 0);
      cursorChunk.numChars = 0;
      pCLine->dcList.push_back(cursorChunk);
      lineList.push_back(pCLine);
    };

  if(viewCursor)
    {
      MakeCursorVisible(oldTopLineNum);
    }else
      {
	startingDrawnLinePlace = lineList.begin();
	lineStartXPixel = GXTEXTWIN_DEFAULT_XSTART;
      };

  SetScrollBarFractions();

  if(editable)
    assert( SetCursorPixPos() );
}

unsigned GxTextWin::FormatWrappedLine(GxTextWin::Line *pCLine,
				      const GxTextHunk::TextPlace &lineStart,
				      GxTextHunk::TextPlace &nextLineStart)
{
  //by defination we fit within the width of the GxTextWin DisplayWindow
  //(unless each line contains only a single character & the window is narrower
  //than this single character)
  //there must be at least *some* text on each line (unless \n prevents it)
  //and even in this case a DisplayChunk must be added to the line.
  //generic algorithim -> search from start of line adding length of each
  //line untill find last character on line. then find the last whitespace
  //in the line

#ifdef LIBGX_DEBUG_BUILD
  assert(pCLine);
  assert( PlaceValid(windowText, lineStart) );
  assert( windowText.Dereferenceable(lineStart) );
#endif //LIBGX_DEBUG_BUILD

  GxTextHunk::TextPlace lastWhitePlace;
  unsigned lineLenToLastWhiteSpace = 0;
  unsigned whiteSpaceCharNum = 0;
  //this must point to the disp chunk *in* dcList. i.e. assign after putting
  //it into the list
  GxTextWin::DisplayChunk *pLastWhiteDispChunk = 0;

  unsigned renderWidth = dispWin.GetTextAreaWidth();
  unsigned cLineLen = 0;

  GxTextHunk::TextPlace cPlace = lineStart;

  char *pText = windowText.GetCharPointer(cPlace);
  unsigned numChars = 1;
  GxTextHunk::TextPlace cTextUnitStartPlace = cPlace;

  //the dcList can never be empty; there must always be *something* on a line
  //even if it is not drawn (like a cr)
  list<DisplayChunk> dcList;
  dcList.push_back( GxTextWin::DisplayChunk(cPlace, cLineLen) );

  while(true)
    {
      do
	{
	  unsigned cChar = pText[numChars-1];
	  
	  if(cChar == '\n')
	    {
	      //the line ends here (but we don't want to display the '\n')
	      ( dcList.back() ).numChars = numChars-1;
	      
	      //the next line starts just after the '\n'
	      windowText.IncrementTextPlace(cPlace);
	      nextLineStart = cPlace; //may not be Dereferenceable! (this means end of text too)
	      goto addLine;
	    }else
	      if( isspace((char)cChar) )
		{
		  lineLenToLastWhiteSpace = cLineLen;
		  lastWhitePlace = cPlace;
		  whiteSpaceCharNum = numChars;
		  pLastWhiteDispChunk = &( dcList.back() );
		};
	  
	  short charWidth = GetCharWidth(cChar);
	  cLineLen += charWidth;
	  if(cLineLen >= renderWidth) break;
	  //this cannot fail because we check after every increment if we can increment again
	  //and return from this function if we cannot.

	  assert( windowText.IncrementTextPlace(cPlace) );
	  if( !windowText.Dereferenceable(cPlace) )
	    {
	      //we found the end of our line (because we ran out of characters)
	      ( dcList.back() ).numChars = numChars;
	      nextLineStart = cPlace;
	      goto addLine;
	    }else
	      numChars++;
     }while( windowText.BothPlacesInSameTextUnit(cTextUnitStartPlace, cPlace) );
      /*
	we could be here for two reasons:
	#1 the current character is to the edge or beyond the edge of the
           renderable width. In this case cPlace still points to the final
	   character examined and numChars is still exactly correct.
	#2 we have gone beyond the end of the current textUnit. in this case
	   cPlace points to the next character
	   (which we know exists -> we handled text ending above)
	   and numChars is one two large.
      */
      if( cLineLen < renderWidth )
	{
	  //we finished the end of the current textUnit.
	  //numChars is one two large. we must prepare to move on to the next
	  //text unit (if it exists)
	  ( dcList.back() ).numChars = numChars-1;
	  //get ready to go back around using annother TextUnit
	  pText = windowText.GetCharPointer(cPlace);
	  numChars = 1;
	  dcList.push_back(GxTextWin::DisplayChunk(cPlace, cLineLen));
	  cTextUnitStartPlace = cPlace;
	}else
	  {
	    //we have examined character to the edge of or beyond the
	    //renderable width. we don't yet know which character we
	    //will actuall end the DisplayChunk on. i.e. is could be cPlace
	    //or it could be cPlace--
	    break;
	  };
    };

  /*
    #1 the current character is to the edge or beyond the edge of the
       renderable width. In this case cPlace still points to the final
       character examined and numChars is still exactly correct.
    #2 we have gone beyond the end of the current textUnit. we know that
       there is not annother text unit beyond it. (we have gone through all
       the text in the TextHunk). In this case cPlace.Valid is false.
  */

  //we know we are actually breaking a line
  //if we have a white space, see if we can break the line at it
  //if the distance from the last whitespace to the end of the line
  //is too long, we might not be able to break the line at that space
  //we might have to break the line at the last character on the line
  if( (renderWidth - lineLenToLastWhiteSpace) <= (int)TEXT_WIN_MAX_BLANK_LEN )
    {
      //we have to remove the extra DisplayChunks from dcList
      //untill we reach the displayChunk that our whitespace is in.
      //then we have to modify this
      //if the correct* is not in the list we will crash here.
      //this shouldn't happen.
      while( pLastWhiteDispChunk != &( dcList.back() ) )
	dcList.pop_back();
      //now re-setup this DisplayChunk
      ( dcList.back() ).numChars = whiteSpaceCharNum;
      cLineLen = lineLenToLastWhiteSpace;
      //we start the next line at the character just beyond the
      //last white space
      windowText.IncrementTextPlace(lastWhitePlace);
      nextLineStart = lastWhitePlace;
      goto addLine;
    }else
      {
	//hack; might be including one to many chars.
	//no white space or couldn't break the line at it.
	//we therefore end the line at cPlace
	( dcList.back() ).numChars = numChars;

	//note the next character is at cPlace++
	windowText.IncrementTextPlace(cPlace);
	nextLineStart = cPlace; //may not be dereferenceable
	goto addLine;
      };

 addLine:
  pCLine->dcList.clear();
  pCLine->dcList.splice(pCLine->dcList.begin(), dcList);
  return cLineLen;
}

unsigned GxTextWin::FormatUnWrappedLine(GxTextWin::Line *pCLine,
					const GxTextHunk::TextPlace &lineStart,
					GxTextHunk::TextPlace &nextLineStart)
{
  
#ifdef LIBGX_DEBUG_BUILD
  assert(pCLine);
  assert( PlaceValid(windowText, lineStart) );
  assert( windowText.Dereferenceable(lineStart) );
#endif //LIBGX_DEBUG_BUILD

  unsigned cLineLen = 0;
  GxTextHunk::TextPlace cPlace = lineStart;

  //the dcList can never be empty; there must always be *something* on a line
  //even if it is not drawn (like a cr)

  const char *pText = windowText.GetCharPointer(cPlace);
  int numChars = 1; //the number of chars in the DisplayChunk
  GxTextHunk::TextPlace cTextUnitStartPlace = cPlace;

  list<DisplayChunk> dcList;
  dcList.push_back( GxTextWin::DisplayChunk(cPlace, cLineLen) );

  while(true)
    {
      do
	{
	  unsigned cChar = pText[numChars-1];
	  if(cChar == '\n')
	    {
	      //the line ends here but nextLineStart is cPlace++
	      //we actually don't include the '\n' in numChars so it
	      //is not printed
	      ( dcList.back() ).numChars = numChars-1;
	      if( dcList.size() > 1 )
		if( ( dcList.back() ).numChars == 0 )
		  dcList.pop_back();

	      windowText.IncrementTextPlace(cPlace);
	      nextLineStart = cPlace;
	      goto addLine;
	    };

	  cLineLen += GetCharWidth(cChar);
	  if( !windowText.IncrementTextPlace(cPlace) )
	    {
	      numChars++;//ghetto, but we decrement below.
	      break;
	    };

	  numChars++;
	}while( windowText.BothPlacesInSameTextUnit(cPlace, cTextUnitStartPlace ) );

      //one way or annother we are ending the current DisplayChunk. In order
      //to complete the setup of its internal state, we have to set its
      //numChars. This is set to numChars-1 because we incremented numChars
      //before we knew that we were already at the end of the current TextChunk
      ( dcList.back() ).numChars = numChars-1;
      if( ( dcList.back() ).numChars == 0)
	{
	  dcList.pop_back();
	};

      if( !windowText.Dereferenceable(cPlace) )
	{
	  //we've run out of text to render so the line must end
	  nextLineStart = cPlace;
	  goto addLine;
	}else
	  {
	    //we need to add a new DisplayChunk to dcList
	    //first complete the top DisplayChunk on the dcList
	    
	    //re-initialize various variables
	    pText = windowText.GetCharPointer(cPlace);
	    numChars = 1;
	    //now add the new display chunk
	    dcList.push_back( GxTextWin::DisplayChunk(cPlace, cLineLen) );
	    cTextUnitStartPlace = cPlace;
	  };
    };

 addLine:
  pCLine->dcList.clear();
  pCLine->dcList.splice(pCLine->dcList.begin(), dcList);
  return cLineLen;
}

void GxTextWin::MakeCursorVisible(unsigned oldTopLine)
{
  unsigned numLines = lineList.size();
  unsigned int numViewableLines = dispWin.GetTextAreaHeight()/lineHeight;
  //cout << "numViewableLines: " << numViewableLines << endl;

  std::list<Line*>::iterator cursorLinePos;
  unsigned cursorHPos = 0;
  GetCursorInfo(cursorLinePos, cursorHPos);
#ifdef LIBGX_DEBUG_BUILD
  assert( cursorLinePos != lineList.end() );
#endif //LIBGX_DEBUG_BUILD

  if(longestLine <= dispWin.GetTextAreaWidth() )
    {
      lineStartXPixel = GXTEXTWIN_DEFAULT_XSTART;
    }else
      {
	//make the cursor visible in the horizontal plane
	int cursorWinPixPos = (int)cursorHPos + lineStartXPixel;
	if( cursorWinPixPos < (int)GXTEXTWIN_DEFAULT_XSTART )
	  {
	    int newStart = cursorWinPixPos - PIX_H_MOVE_INC;
	    if( newStart < (int)GXTEXTWIN_DEFAULT_XSTART)
	      lineStartXPixel = GXTEXTWIN_DEFAULT_XSTART;
	    else
	      lineStartXPixel = newStart;
	  }else
	    if( cursorWinPixPos > (int)dispWin.GetTextAreaWidth() + (int)GXTEXTWIN_DEFAULT_XSTART)
	      {
		if((int)cursorHPos + PIX_H_MOVE_INC > longestLine)
		  lineStartXPixel = -(longestLine - dispWin.GetTextAreaWidth() + GXTEXTWIN_DEFAULT_XSTART);
		else
		  lineStartXPixel = -( (int)cursorHPos + PIX_H_MOVE_INC - (int)dispWin.GetTextAreaWidth()
				       + (int)GXTEXTWIN_DEFAULT_XSTART );
	      };
      };

  unsigned cursorLineNum = (*cursorLinePos)->lineNum;  
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxTextWin::MakeCursorLineVisible cursorLineNum: " << cursorLineNum << " oldTopLine: " << oldTopLine << endl;
#endif //LIBGX_DEBUG_BUILD

  if(numLines <= numViewableLines)
    {
      startingDrawnLinePlace = lineList.begin();
      return;
    };

  //if the cursor line is visible do nothing
  if(cursorLineNum >= oldTopLine && cursorLineNum < oldTopLine + numViewableLines)
    {
#ifdef LIBGX_DEBUG_BUILD
      cout << "GxTextWin::MakeCursorLineVisible  case 1" << endl;
#endif //LIBGX_DEBUG_BUILD
      startingDrawnLinePlace = lineList.begin();
      for(unsigned ii = 0; ii < oldTopLine; ii++)
	startingDrawnLinePlace++;
      assert(startingDrawnLinePlace != lineList.end() );
      return;
    };

  if(cursorLineNum == oldTopLine+numViewableLines)
    {
#ifdef LIBGX_DEBUG_BUILD
      cout << "GxTextWin::MakeCursorLineVisible  case 2" << endl;
#endif //LIBGX_DEBUG_BUILD
      //move down one line presumably we are following the cursor after a cr.
      startingDrawnLinePlace = lineList.begin();
      for(unsigned ii = 0; ii < oldTopLine+1; ii++)
	startingDrawnLinePlace++;
      assert(startingDrawnLinePlace != lineList.end() );
      return;
    };

  if(cursorLineNum+1 == oldTopLine)
    {
      //try to move up one line.
#ifdef LIBGX_DEBUG_BUILD
      cout << "GxTextWin::MakeCursorLineVisible  case 3" << endl;
#endif //LIBGX_DEBUG_BUILD
      startingDrawnLinePlace = lineList.begin();
      for(int ii = 0; ii < ((int)oldTopLine)-1; ii++)
	startingDrawnLinePlace++;
      return;
    };

  //try to center the line in the window, if not enough text is previous to allow centering it, start at the first line.

  if( cursorLineNum < numViewableLines/2 )
    {
#ifdef LIBGX_DEBUG_BUILD
      cout << "GxTextWin::MakeCursorLineVisible  case 4" << endl;
#endif //LIBGX_DEBUG_BUILD
      startingDrawnLinePlace  = lineList.begin();
      return;
    };

  //if not enough text follows the cursor line to fill the window, put the last line of text at the bottom of the window.
  if(cursorLineNum + numViewableLines/2 > numLines)
    {
#ifdef LIBGX_DEBUG_BUILD
      cout << "GxTextWin::MakeCursorLineVisible  case 5" << endl;
#endif //LIBGX_DEBUG_BUILD
      startingDrawnLinePlace = lineList.end();
      startingDrawnLinePlace--; //make it valid
      for(unsigned ii = 0; ii < numViewableLines; ii++)
	startingDrawnLinePlace--;
      return;
    }

#ifdef LIBGX_DEBUG_BUILD
  cout << "GxTextWin::MakeCursorLineVisible  case 6" << endl;
#endif //LIBGX_DEBUG_BUILD
  //total success. center the cursor line
  startingDrawnLinePlace = cursorLinePos;
  for(unsigned ii = 0; ii < numViewableLines/2; ii++)
    startingDrawnLinePlace--;
}

void GxTextWin::SetScrollBarFractions(void)
{
  unsigned numLines = lineList.size();
  unsigned int numViewableLines = dispWin.GetTextAreaHeight()/lineHeight;
  unsigned int winHeight = dispWin.GetTextAreaHeight();

  if(numViewableLines >= numLines)
    {
      vScrollBar.SetBothFractions(GX_MAX_FRACTION, GX_MIN_FRACTION);
    }else
      {
	vScrollBar.SetBothFractions( GxFraction(winHeight, numLines*lineHeight),
				     GxFraction( (*startingDrawnLinePlace)->lineNum, (unsigned)(numLines - numViewableLines)) );
      };

  unsigned int winWidth = dispWin.GetTextAreaWidth();
  if(winWidth >= longestLine)
    {
      hScrollBar.SetBothFractions(GX_MAX_FRACTION, GX_MIN_FRACTION);
    }else
      {
	hScrollBar.SetBothFractions( GxFraction(winWidth, longestLine),
				     GxFraction( -(lineStartXPixel - GXTEXTWIN_DEFAULT_XSTART), longestLine-winWidth) );
      };
}

void GxTextWin::GetCursorInfo(std::list<Line*>::iterator &rCursorLinePos, unsigned &rCursorLineHPixPos)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxTextWin::GetCursorInfo" << endl;
#endif //LIBGX_DEBUG_BUILD

  list<Line*>::iterator cPlace = lineList.begin();
  list<Line*>::iterator cEnd = lineList.end();
  while(cPlace != cEnd)
    {
      //move through every line's dcList looking for a displayChunk
      //which mentions cursorPlace's listPlace.
      Line* pCLine = *cPlace;
	  
      if( FindCursorPlaceInLineText(pCLine, rCursorLineHPixPos) )
	{
	  rCursorLinePos = cPlace;
	  return;
	}else
	  { //walk from the last character on the end of the line to the lineEndPlace. note this will happen on _every_ line.
	    list<Line*>::iterator nextPlace = cPlace;
	    nextPlace++;

	    GxTextHunk::TextPlace lineEndPlace;
	    if(nextPlace != cEnd)
	      {
#ifdef LIBGX_DEBUG_BUILD
		assert( !(*nextPlace)->dcList.empty() );
#endif //LIBGX_DEBUG_BUILD
		
		lineEndPlace = (*nextPlace)->dcList.front().startPlace;
		windowText.DecrementTextPlace(lineEndPlace); //one char less than the start of the next line
	      }else
		lineEndPlace = windowText.GetEndPlace();

	    //GxTextHunk::TextPlace cTextPlace = pCLine->GetLastPlace(); //the last valid character printed on the line.
	    GxTextHunk::TextPlace cTextPlace = pCLine->dcList.back().startPlace;
	    //somewhat hackish (slow)
	    if( pCLine->dcList.back().numChars )
	      for(unsigned ii = 0; ii < pCLine->dcList.back().numChars-1; ii++)
		windowText.IncrementTextPlace(cTextPlace);

	    unsigned ii = 0;
	    while(true)
	      {
		//this could very possibly be the end of the text too. because the cursorPos _must_ be valid we don't have
		//to worry going beyond the end of the windowText (except in debug mode)
		//windowText.IncrementTextPlace(cTextPlace);
#ifdef LIBGX_DEBUG_BUILD
		assert( PlaceValid(windowText, cTextPlace ) );
#endif //LIBGX_DEBUG_BUILD
		if(cTextPlace == cursorPos)
		  {
		    rCursorLinePos = cPlace;
		    rCursorLineHPixPos = pCLine->lineLength;
		    return;
		  };
		if( cTextPlace == lineEndPlace ) break; //go on to next line.

		windowText.IncrementTextPlace(cTextPlace); //moved here to avoid problems with zero length display chunks.
		ii++;
#ifdef LIBGX_DEBUG_BUILD
		assert( ii < 5 ); //should not be greater than 1.
#endif //LIBGX_DEBUG_BUILD
	      };
	  };

      cPlace++;
    };
  
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxTextWin::GetCursorInfo failure" << endl;
#endif //LIBGX_DEBUG_BUILD
  assert(false);
}

bool GxTextWin::FindCursorPlaceInLineText(Line *pLine, unsigned &rCursorLineHPixPos) const
{
  //no matter what we must handwalk through every place from the end of the last display chunk to lineEndPlace. the
  //caller is supposed to do this.

  list<DisplayChunk>::const_iterator dPlace = (pLine->dcList).begin();
  list<DisplayChunk>::const_iterator dEnd = (pLine->dcList).end();

  while(true)
    {
      const DisplayChunk &rChunk = *dPlace;
      if(rChunk.startPlace.listPlace != cursorPos.listPlace)
	{
	  dPlace++;
	  if(dPlace == dEnd) return false;
	  continue; //go on to the next chunk
	}else
	  break;
    };

  while(true)
    {
      //if here we know the cursor pos _could be_ in the current chunk. it could also be on the next line.
      //this makes the assumption that the Format() stuff will not break a TextUnit into multiple DisplayChunks
      //on the same line.

      const DisplayChunk &rChunk = *dPlace;
      if(cursorPos.subPlace > rChunk.startPlace.subPlace + rChunk.numChars)
	{
	  dPlace++;
	  if(dPlace == dEnd) return false;
	  continue;
	}else
	  break;
    };

  //we know the cursor is in the current chunk. walk through it to find the cursor xPos
  const DisplayChunk &rChunk = *dPlace;
  unsigned xPix = rChunk.xPixel;

  //if here we will return either the position of the cursor within the chunk, or the position after the last
  //displayed character in the chunk (should not happen)
  unsigned ii = 0;
  unsigned unitStartPlace = rChunk.startPlace.subPlace;
  const char *pStart = rChunk.StartPtr();

  while(true)
    {
      if(cursorPos.subPlace == unitStartPlace + ii)
	{
	  //we found the cursor position.
	  rCursorLineHPixPos = xPix;
	  return true;
	};
#ifdef LIBGX_DEBUG_BUILD
      GxTextHunk::TextPlace tPlace(rChunk.startPlace.listPlace, rChunk.startPlace.subPlace + ii);
      assert( windowText.Dereferenceable(tPlace) );
#endif //LIBGX_DEBUG_BUILD
      
      xPix += GetCharWidth( pStart[ii] );
      ii++;
      if(ii >=  rChunk.numChars)
	{
	  rCursorLineHPixPos = xPix;
	  return true;
	  //cout << "found exexpected end of a chunk" << endl;
	  //assert(false);
	};
    };
  
  //not reachable.
  assert(false);
}


bool GxTextWin::SetCursorPixPos(void)
{
#ifdef LIBGX_DEBUG_BUILD
  assert( !lineList.empty() );
  assert( startingDrawnLinePlace != lineList.end() );
#endif //LIBGX_DEBUG_BUILD

  list<Line*>::iterator cursorLinePlace;
  unsigned cursorLineHPixPos = 0;
  GetCursorInfo(cursorLinePlace, cursorLineHPixPos);

  //don't forget the cursor might be off of the screen. we _could_ only search through the displayed lines for the cursor.
  //it would be much faster. this however is a much faster way (timewise) of correctly implementing this function
  int cursorLineNum = (*cursorLinePlace)->lineNum;
  int startLineNum = (*startingDrawnLinePlace)->lineNum;

  //hack. I don't like this. we should only set the pixel pos if we are a line or to above or below being displayed, otherwise
  //we should move it to some undisplayed pixel location.
  dispWin.cursor.SetPos((int)cursorLineHPixPos+lineStartXPixel, (cursorLineNum-startLineNum+1)*(int)lineHeight);
  return true;
}

void GxTextWin::GetBestCursorPlace(GxTextWin::Line *pLine, int xPos,  GxTextHunk::TextPlace &rPlace)
{
#ifdef LIBGX_DEBUG_BUILD
  assert(pLine);
  assert( !(pLine->dcList).empty() );
#endif //LIBGX_DEBUG_BUILD

  list<DisplayChunk>::const_iterator dPlace = (pLine->dcList).begin();
  list<DisplayChunk>::const_iterator dEnd = (pLine->dcList).end();
  while(true) //safe because we know the line dcList is not empty (cannot be legaly empty)
    {
      const GxTextWin::DisplayChunk &rChunk = *dPlace;
      //we are not allowed to fail so just make sure things are up to date
      rPlace = rChunk.startPlace;

      unsigned xPix = rChunk.xPixel+lineStartXPixel;

      unsigned unitStartPlace = rChunk.startPlace.subPlace;
      const char *pStart = rChunk.StartPtr();
      for(unsigned ii = 0; ii < rChunk.numChars; ii++)
	{
	  unsigned cWidth = GetCharWidth( pStart[ii] );
	  
	  if( (xPos >= (int)xPix) && ( xPos <= (int)(xPix + cWidth)) )
	    {
	      //we end either before or after this character
	      rPlace.listPlace = rChunk.startPlace.listPlace;
	      if( (xPos-xPix) < (cWidth/2) ) //we end before this character
		{
		  rPlace.subPlace = unitStartPlace + ii;
		}else
		  {
		    rPlace.subPlace = unitStartPlace + ii;
		    windowText.IncrementTextPlace(rPlace);
		  };
#ifdef LIBGX_DEBUG_BUILD
	      assert( PlaceValid(windowText, rPlace) );
#endif //LIBGX_DEBUG_BUILD
	      return;
	    };

	  rPlace.subPlace = unitStartPlace + ii;
	  xPix += cWidth;
	};

      list<DisplayChunk>::const_iterator nextPlace = dPlace;
      nextPlace++;
      if(nextPlace == dEnd) break;
      dPlace = nextPlace;
    };

  //we are apparently beyond the last displayable character on the line
  GxTextWin::DisplayChunk &rEndChunk = (pLine->dcList).back();
  if(rEndChunk.numChars == 0 && pLine->dcList.size() == 1) //i.e. we just have a c.r.
    rPlace = rEndChunk.startPlace;
  else
    {
#ifdef LIBGX_DEBUG_BUILD
      assert( PlaceValid(windowText, rEndChunk.startPlace) );
      cout << "right here. dcList.size(): " << pLine->dcList.size() << endl;
#endif //LIBGX_DEBUG_BUILD
      rPlace = rEndChunk.startPlace;
      for(unsigned ii = 0; ii < rEndChunk.numChars; ii++)
	windowText.IncrementTextPlace(rPlace);
#ifdef LIBGX_DEBUG_BUILD
      cout << "beyond end default. endChunk.numChars: " << rEndChunk.numChars << " rPlace.subPlace: " << rPlace.subPlace << endl;
      assert( PlaceValid(windowText, rPlace) );
#endif //LIBGX_DEBUG_BUILD
    };

}

void GxTextWin::GetLinePlace(int yPix, list<Line*>::iterator &rLinePlace)
{
  rLinePlace = lineList.begin();
  unsigned cY = GX_BORDER_WD; //hack. wrong.

  //hack. we should not traverse the entire line list. rather we should start at the first visible line.
  list<Line*>::iterator cPlace = lineList.begin();
  list<Line*>::iterator cEnd = lineList.end();
  while(cPlace != cEnd)
    {
      if( (*cPlace)->lineNum < (*startingDrawnLinePlace)->lineNum)
	{
	  cPlace++;
	  continue;
	};

      if( (yPix >= (int)cY) && (yPix < (int)(cY + lineHeight) ) )
	{
	  rLinePlace = cPlace;
	  return;
	};

      cY += lineHeight;
      cPlace++;
    };

  //this can happen if we click below the last line of text. i.e.
  //if our window is larger than or text needs and we click below it.
  rLinePlace = lineList.end();
}

short GxTextWin::GetCharWidth(unsigned cChar) const
{
  if( dInfo.pDefaultFont->min_char_or_byte2 <= cChar ||
      dInfo.pDefaultFont->max_char_or_byte2 >= cChar )
    {
      return (dInfo.pDefaultFont->per_char[cChar]).width;
    }else
      {
#ifdef LIBGX_DEBUG_BUILD
	cout << "char: " << cChar << " is out of range" << endl;
#endif //LIBGX_DEBUG_BUILD
	return 0;
      };  
}

// ****************** start GxKeyHandler overloads ******************

#include <string>
void GxTextWin::HandleKeyString(const char *pBuffer, unsigned long buffLen)
{
  if(textGrabbed || !editable) return;
  if(!buffLen) return;

  //cout << "got key string: " << string(pBuffer, buffLen) << endl;
  windowText.AppendText(cursorPos, pBuffer, buffLen);
  modifyCB();

  //windowText.GetText(cout);
  //cout << endl;
  Format(true);
  //SetCursorPixPos();
  //ViewCursor();
  dispWin.ClearAndDisplay();
}

void GxTextWin::KeyLeft(void)
{
  if(textGrabbed || !editable) return;

  if( windowText.DecrementTextPlace(cursorPos) )
    PosKeyUtility();
}

void GxTextWin::KeyRight(void)
{
  if(textGrabbed || !editable) return;

  if(windowText.IncrementTextPlace(cursorPos) )
    PosKeyUtility();
}

void GxTextWin::KeyUp(void)
{
  //cout << "GxTextWin::KeyUp" << endl;
  if(textGrabbed || !editable) return;

  list<Line*>::iterator linePlace;
  unsigned lineHPos;
  GetCursorInfo(linePlace, lineHPos);

  if(linePlace == lineList.begin() ) return;
#ifdef LIBGX_DEBUB_BUILD
  assert( linePlace != lineList.end() );
#endif //LIBGX_DEBUG_BUILD

  linePlace--;

#ifdef LIBGX_DEBUG_BUILD
  cout << "GxTextWin::KeyUp valid key up" << endl;
#endif //LIBGX_DEBUG_BUILD
  Line *pLine = *linePlace;

  GetBestCursorPlace(pLine, ((int)lineHPos + lineStartXPixel), cursorPos); //the same hpos on a new line

  PosKeyUtility();
}

void GxTextWin::KeyDown(void)
{
  if(textGrabbed || !editable) return;

  list<Line*>::iterator linePlace;
  unsigned lineHPos;
  GetCursorInfo(linePlace, lineHPos);

  linePlace++;
  if( linePlace == lineList.end() ) return;
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxTextWin::KeyDown valid key down" << endl;
#endif //LIBGX_DEBUG_BUILD
  Line *pLine = *linePlace;

  GetBestCursorPlace(pLine, ((int)lineHPos + lineStartXPixel), cursorPos); //the same hpos on a new line

  PosKeyUtility();
}

void GxTextWin::KeyDelete(void)
{
  if(textGrabbed || !editable) return;

  //cursorPos; start place
  GxTextHunk::TextPlace endPlace;
  windowText.GetPlaceSpaced(cursorPos, 1, endPlace);

#ifdef LIBGX_DEBUG_BUILD
  string textBetween;
  windowText.GetTextBetween(cursorPos, endPlace, textBetween);
  cout << "GxTextWin::KeyDelete textBetween: \"" << textBetween << "\""<< endl;
#endif //LIBGX_DEBUG_BUILD

  windowText.DeleteTextBetween(cursorPos, endPlace);
  modifyCB(); //hack. we might not be doing anything here!

  Format(true); //keep the cursor visible.
  SetCursorPixPos();
  dispWin.ClearAndDisplay();  
}

void GxTextWin::KeyBackspace(void)
{
  if(textGrabbed || !editable) return;

  GxTextHunk::TextPlace startPlace;
  //cursorPos; end place
  windowText.GetPlaceSpaced(cursorPos, -1, startPlace);
  
#ifdef LIBGX_DEBUG_BUILD
  string textBetween;
  windowText.GetTextBetween(startPlace, cursorPos, textBetween);
  cout << "GxTextWin::KeyBackspace textBetween: \"" << textBetween << "\""<< endl;
#endif //LIBGX_DEBUG_BUILD

  windowText.DeleteTextBetween(startPlace, cursorPos);
  modifyCB(); //hack. we might not be doing anything here!

  Format(true); //keep the cursor visible.
  SetCursorPixPos();
  dispWin.ClearAndDisplay();  
}

void GxTextWin::KeyEnter(void)
{
  HandleKeyString("\n", 1);
}

// ****************** end GxKeyHandler overloads ******************

void GxTextWin::PosKeyUtility(void)
{
  unsigned oldTopLine = (*startingDrawnLinePlace)->lineNum;
  int oldXOffset = lineStartXPixel;
  MakeCursorVisible( oldTopLine );
  bool redrawAll = (oldTopLine != (*startingDrawnLinePlace)->lineNum) || (oldXOffset != lineStartXPixel);
  
  SetScrollBarFractions();

  dispWin.cursor.Draw(false);
  SetCursorPixPos();
  if(redrawAll)
    dispWin.ClearAndDisplay();
  else
    DrawDisplayWin(); //hack: redraw only current lines
}

void GxTextWin::DrawDisplayWin(void)
{
  int cY = GX_BORDER_WD+1; //hack.
  unsigned cLineNum = 0;

  //cout << "GxTextWin::DrawDisplayWin start x: " << lineStartXPixel << endl;

  list<Line*>::const_iterator cPlace = startingDrawnLinePlace;
  list<Line*>::const_iterator cEnd = lineList.end();
  while(cPlace != cEnd)
    {
      const Line *pLine = *cPlace;
      pLine->Display(dispWin.lInfo, lineStartXPixel, cY);
      cY += dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent;

      if((unsigned)cY > height) break;

      cLineNum++;
      cPlace++;
    };

  //handle highlighting any selected text.

  if(editable)
    dispWin.cursor.Draw(true);

  dispWin.DrawBorder();
}

void GxTextWin::HandleClick(const CCoord &rClickCoord, int type)
{
  //we must know if we are locked before using this.
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxTextWin::HandleClick" << endl;
#endif //LIBGX_DEBUG_BUILD

  if(!haveFocus && editable)
    {
      AcceptFocus(rClickCoord.time);
    };

  list<Line*>::iterator linePlace;
  GetLinePlace(rClickCoord.yC, linePlace);
  selectStartLineListPlace = linePlace;

  GxTextHunk::TextPlace newCursorPos;
  if( linePlace != lineList.end() )
    GetBestCursorPlace( *linePlace, rClickCoord.xC, newCursorPos);
  else
    newCursorPos = windowText.GetEndPlace();

  if( !windowText.Equal(newCursorPos, cursorPos) )
    {
      dispWin.cursor.Draw(false);
      cursorPos = newCursorPos;
      SetCursorPixPos();
      dispWin.cursor.Draw(true);
    };

  selectStartTextPlace = cursorPos;
  selectStartLineListPlace = linePlace;
}

void GxTextWin::HandleClickDragMotion(const CCoord &rMotionCoord)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxTextWin::HandleClickDragMotion unimplemented" << endl;
#endif //LIBGX_DEBUG_BUILD
  return;
  /*
  list<Line*>::iterator cLinePlace;
  GxTextHunk::TextPlace cTextPlace;

  GetLinePlace(rMotionCoord.yC, cLinePlace);

  if( cLinePlace != lineList.end() )
    GetCursorPos( *cLinePlace, rMotionCoord.xC, cTextPlace);
  else
    cTextPlace = windowText.GetEndPlace();

  //at this point we have the current textPlace and the current line.
  //we must potientially scroll the window to position the cursor in a visible
  //reagon.  We must clear any pre-existing hightlight marks, reset the highlight
  //marks to cover the relevant regon, and redraw the window.

  //the simplest way to clear any highlight marks is to traverse the entire line list
  //resetting everything.  Unfortunatly this is both slower than computing changes to
  //the line list and _much_ easier.  For now I pick the easy way.
  list<Line*>::iterator cPlace = lineList.begin();
  list<Line*>::iterator cEnd = lineList.end();
  while(cPlace != cEnd)
    {
      (*cPlace)->ClearHighlight();
      cPlace++;
    };
  
  //ok. now for the hard part. we must actually determine the hightlighed sections.
  //we must determine whether the cLinePlace is before or after the startSelectionLine

  //reusing cPlace and cEnd
  if(selectStartLineNum <= cursorLineNum)
    {
      cPlace = selectStartLineListPlace;
      cEnd = cLinePlace;
    }else
      {
	cPlace = cLinePlace;
	cEnd = selectStartLineListPlace;
      };

  while(cPlace != cEnd)
    {
      cout << "highlighting line" << endl;
      //horrible
      (*cPlace)->hlStart = (*cPlace)->dcList.front().startPlace;
      (*cPlace)->dcList.back().GetEndPlace( (*cPlace)->hlEnd );
      cPlace++;
    };

  dispWin.ClearAndDisplay();  
  */
}

void GxTextWin::HandleFocusChange(bool focusStat)
{
  //we should not have children, but handle it gracefully
  //if(!haveFocus) break;
  haveFocus = focusStat;
}

void GxTextWin::ClearLines(void)
{
  while( !lineList.empty() )
    {
      GxTextWin::Line *pCLine = lineList.front();
      lineList.pop_front();
      delete pCLine;
    };

  startingDrawnLinePlace = lineList.begin();
  selectStartLineListPlace = lineList.end();
}

void GxTextWin::SetTextGrabbed(bool newStat)
{
  dispWin.freezeEvents = newStat;
  textGrabbed = newStat;
}

#ifdef LIBGX_DEBUG_BUILD
void CheckValid(GxTextWin &rWin)
{
  assert( Valid(rWin.windowText) );
  //hack.  assert( PlaceValid(windowText, selectStartTextPlace) );
  assert( PlaceValid(rWin.windowText, rWin.cursorPos) );

  
  CheckLinePlaceValid(rWin, rWin.startingDrawnLinePlace, false );
  //  CheckLinePlaceValid( selectStartLineListPlace, true );
  assert( !rWin.lineList.empty() );
  list<GxTextWin::Line*>::iterator cPlace = rWin.lineList.begin();
  list<GxTextWin::Line*>::iterator cEnd = rWin.lineList.end();
  while(cPlace != cEnd)
    {
      assert( !(*cPlace)->dcList.empty() );
      cPlace++;
    };
}

void CheckLinePlaceValid(GxTextWin &rWin, std::list<GxTextWin::Line*>::iterator desPlace, bool endOk)
{
  list<GxTextWin::Line*>::iterator cPlace = rWin.lineList.begin();
  list<GxTextWin::Line*>::iterator cEnd = rWin.lineList.end();

  if(endOk)
    if(desPlace == cEnd) return; //theoretically must belong to same container for this to be true. not saying much.

  while(cPlace != cEnd)
    {
      if(desPlace == cPlace) return;
      cPlace++;
    };

  assert(false);
}
#endif //LIBGX_DEBUG_BUILD
