#include <libGx/GxColumn.hh>

#include "GxDefines.hh"

GxColumn::GxColumn(GxRealOwner *pOwner) :
  GxGhost(pOwner), aGap(0), hStacking(GX_STACK_LEFT),
  vStacking(GX_STACK_V_CEN), wIdentical(false), hIdentical(false)
{}

GxColumn::~GxColumn(void)
{}

UINT GxColumn::GetDesiredWidth(void) const
{
  if( childList.empty() ) return 0; //we are a ghost

  UINT nWidth = 0;

  std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
  std::list<GxWinArea*>::const_iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      GxWinArea *pChild = *cPlace;
 
      UINT tWidth = pChild->GetDesiredWidth() + pChild->LBorder() +
	pChild->RBorder();

      nWidth = (tWidth > nWidth) ? tWidth : nWidth;
      
      cPlace++;
    };

  return nWidth;
}

UINT GxColumn::GetDesiredHeight(void) const
{
  if( childList.empty() ) return 0; //we are a ghost

  UINT nHeight = 0;

  if(hIdentical)
    {
      UINT numChildren = 0;
      UINT tallestH = 0;
      UINT borderSum = 0;
      std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
      std::list<GxWinArea*>::const_iterator cEnd = childList.end();
      while(cPlace != cEnd)
	{
	  numChildren++;
	  GxWinArea *pChild = *cPlace;

	  borderSum += pChild->TBorder() + pChild->BBorder();

	  UINT tHeight = pChild->GetDesiredHeight();
	  tallestH = (tHeight > tallestH) ? tHeight : tallestH;
      
	  cPlace++;
	};

      if(numChildren == 1)
	nHeight = numChildren*tallestH + borderSum;
      else
	nHeight = numChildren*tallestH + borderSum + aGap*(numChildren-1);
    }else
      {
	UINT numChildren = 0;
	std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
	std::list<GxWinArea*>::const_iterator cEnd = childList.end();
	while(cPlace != cEnd)
	  {
	    numChildren++;

	    nHeight += (*cPlace)->GetDesiredHeight() +
	      (*cPlace)->TBorder() + (*cPlace)->BBorder();
      
	    cPlace++;
	  };

	if(numChildren != 1)
	  nHeight += aGap*(numChildren-1);
      };

  return nHeight;
}

void GxColumn::SetHStacking(GX_H_STACKING newHStacking)
{
  hStacking = newHStacking;;
}

void GxColumn::SetVStacking(GX_V_STACKING newVStacking)
{
  vStacking = newVStacking;
}

void GxColumn::SetWidthIdentical(bool newStat)
{
  wIdentical = newStat;
}

void GxColumn::SetHeightIdentical(bool newStat)
{
  hIdentical = newStat;
}

void GxColumn::SetAdditionalGap(UINT additionalGap)
{
  aGap = additionalGap*GX_SPACE_INC;
}

void GxColumn::PlaceChildren(void)
{
  if( childList.empty() ) return;
  //calculating the total height needed for the column of children
  UINT nHeight = 0;
  UINT numChildren = 0;

  //the widestWidth and tallestHeight of an object excluding borders
  UINT widestW = 0;
  UINT tallestH = 0;

  //the widest width of an object borders considered
  UINT widestTotal = 0;

  if(hIdentical)
    {
      UINT vBorderSum = 0;
      std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
      std::list<GxWinArea*>::const_iterator cEnd = childList.end();
      while(cPlace != cEnd)
	{
	  numChildren++;

	  vBorderSum += (*cPlace)->TBorder() + (*cPlace)->BBorder();

	  UINT tHeight = (*cPlace)->GetDesiredHeight();
	  UINT tWidth = (*cPlace)->GetDesiredWidth();
	  tallestH = (tHeight > tallestH) ? tHeight : tallestH;
	  widestW = (tWidth > widestW) ? tWidth : widestW;

	  UINT otWidth = tWidth + (*cPlace)->LBorder() +
	    (*cPlace)->RBorder();
	  widestTotal = (otWidth > widestTotal) ? otWidth : widestTotal;

	  cPlace++;
	};

      if(numChildren == 1)
	nHeight = tallestH*numChildren + vBorderSum;
      else
	nHeight = tallestH*numChildren + vBorderSum + (numChildren-1)*aGap;
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
	    
	    UINT otWidth = tWidth + (*cPlace)->LBorder() +
	      (*cPlace)->RBorder();
	    widestTotal = (otWidth > widestTotal) ? otWidth : widestTotal;
	    
	    nHeight += tHeight + (*cPlace)->TBorder() +
	      (*cPlace)->BBorder();

	    cPlace++;
	  };

	if(numChildren != 1)
	  nHeight += (numChildren-1)*aGap;
      };

  UINT availHeight = height;

  UINT availWidth = width;

  int cX, cY;

  if(numChildren == 1)
    {
      GxWinArea *pChild = childList.front();
      if(availHeight < nHeight)
	cY = y;
      else
	cY = y + ((availHeight - nHeight)/2);

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
	
      switch(hStacking)
	{
	case GX_STACK_LEFT:
	  cX = x;
	  break;
	case GX_STACK_RIGHT:
	  cX = x + width - boxWidth;
	  break;
	default: //GX_STACK_H_CEN
	  if(boxWidth > (int)availWidth)
	    cX = x - ((boxWidth - availWidth)/2);
	  else
	    cX = x + ((availWidth - boxWidth)/2);
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
  UINT vGap = aGap;
  if(availHeight > nHeight)
    vGap = (availHeight - nHeight)/(numChildren - 1);

  cY = y;
  std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
  std::list<GxWinArea*>::const_iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      GxWinArea *pCChild = *cPlace;
      int boxWidth;
      if(wIdentical)
	boxWidth = widestW + pCChild->LBorder() + pCChild->RBorder();
      else
	boxWidth = pCChild->GetDesiredWidth() +
	  pCChild->LBorder() + pCChild->RBorder();

      int boxHeight;
      if(hIdentical)
	boxHeight = tallestH + pCChild->TBorder() + pCChild->BBorder();
      else
	boxHeight = pCChild->GetDesiredHeight() +
	  pCChild->TBorder() + pCChild->BBorder();
	
      switch(hStacking)
	{
	case GX_STACK_LEFT:
	  cX = x;
	  break;
	case GX_STACK_RIGHT:
	  cX = x + width - boxWidth;
	  break;
	default: //GX_STACK_H_CEN
	  if(boxWidth > (int)availWidth)
	    cX = x - ((boxWidth - availWidth)/2);
	  else
	    cX = x + ((availWidth - boxWidth)/2);
	};

      //the use of tcY prevents some erent child from modifying our locally
      //critical cY, we don't care what a child does to cX, because it is
      //re-calculated every iteration.
      int tcY = cY;
      int tX = cX + boxWidth;
      int tY = cY + boxHeight;
      pCChild->Place(cX, tX, tcY, tY);

      cY += boxHeight + vGap;
      cPlace++;
    };
}

// ******************************** end GxColumn ***********************

// ************************** start GxThinColumn *************************

GxThinColumn::GxThinColumn(GxRealOwner *pOwner) :
  GxGhost(pOwner)
{}

GxThinColumn::~GxThinColumn(void)
{}

UINT GxThinColumn::GetDesiredWidth(void) const
{
  if( childList.empty() ) return 0; //we are a ghost

  UINT nWidth = 0;

  std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
  std::list<GxWinArea*>::const_iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      UINT tWidth = (*cPlace)->GetDesiredWidth() +
	(*cPlace)->LBorder() + (*cPlace)->RBorder();

      nWidth = (tWidth > nWidth) ? tWidth : nWidth;
      
      cPlace++;
    };

  return nWidth;
}

UINT GxThinColumn::GetDesiredHeight(void) const
{
  if( childList.empty() ) return 0; //we are a ghost

  UINT nHeight = 0;

  std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
  std::list<GxWinArea*>::const_iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      nHeight += (*cPlace)->GetDesiredHeight() +
	(*cPlace)->TBorder() + (*cPlace)->BBorder();
      
      cPlace++;
    };

  return nHeight;
}

// ************************** end GxThinColumn *************************
