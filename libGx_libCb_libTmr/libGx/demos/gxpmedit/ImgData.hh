#ifndef IMGDATA_INCLUDED
#define IMGDATA_INCLUDED

#include <vector>

#include "PixValue.hh"

class PixCoord
{
public:
  PixCoord(void) {};
  PixCoord(int nX, int nY) : x(nX), y(nY) {};
  ~PixCoord(void) {};

  int x, y;
};

class ImgData
{
public:
  ImgData(void);
  ~ImgData(void);

  bool Modified(void) const;
  void ClearModified(void);
 
  void Clear(const PixValue &rValue);
  void ClearSelected(void);
  void ModifySelected(const PixValue &rValue, PIX_FIELD modifiers);

  unsigned Width(void) const;
  unsigned Height(void) const;

  //clears only newly created pixels to rFillValue
  void SafeResize(unsigned newWidth, unsigned newHeight, const PixValue &rFillValue);
  //clears everything to rClearValue
  void ResizeAndClear(unsigned newWidth, unsigned newHeight, const PixValue &rClearValue);

  void SetValue(unsigned x, unsigned y, const PixValue &rValue, PIX_FIELD modifiers);
  void FillArea(unsigned x1, unsigned y1, unsigned width, unsigned height, const PixValue &rValue, PIX_FIELD modifiers);
  void DrawLine(unsigned x1, unsigned y1, unsigned x2, unsigned y2, const PixValue &rValue, PIX_FIELD modifiers);

  void MoveBlock(unsigned blockWidth, unsigned blockHeight, const PixCoord &from, const PixCoord &to);

  const PixValue & GetValue(unsigned tx, unsigned ty) const;

protected:
  PixValue& GetValue(unsigned tx, unsigned ty);
  std::vector<PixValue> data;

  bool modified;

  unsigned width;
  unsigned height;
};

#endif //IMGDATA_INCLUDED
