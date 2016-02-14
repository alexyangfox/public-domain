#ifndef GXROOTTRANSIENT_INCLUDED
#define GXROOTTRANSIENT_INCLUDED

//a decoration free (unknown to window manager)
//manager window which belongs to the root window

#include <libGx/GxInc.hh>
#include <libGx/GxCoreOwnerWin.hh>

class GxRootTransient : public GxCoreOwnerWin
{
public:
  GxRootTransient(GxRealOwner *pOwner);
  virtual ~GxRootTransient(void);

  //overloaded to do nothing; use below function instead.  MenuPanes are
  //by defination temporary; we don't want to Display them initally
  virtual void Display(void);
  virtual void Display(int xRoot, int yRoot);

protected:
  //returns RootWindow; overload of GxCoreWin function
  Window GetParentWindow(void);

  //this sets critical information: if subclases over-load this should
  //always call this via GxRootTransient::GetWindowData in the overloaded function
  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);
};

#endif //GXROOTTRANSIENT_INCLUDED
