#ifndef GXGHOST_INCLUDED
#define GXGHOST_INCLUDED

#include "GxRealOwner.hh"
#include "GxWinArea.hh"

class GxGhost : public GxRealOwner, public GxWinArea
{
public:
  GxGhost(GxRealOwner *pOwner);
  virtual ~GxGhost(void);

  virtual void Place(int &lX, int &rX, int &tY, int &bY);
  virtual void PlaceChildren(void);
  virtual void Create(void);
  virtual void Display(void);
  virtual void Hide(void);

  //return true if this point (in my parent's Coords) lies inside of my border
  bool CoordInside(int xCoord, int yCoord);
  //these adjust my coordinates to "real" xWin coords
  int AdjustX(int xCoord);
  int AdjustY(int yCoord);

  //returns the desired size of our first child
  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  Window GetClosestXWin(void);
  virtual GxMapHolder* GetClosestMapHolder(void);
  virtual void UnManageWindow(Window winID);

  virtual void MoveFocusToChild(GxWinArea *pChild, Time eventTime);

  GxDisplayInfo& GetDisplayInfo(void);
  GxVolatileData& GetVolatileData(void);
};

#endif //GXGHOST_INCLUDED
