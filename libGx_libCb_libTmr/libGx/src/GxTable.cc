#include <libGx/GxTable.hh>
#include "GxDefines.hh"

GxTable::GxTable(GxRealOwner *pOwner) :
  GxGhost(pOwner), numObjPerRow(2), columnToExpand(0), expandColumn(false),
  rowToExpand(0), expandRow(false), hGap(0), vGap(0)
{}

GxTable::~GxTable(void)
{}

void GxTable::SetNumObjectsPerRow(UINT numObj)
{
  numObjPerRow = numObj;
}

void GxTable::SetColumnToExpand(UINT cToExpand)
{
  columnToExpand = cToExpand;
}

void GxTable::ExpandColumn(bool state)
{
  expandColumn = state;
}

void GxTable::SetRowToExpand(UINT rToExpand)
{
  rowToExpand = rToExpand;
}

void GxTable::ExpandRow(bool state)
{
  expandRow = state;
}

void GxTable::SetAdditionalHGap(UINT additionalGap)
{
  hGap = additionalGap;
}

void GxTable::SetAdditionalVGap(UINT additionalGap)
{
  vGap = additionalGap;
}

UINT GxTable::GetDesiredWidth(void) const
{
  if( childList.empty() ) return 0; //we are a ghost.

  UINT numObjects = childList.size();

  UINT numRows = numObjects/numObjPerRow;
  if(numObjects%numObjPerRow)
    numRows++;

  UINT columnWidths[numObjPerRow];
  UINT rowHeights[numRows];
  BuildColumnWidthsAndRowHeights(columnWidths, rowHeights, numRows);

  UINT totalWidth = 0;
  for(UINT ii = 0; ii < numObjPerRow; ii++)
    totalWidth += columnWidths[ii];

  UINT numRealColumns = numObjPerRow;
  if(numObjects < numObjPerRow)
    numRealColumns = numObjects;

  return totalWidth + (numRealColumns-1)*hGap*GX_SPACE_INC;
}

UINT GxTable::GetDesiredHeight(void) const
{
  if( childList.empty() ) return 0; //we are a ghost.

  UINT numObjects = childList.size();

  UINT numRows = numObjects/numObjPerRow;
  if(numObjects%numObjPerRow)
    numRows++;

  UINT columnWidths[numObjPerRow];
  UINT rowHeights[numRows];
  BuildColumnWidthsAndRowHeights(columnWidths, rowHeights, numRows);

  UINT totalHeight = 0;
  for(UINT ii = 0; ii < numRows; ii++)
    totalHeight += rowHeights[ii];

  return totalHeight + (numRows - 1)*vGap*GX_SPACE_INC;
}

// ******** don't forget we are a ghost ******************
void GxTable::PlaceChildren(void)
{
  if( childList.empty() ) return;

  UINT numObjects = childList.size();

  UINT numRows = numObjects/numObjPerRow;
  if(numObjects%numObjPerRow)
    numRows++;

  UINT columnWidths[numObjPerRow];
  UINT rowHeights[numRows];
  BuildColumnWidthsAndRowHeights(columnWidths, rowHeights, numRows);

  UINT desiredHeight = 0;
  for(UINT ii = 0; ii < numRows; ii++)
    desiredHeight += rowHeights[ii];

  UINT desiredWidth = 0;
  for(UINT ii = 0; ii < numObjPerRow; ii++)
    desiredWidth += columnWidths[ii];

  /*
    the buffer added to the row/column that is expanded is the difference
    between my width/height and my desired width height. If we are too large
    we don't expand at all
  */

  UINT hExpansion = 0;
  if(desiredWidth < width)
    hExpansion = width - desiredWidth;

  UINT vExpansion = 0;
  if(desiredHeight < height)
    vExpansion = height - desiredHeight;

  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
  UINT cRow = 0;
  int cY = y;
  do
    {
      int cX = x;

      UINT rowHeight = rowHeights[cRow];
      if(expandRow && (cRow == rowToExpand))
	rowHeight += vExpansion;
      for(UINT ii = 0; ii < numObjPerRow; ii++)
	{
	  UINT columnWidth = columnWidths[ii];
	  if(expandColumn && (ii == columnToExpand))
	    columnWidth += hExpansion;

	  int rX = cX + columnWidth;
	  int bY = cY + rowHeight;
	  (*cPlace)->Place(cX, rX, cY, bY);

	  cX += columnWidth + hGap*GX_SPACE_INC;

	  cPlace++;
	  if(cPlace == cEnd) break;
	};
      cY += rowHeight + vGap*GX_SPACE_INC;
      cRow++;
    }while(cPlace != cEnd);
}

void GxTable::BuildColumnWidthsAndRowHeights(UINT *pCWM, //pColumnWidthsMatrix
					     UINT *pRHM, //pRowHeightsMatrix
					     UINT numRows) const
{
  if(!pCWM || !pRHM) return; //useless little check.
  if( childList.empty() ) return;

  UINT numObjects = childList.size();

  for(UINT ii = 0; ii < numObjPerRow; ii++)
    pCWM[ii] = 1;

  for(UINT ii = 0; ii < numRows; ii++)
    pRHM[ii] = 1;

  std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
  std::list<GxWinArea*>::const_iterator cEnd = childList.end();
  UINT cRow = 0;
  do
    {
      UINT tallestHeight = 1; //tallest on this row
      for(UINT ii = 0; ii < numObjPerRow; ii++)
	{
	  UINT objWidth = (*cPlace)->GetDesiredWidth() +
	    (*cPlace)->LBorder() + (*cPlace)->RBorder();
	  if(objWidth > pCWM[ii])
	    pCWM[ii] = objWidth;

	  UINT objHeight = (*cPlace)->GetDesiredHeight() +
	    (*cPlace)->TBorder() + (*cPlace)->BBorder();
	  if(objHeight > tallestHeight)
	    tallestHeight = objHeight;

	  cPlace++;
	  if(cPlace == cEnd) break;
	};
      //it is easier to mis-calculate numRows, so we check on that here.
      if(cRow == numRows)
	return;
      pRHM[cRow] = tallestHeight;
      cRow++;
    }while(cPlace != cEnd);

  return;
}
