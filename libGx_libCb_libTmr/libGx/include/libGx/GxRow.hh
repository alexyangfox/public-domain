#ifndef GXROW_INCLUDED
#define GXROW_INCLUDED

#include <libGx/GxGhost.hh>

class GxRow : public GxGhost
{
public:
  GxRow(GxRealOwner *pOwner);
  virtual ~GxRow(void);

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  void SetHStacking(GX_H_STACKING newHStacking);
  void SetVStacking(GX_V_STACKING newVStacking);

  void SetWidthIdentical(bool newStat);
  void SetHeightIdentical(bool newStat);
  //the gap is in GX_SPACE_INCs
  void SetAdditionalGap(UINT additionalGap);

  virtual void PlaceChildren(void);

protected:
  GX_H_STACKING hStacking;
  GX_V_STACKING vStacking;
  bool wIdentical, hIdentical;
  //stored as number pixels or GX_SPACE_INC*additionalGap
  //prevents lots of multiplies inside code
  UINT aGap;
};

//just assumes the children will be arranged in a row, reports this in
//GetDesiredWidth and GetDesiredHeight. This uses default placing mechanisms
class GxThinRow : public GxGhost
{
public:
  GxThinRow(GxRealOwner *pOwner);
  virtual ~GxThinRow(void);

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  //this class relies on the default placing mechanism
  //virtual void PlaceChildren(void);
};

#endif //GX_ROW_INCLUDED
