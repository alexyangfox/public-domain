#include <libGx/GxRealOwner.hh>

GxRealOwner::GxRealOwner(void) :
  GxOwner()
{}

GxRealOwner::~GxRealOwner(void)
{}

void GxRealOwner::CreateChildren(void)
{
  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      (*cPlace)->Create();
      cPlace++;
    };

  //for_each(childList.begin(), childList.end(), mem_fun(&GxWinArea::Create));
}

void GxRealOwner::DisplayChildren(void)
{
  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      (*cPlace)->Display();
      cPlace++;
    };

  //for_each(childList.begin(), childList.end(), mem_fun(&GxWinArea::Display));
}

void GxRealOwner::HideChildren(void)
{
  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      (*cPlace)->Hide();
      cPlace++;
    };
}
