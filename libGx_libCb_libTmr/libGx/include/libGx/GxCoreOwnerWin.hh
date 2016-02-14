#ifndef GXCOREOWNERWIN_INCLUDED
#define GXCOREOWNERWIN_INCLUDED

#include <libGx/GxCoreWin.hh>
#include <libGx/GxRealOwner.hh>

//order of inheritance is extremly important:
//Owner part must be distructed before GxCoreWin is Distructed; else
//child xWindows will be invalid
class GxCoreOwnerWin : public GxCoreWin, public GxRealOwner
{
public:
  virtual ~GxCoreOwnerWin(void);

  virtual void Create(void);
  virtual void Place(int &lX, int &rX, int &tY, int &bY);
  virtual void PlaceChildren(void);
  virtual void Display(void);
  //the only reason to overload this would be to PlaceChildren after
  //we are resized;
  virtual void Resize(UINT tWidth, UINT tHeight);

  //by default we return the desired size of the first child in our list
  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  //GxOwner Overloads
  virtual Window GetClosestXWin(void); //virtual because some may want to lie
  //the default is to request the same from my Owner
  virtual GxMapHolder* GetClosestMapHolder(void);
  virtual void UnManageWindow(Window winID);
  virtual void MoveFocusToChild(GxWinArea *pChild, Time eventTime);

  GxDisplayInfo& GetDisplayInfo(void);
  GxVolatileData& GetVolatileData(void);

protected:
  GxCoreOwnerWin(GxOwner *pOwner);
};

#endif //GXCOREOWNERWIN_INCLUDED
