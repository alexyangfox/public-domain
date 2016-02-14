#ifndef GXTABLE_INCLUDED
#define GXTABLE_INCLUDED

#include <libGx/GxGhost.hh>

//organizes its children into rows and columns. different than a column of rows
//and a row of coulumns because the rows and columns are rigidly defined

class GxTable : public GxGhost
{
public:
  GxTable(GxRealOwner *pOwner);
  virtual ~GxTable(void);

  void SetNumObjectsPerRow(UINT numObj);

  void SetColumnToExpand(UINT cToExpand); //valid columns start from 0
  void ExpandColumn(bool state);

  void SetRowToExpand(UINT rToExpand); //valid rows start from 0;
  void ExpandRow(bool state);

  //specified in multiples of GX_SPACE_INC
  void SetAdditionalHGap(UINT additionalGap);
  void SetAdditionalVGap(UINT additionalGap);

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  virtual void PlaceChildren(void);

protected:
  //fills in the matrix. pColumnWidthsMatrix *must* have numObjPerRow elements
  //we really should not bother passing in numRows
  void BuildColumnWidthsAndRowHeights(UINT *pColumnWidthsMatrix,
				      UINT *pRowHeightsMatrix, UINT numRows) const;

  UINT numObjPerRow;

  UINT columnToExpand;
  bool expandColumn;

  UINT rowToExpand;
  bool expandRow;

  UINT hGap;
  UINT vGap;
};

#endif //GXTABLE_INCLUDED
