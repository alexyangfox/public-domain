#include <libGx/GxRow.hh>

#include "GxDefines.hh"

GxRow::GxRow(GxRealOwner *pOwner) :
  GxGhost(pOwner), hStacking(GX_STACK_LEFT), vStacking(GX_STACK_V_CEN),
  wIdentical(false), hIdentical(false), aGap(0)
{}

GxRow::~GxRow(void)
{}

UINT GxRow::GetDesiredWidth(void) const
{
  if( childList.empty() ) return 0; //we are a ghost

  UINT nWidth = 0;

  if(wIdentical)
    {
      UINT widestWidth = 0;
      UINT borderSum = 0;
      UINT numChildren = 0;
      
      std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
      std::list<GxWinArea*>::const_iterator cEnd = childList.end();
      while(cPlace != cEnd)
	{
	  numChildren++;
	  	  
	  widestWidth = ((*cPlace)->GetDesiredWidth() > widestWidth) ?
	    ( (*cPlace)->GetDesiredWidth() ) : widestWidth;

	  borderSum += (*cPlace)->LBorder() + (*cPlace)->RBorder();

	  cPlace++;
	};
      nWidth = widestWidth*numChildren + borderSum;
    }else
      {  
	UINT numChildren = 0;
	std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
	std::list<GxWinArea*>::const_iterator cEnd = childList.end();
	while(cPlace != cEnd)
	  {
	    numChildren++;

	    nWidth += (*cPlace)->GetDesiredWidth() +
	      (*cPlace)->LBorder() + (*cPlace)->RBorder();

	    cPlace++;
	  };

	if(numChildren != 1)
	  {
	    nWidth += aGap*(numChildren-1);
	  }
      };


  return nWidth;
}

UINT GxRow::GetDesiredHeight(void) const
{
  if( childList.empty() ) return 0; //we are a ghost

  UINT nHeight = 0;

  std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
  std::list<GxWinArea*>::const_iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      UINT tHeight = (*cPlace)->GetDesiredHeight() +
	(*cPlace)->TBorder() + (*cPlace)->BBorder();

      nHeight = (tHeight > nHeight) ? tHeight : nHeight;
      
      cPlace++;
    };

  return nHeight;
}

void GxRow::SetHStacking(GX_H_STACKING newHStacking)
{
  hStacking = newHStacking;
}

void GxRow::SetVStacking(GX_V_STACKING newVStacking)
{
  vStacking = newVStacking;
}

void GxRow::SetWidthIdentical(bool newStat)
{
  wIdentical = newStat;
}

void GxRow::SetHeightIdentical(bool newStat)
{
  hIdentical = newStat;
}

void GxRow::SetAdditionalGap(UINT additionalGap)
{
  aGap = additionalGap*GX_SPACE_INC;
}

void GxRow::PlaceChildren(void)
{
  if( childList.empty() ) return;

  //calculating the total width needed for the row of children
  UINT nWidth = 0;
  UINT numChildren = 0;

  //the widestWidth and tallestHeight of an object excluding borders
  UINT widestW = 0;
  UINT tallestH = 0;

  //the widest width of an object, borders considered
  UINT tallestTotal = 0;

  if(wIdentical)
    {
      UINT hBorderSum = 0;
      std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
      std::list<GxWinArea*>::const_iterator cEnd = childList.end();
      while(cPlace != cEnd)
	{
	  numChildren++;

	  hBorderSum += (*cPlace)->LBorder() +
	    (*cPlace)->RBorder();

	  UINT tHeight = (*cPlace)->GetDesiredHeight();
	  UINT tWidth = (*cPlace)->GetDesiredWidth();
	  tallestH = (tHeight > tallestH) ? tHeight : tallestH;
	  widestW = (tWidth > widestW) ? tWidth : widestW;

	  UINT otHeight = tHeight + (*cPlace)->TBorder() +
	    (*cPlace)->BBorder();
	  tallestTotal = (otHeight > tallestTotal) ? otHeight : tallestTotal;

	  cPlace++;
	};

      nWidth = widestW*numChildren + hBorderSum;
    }else
      {
	std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
	std::list<GxWinArea*>::const_iterator cEnd = childList.end();
	while(cPlace != cEnd)
	  {
	    numChildren++;

	    UINT tHeight = (*cPlace)->GetDesiredHeight();
	    UINT tWidth = (*cPlace)->GetDesiredWidth();
	    tallestH = (tHeight > tallestH) ? tHeight : tallestH;
	    widestW = (tWidth > widestW) ? tWidth : widestW;
	    
	    UINT otHeight = tHeight + (*cPlace)->TBorder() +
	      (*cPlace)->BBorder();
	    tallestTotal = (otHeight > tallestTotal) ? otHeight : tallestTotal;
	    
	    nWidth += tWidth + (*cPlace)->LBorder() + (*cPlace)->RBorder();

	    cPlace++;
	  };
      };

  UINT availHeight = height;

  UINT availWidth = width;

  int cX, cY;
  if(numChildren == 1)
    {
      GxWinArea *pChild = childList.front();
      if(availWidth < nWidth)
	cX = x;
      else
	cX = x + ((availWidth - nWidth)/2);

      int boxWidth;
      if(wIdentical)
	boxWidth = widestW + pChild->LBorder() + pChild->RBorder();
      else
	boxWidth = pChild->GetDesiredWidth() +
	  pChild->LBorder() + pChild->RBorder();

      int boxHeight;
      if(hIdentical)
	boxHeight = tallestH + pChild->TBorder() + pChild->BBorder();
      else
	boxHeight = pChild->GetDesiredHeight() +
	  pChild->TBorder() + pChild->BBorder();
	
      switch(vStacking)
	{
	case GX_STACK_TOP:
	  cY = y;
	  break;
	case GX_STACK_BOTTOM:
	  cY = y + height - boxHeight;
	  break;
	default: //GX_STACK_V_CEN
	  if(boxHeight > (int)availHeight)
	    cY = y - ((boxHeight - availHeight)/2);
	  else
	    cY = y + ((availHeight - boxHeight)/2);
	};

      //widestW and tallestH are the desired sizes of this object, so
      //reuse them + the borders
      int tX = cX + boxWidth;
      int tY = cY + boxHeight;
      pChild->Place(cX, tX, cY, tY);
      return;
    };

  //the gab between each item.
  //note: we took care of the eventuallity that the denominator could be zero
  UINT hGap = aGap;
  if(availWidth > nWidth)
    hGap = (availWidth - nWidth)/(numChildren - 1);

  cX = x;
  std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
  std::list<GxWinArea*>::const_iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      int boxWidth;
      if(wIdentical)
	boxWidth = widestW + (*cPlace)->LBorder() + (*cPlace)->RBorder();
      else
	boxWidth = (*cPlace)->GetDesiredWidth() +
	  (*cPlace)->LBorder() + (*cPlace)->RBorder();

      int boxHeight;
      if(hIdentical)
	boxHeight = tallestH + (*cPlace)->TBorder() + (*cPlace)->BBorder();
      else
	boxHeight = (*cPlace)->GetDesiredHeight() +
	  (*cPlace)->TBorder() + (*cPlace)->BBorder();
	
      switch(vStacking)
	{
	case GX_STACK_TOP:
	  cY = y;
	  break;
	case GX_STACK_BOTTOM:
	  cY = y + height - boxHeight;
	  break;
	default: //GX_STACK_V_CEN
	  if(boxHeight > (int)availHeight)
	    cY = y - ((boxHeight - availHeight)/2);
	  else
	    cY = y + ((availHeight - boxHeight)/2);
	};

      //widestW and tallestH are the desired sizes of this object, so
      //reuse them + the borders
      int tX = cX + boxWidth;
      int tY = cY + boxHeight;
      (*cPlace)->Place(cX, tX, cY, tY);

      cX += boxWidth + hGap;
      cPlace++;
    };
}


// ************************** start GxThinRow *************************
GxThinRow::GxThinRow(GxRealOwner *pOwner) :
  GxGhost(pOwner)
{}

GxThinRow::~GxThinRow(void)
{}

UINT GxThinRow::GetDesiredWidth(void) const
{
  if( childList.empty() ) return 0; //we are a ghost

  UINT nWidth = 0;
  std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
  std::list<GxWinArea*>::const_iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      nWidth += (*cPlace)->GetDesiredWidth() +
	(*cPlace)->LBorder() + (*cPlace)->RBorder();
      
      cPlace++;
    };

  return nWidth;
}

UINT GxThinRow::GetDesiredHeight(void) const
{
  if( childList.empty() ) return 0; //we are a ghost

  UINT nHeight = 0;

  std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
  std::list<GxWinArea*>::const_iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      UINT tHeight = (*cPlace)->GetDesiredHeight() +
	(*cPlace)->TBorder() + (*cPlace)->BBorder();

      nHeight = (tHeight > nHeight) ? tHeight : nHeight;
      
      cPlace++;
    };

  return nHeight;
}
