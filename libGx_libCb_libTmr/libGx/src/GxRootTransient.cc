#include <libGx/GxRootTransient.hh>

GxRootTransient::GxRootTransient(GxRealOwner *pOwner) :
  GxCoreOwnerWin(pOwner)
{}

GxRootTransient::~GxRootTransient(void)
{}

void GxRootTransient::Display(void)
{}

void GxRootTransient::Display(int xRoot, int yRoot)
{
  Move(xRoot, yRoot);
  XMapRaised(dInfo.display, xWin);
  DisplayChildren();
}


Window GxRootTransient::GetParentWindow(void)
{
  return RootWindow(dInfo.display, dInfo.screenNum);
}

void GxRootTransient::GetWindowData(XSetWindowAttributes &winAttributes,
				    ULINT &valueMask)
{
  winAttributes.cursor = dInfo.defaultCursor;
  winAttributes.save_under = true;
  winAttributes.override_redirect = true;
  valueMask |= CWOverrideRedirect | CWSaveUnder | CWCursor;
}
