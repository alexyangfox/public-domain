#ifdef LIBGX_DEBUG_BUILD
#include <assert.h>
#endif //LIBGX_DEBUG_BUILD

#include <libGx/GxOwner.hh>

GxOwner::~GxOwner(void)
{
  //notify all of my children that I have been deleted.
  //hackish. there is probably a more stl_ish way of using the list.
  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
#ifdef LIBGX_DEBUG_BUILD
      assert( (*cPlace) );
#endif //LIBGX_DEBUG_BUILD
      (*cPlace)->OwnerDeleted();
      cPlace++;
    };

  //hack: old DestroyChildren note, not sure how signifigant this is anymore
  //because we are going to delete all of my children; and we don't want
  //them to slowly remove themselves from my list via RemoveChild; it faster
  //to set pFirstNode to NULL here and then delete them one by one
}

void GxOwner::UnManageWindow(Window)
{
  //  cerr << "Would of been a fatal error if virtual void" << endl;
}

void GxOwner::AddChild(GxWinArea *pNewChild)
{
#ifdef LIBGX_DEBUG_BUILD
  assert(pNewChild);
#endif //LIBGX_DEBUG_BUILD
  //std::cout << "GxOwner::AddChild size before: " << childList.size() << std::endl;
  childList.push_back(pNewChild);
  //std::cout << "GxOwner::AddChild size after: " << childList.size() << std::endl;
}

void GxOwner::RemoveChild(GxWinArea *pChild)
{
  // DestroyChildren() prematurly emptys the list so that
  //as each child tries to remove itself from the list, we
  //do not cause a list traversel
#ifdef LIBGX_DEBUG_BUILD
  assert(pChild);
#endif //LIBGX_DEBUG_BUILD
  
  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      if(*cPlace == pChild)
	{
	  cPlace = childList.erase(cPlace);
	  return;
	};
      cPlace++;
    };

}

GxOwner::GxOwner(void)
{}
