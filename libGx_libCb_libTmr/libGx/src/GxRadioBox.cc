#include <libGx/GxRadioBox.hh>

#include "GxDefines.hh"

GxRadioButton::GxRadioButton(GxRadioBox *pOwner, const char *pLabel) :
  GxToggleButton(pOwner, pLabel)
{
  pOwner->AddToggleButton(this);
}

GxRadioButton::~GxRadioButton(void)
{}

void GxRadioButton::State(bool newState)
{
  if(state == newState)
    return;

  state = newState;
  
  if(newState == true)
    ((GxRadioBox*)pWinAreaOwner)->ChildActivated(this);

  if(Created())
    {
      //XClearWindow?
      DrawButton();
    };
}

void GxRadioButton::CutOff(void)
{
  if(state == false)
    return;

  state = false;
  if( Created() )
    {
      //XClearWindow?
      DrawButton();
    };
}

//each radio button can only cut itself on. it relies on the RadioBox
//to cut it off
void GxRadioButton::DoAction(void)
{
  if(!state)
    {
      //this is a safe cast;
      ((GxRadioBox*)pWinAreaOwner)->ChildActivated(this);
      state = true;
      DrawButton();
      cb();
    };
}

GxRadioBox::GxRadioBox(GxRealOwner *pOwner) :
  GxOwnerWin(pOwner), radio(true), numPer(2), order(true)
{}

GxRadioBox::~GxRadioBox(void)
{}

void GxRadioBox::SetRadioBehavior(bool newStat)
{
  radio = newStat;
}

void GxRadioBox::SetNum(UINT numberPer)
{
  numPer = numberPer;
}

void GxRadioBox::SetFormating(bool format)
{
  order = format;
}

void GxRadioBox::SetActive(bool nActive)
{
  std::list<GxRadioButton*>::iterator cPlace = radioList.begin();
  std::list<GxRadioButton*>::iterator cEnd = radioList.end();

  while(cPlace != cEnd)
    {
      (*cPlace)->SetActive(nActive);
      cPlace++;
    };
}

void GxRadioBox::RemoveChild(GxWinArea *pChild)
{
  GxOwner::RemoveChild(pChild);
  std::list<GxRadioButton*>::iterator cPlace = radioList.begin();
  std::list<GxRadioButton*>::iterator cEnd = radioList.end();

  while(cPlace != cEnd)
    {
      if(pChild == *cPlace)
	{
	  cPlace = radioList.erase(cPlace);
	  return;
	};
      cPlace++;
    };
}

void GxRadioBox::AddToggleButton(GxRadioButton *pNewButton)
{
  radioList.push_back(pNewButton);
}

//hack none of these take into account the posibility that we have a single
//row or column with less than numPer objects on a column or row.
UINT GxRadioBox::GetDesiredWidth(void) const
{
  if( radioList.empty() ) return GX_BORDER_WD; //we are a x window after all

  if(order)
    {
      //things are arranged into nice neat columns, we therefore just cannot
      //consider the widest row
      UINT widestPerColumn[numPer];
      for(UINT i = 0; i < numPer; i++)
	widestPerColumn[i] = 0;

      std::list<GxRadioButton*>::const_iterator cPlace = radioList.begin();
      std::list<GxRadioButton*>::const_iterator cEnd = radioList.end();
      while(cPlace != cEnd)
	{
	  for(UINT i = 0; i < numPer; i++)
	    {
	      UINT dWidth = (*cPlace)->GetDesiredWidth();
	      
	      if(dWidth > widestPerColumn[i])
		widestPerColumn[i] = dWidth;

	      cPlace++;
	      if(cPlace == cEnd) break;
	    };
	};

      UINT totalWidth = 0;
      for(UINT i = 0; i < numPer; i++)
	totalWidth += widestPerColumn[i];

      if(numPer > 1)
	return totalWidth + (numPer-1)*2*GX_SPACE_INC;
      else
	return totalWidth;
    }else //arranged into columns
      {
	UINT totalWidth = 0;
	std::list<GxRadioButton*>::const_iterator cPlace = radioList.begin();
	std::list<GxRadioButton*>::const_iterator cEnd = radioList.end();
	while(cPlace != cEnd)
	{
	  UINT cColumnWidth = 1;
	  for(UINT i = 0; i < numPer; i++)
	    {
	      UINT cWidth = (*cPlace)->GetDesiredWidth();
	      if(cWidth > cColumnWidth)
		cColumnWidth = cWidth;

	      cPlace++;
	      if(cPlace == cEnd) break;
	    };      
	  
	  //update the total width with the width of this column
	  totalWidth += cColumnWidth;

	  //?hack? don't we break to beyond this if cPlace == cEnd
	  if(cPlace != cEnd) //there is annother column
	    totalWidth += GX_SPACE_INC;
	  else
	    break;
	};

	return totalWidth;
      };
}

UINT GxRadioBox::GetDesiredHeight(void) const
{
  if( radioList.empty() ) return GX_BORDER_WD; //we are a x window after all

  if(order)
    {
      UINT tHeight = 1; //the total height
      std::list<GxRadioButton*>::const_iterator cPlace = radioList.begin();
      std::list<GxRadioButton*>::const_iterator cEnd = radioList.end();
      while(1) //if there wasn't a first node we wouldn't be here
	{
	  UINT tallestLineItem = 1;
	  for(UINT ii = 0; ii < numPer; ii++)
	    {
	      UINT cHeight = (*cPlace)->GetDesiredHeight();
	      
	      if(cHeight > tallestLineItem)
		tallestLineItem = cHeight;
	      
	      cPlace++;
	      if(cPlace == cEnd) break;
	    };      

	  tHeight += tallestLineItem;
	  if(cPlace != cEnd) //there is annother row
	    tHeight += GX_SPACE_INC;
	  else
	    break;
	};
      return tHeight;
    }else //arranged into columns
      {
	//we are arranged into nice neat rows, so cannot just consider the
	//total column heights.
	UINT tallestPerRow[numPer];
	for(UINT ii = 0; ii < numPer; ii++)
	  tallestPerRow[ii] = 0;

	std::list<GxRadioButton*>::const_iterator cPlace = radioList.begin();
	std::list<GxRadioButton*>::const_iterator cEnd = radioList.end();
	while(cPlace != cEnd)
	  {
	    for(UINT ii = 0; ii < numPer; ii++)
	      {
		UINT dHeight = (*cPlace)->GetDesiredHeight();

		if(dHeight > tallestPerRow[ii])
		  tallestPerRow[ii] = dHeight;

		cPlace++;
		if(cPlace == cEnd) break;
	      };
	  };

	UINT tWidth = 0;
	for(UINT ii = 0; ii < numPer; ii++)
	  tWidth += tallestPerRow[ii];

	if(numPer > 1)
	  return tWidth + (numPer-1)*GX_SPACE_INC;
	else
	  return tWidth;
      };
}

void GxRadioBox::PlaceChildren(void)
{
  if( radioList.empty() ) return;
  UINT numItems = radioList.size(); //we're here so we know there is at least 1

  if(order) //formating in rows
    {
#ifdef LIBGX_DEBUG_BUILD
      std::cout << "in GxRadioBox::PlaceChildren " << width << "," << height << std::endl;
#endif //LIBGX_DEBUG_BUILD

      UINT numLines = numItems/numPer;
      if(numItems%numPer)
	numLines++;
      
      UINT lineHeights[numLines];
      for(UINT ii = 0; ii < numLines; ii++)
	lineHeights[ii] = 1;
      
      UINT widestPerColumn[numPer];
      for(UINT ii = 0; ii < numPer; ii++)
	widestPerColumn[ii] = 1;
      
      UINT cLine = 0;
      UINT packedHeight = 0;
      std::list<GxRadioButton*>::iterator cPlace = radioList.begin();
      std::list<GxRadioButton*>::iterator cEnd = radioList.end();
      while(cPlace != cEnd)
	{
	  UINT tallestHeight = 0;
	  for(UINT ii = 0; ii < numPer; ii++)
	    {
	      UINT cWidth = (*cPlace)->GetDesiredWidth();
	      UINT cHeight = (*cPlace)->GetDesiredHeight();
	      
	      if(cHeight > tallestHeight)
		tallestHeight = cHeight;
	      
	      if(cWidth > widestPerColumn[ii])
		widestPerColumn[ii] = cWidth;

	      cPlace++;
	      if(cPlace == cEnd) break;
	    };      

	  lineHeights[cLine] = tallestHeight;
	  packedHeight += tallestHeight;
	  cLine++;
	};

      //if we have a larger width or height than we need, balance out
      //the extra space
      UINT neededHeight;
      if(numLines > 1)
	neededHeight = packedHeight + (numLines-1)*GX_SPACE_INC;
      else
	neededHeight = packedHeight;

#ifdef LIBGX_DEBUG_BUILD
      std::cout << "GxRadioBox::PlaceChildren packedHeight: "
	   << packedHeight << std::endl;
      std::cout << "GxRadioBox::PlaceChildren neededHeight: "
	   << neededHeight << std::endl;
#endif //LIBGX_DEBUG_BUILD

      UINT vGap;
      if(height <= neededHeight)
	vGap = GX_SPACE_INC; //we will be cliping children if <
      else
	if(numLines > 1)
	  vGap = (height-packedHeight)/(numLines-1);
      //else //won't be using vGap anyway
      //vGap = 0;

#ifdef LIBGX_DEBUG_BUILD
      std::cout << "GxRadioBox::PlaceChildren vGap == " << vGap << std::endl;
#endif //LIBGX_DEBUG_BUILD

      UINT packedWidth = 0;
      for(UINT ii = 0; ii < numPer; ii++)
	packedWidth += widestPerColumn[ii];

      UINT neededWidth;
      if(numPer > 1)
	neededWidth = packedWidth + (numPer-1)*GX_SPACE_INC;
      else
	neededWidth = packedWidth;

      UINT hGap;
      if(width <= neededWidth)
	hGap = GX_SPACE_INC; //we will be cliping children if <
      else
	if(numPer > 1)
	  hGap = (width-packedWidth)/(numPer-1);

      int cX;
      int cY = 0;
      cLine = 0;
      cPlace = radioList.begin();
      cEnd = radioList.end();
      while(1)
	{
	  cX = 0;
	  for(UINT ii = 0; ii < numPer; ii++)
	    {
	      UINT cWidth = (*cPlace)->GetDesiredWidth();
	      UINT cHeight = (*cPlace)->GetDesiredHeight();
	      
	      //GX_FLOW_LEFT
	      int lX = cX;
	      int rX = cX + cWidth;
	      //we want these on the baseline of each row. GX_FLOW_DOWN
	      int tY = cY;
	      int bY = cY + cHeight;
	      //we don't pass in cX or cY because the child could modify them
	      (*cPlace)->Place(lX, rX, tY, bY);
	      
	      cPlace++;
	      if(cPlace == cEnd)
		break;
	      else
		cX += widestPerColumn[ii] + hGap;
	    };      
	  
	  if(cPlace == cEnd) return;
	  cY += lineHeights[cLine] + vGap;
	  cLine++;
	};

      return; //should not happen
    };

  //if we're here we are formating in columns
  UINT numColumns = numItems/numPer;
  if(numItems%numPer)
    numColumns++;
      
  UINT columnWidths[numColumns];
  for(UINT ii = 0; ii < numColumns; ii++)
    columnWidths[ii] = 1;
  
  UINT tallestPerRow[numPer];
  for(UINT ii = 0; ii < numPer; ii++)
    tallestPerRow[ii] = 1;
      
  UINT cColumn = 0;
  UINT packedWidth = 0;
  std::list<GxRadioButton*>::iterator cPlace = radioList.begin();
  std::list<GxRadioButton*>::iterator cEnd = radioList.end();
  while(cPlace != cEnd)
    {
      UINT widestWidth = 0;
      for(UINT ii = 0; ii < numPer; ii++)
	{
	  UINT cWidth = (*cPlace)->GetDesiredWidth();
	  UINT cHeight = (*cPlace)->GetDesiredHeight();
	  
	  if(cWidth > widestWidth)
	    widestWidth = cWidth;
	  
	  if(cHeight > tallestPerRow[ii])
	    tallestPerRow[ii] = cHeight;
	  
	  cPlace++;
	  if(cPlace == cEnd) break;
	};      
      
      columnWidths[cColumn] = widestWidth;
      packedWidth += widestWidth;
      cColumn++;
    };

  //if we have a larger width or height than we need, balance out
  //the extra space
  UINT neededWidth;
  if(numColumns > 1)
    neededWidth = packedWidth + (numColumns-1)*GX_SPACE_INC;
  else
    neededWidth = packedWidth;
  
  UINT hGap;
  if(width <= neededWidth)
    hGap = GX_SPACE_INC; //we will be cliping children if <
  else
    if(numColumns > 1)
      hGap = (width-packedWidth)/(numColumns-1);
  //else //won't be using hGap anyway
  //vGap = 0;

  UINT packedHeight = 0;
  for(UINT ii = 0; ii < numPer; ii++)
    packedHeight += tallestPerRow[ii];
  
  UINT neededHeight;
  if(numPer > 1)
    neededHeight = packedHeight + (numPer-1)*GX_SPACE_INC;
  else
    neededHeight = packedHeight;
  
  UINT vGap;
  if(height <= neededHeight)
    vGap = GX_SPACE_INC; //we will be cliping children if <
  else
    if(numPer > 1)
      vGap = (height-packedHeight)/(numPer-1);
  
  int cX = 0;
  int cY;
  cColumn = 0;
  cPlace = radioList.begin();
  cEnd = radioList.end();
  while(1)
    {
      cY = 0;
      for(UINT ii = 0; ii < numPer; ii++)
	{
	  UINT cWidth = (*cPlace)->GetDesiredWidth();
	  UINT cHeight = (*cPlace)->GetDesiredHeight();
	      
	  //GX_FLOW_LEFT
	  int lX = cX;
	  int rX = cX + cWidth;
	  //we want these on the baseline of each row. GX_FLOW_DOWN
	  int tY = cY;
	  int bY = cY + cHeight;
	  //we don't pass in cX or cY because the child could modify them
	  (*cPlace)->Place(lX, rX, tY, bY);
	  
	  cPlace++;
	  if(cPlace == cEnd)
	    break;
	  else
	    cY += tallestPerRow[ii] + vGap;
	};      
      
      if(cPlace == cEnd) return;
      cX += columnWidths[cColumn] + hGap;
      cColumn++;
    };
  
  return;
}

void GxRadioBox::ChildActivated(GxRadioButton *pActivatedChild)
{
  std::list<GxRadioButton*>::iterator cPlace = radioList.begin();
  std::list<GxRadioButton*>::iterator cEnd = radioList.end();
  while(cPlace != cEnd)
    {
      if(*cPlace != pActivatedChild)
	(*cPlace)->CutOff();

      cPlace++;
    };  
}
