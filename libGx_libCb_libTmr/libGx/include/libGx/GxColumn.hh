#ifndef GXCOLUMN_INCLUDED
#define GXCOLUMN_INCLUDED

#include <libGx/GxGhost.hh>

class GxColumn : public GxGhost
{
public:
  GxColumn(GxRealOwner *pOwner);
  virtual ~GxColumn(void);

  //arranges itself around all of its children arranged in a column
  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  void SetHStacking(GX_H_STACKING newHStacking);
  void SetVStacking(GX_V_STACKING newVStacking);

  void SetWidthIdentical(bool newStat);
  void SetHeightIdentical(bool newStat);
  //specified in multiples of GX_SPACE_INC
  void SetAdditionalGap(UINT additionalGap);

  virtual void PlaceChildren(void);

protected:
  //stored as pixels or additionalGap*GX_SPACE_INC
  UINT aGap;
  GX_H_STACKING hStacking;
  GX_V_STACKING vStacking;
  bool wIdentical, hIdentical;
};

//Just assumes the children will be arranged in a Column, reports this in
//GetDesiredWidth and GetDesiredHeight. This uses default placing mechanisms
class GxThinColumn : public GxGhost
{
public:
  GxThinColumn(GxRealOwner *pOwner);
  virtual ~GxThinColumn(void);

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  //this class relies on the default placing mechanism
  //virtual void PlaceChildren(void);
};

#endif //GXCOLUMN_INCLUDED
