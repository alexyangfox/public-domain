#include <libGx/GxCBList.hh>

using namespace std;

GxCBList::GxCBList(void)
{}

GxCBList::~GxCBList(void)
{
  while( cbList.empty() )
    {
      CbVoidBase *pCB = cbList.front();
      cbList.pop_front();
      delete pCB;
      pCB = 0;
    };
}

void GxCBList::CallCBList(void)
{
  list<CbVoidBase*>::iterator cPlace = cbList.begin();
  while( cPlace != cbList.end() )
    {
      (*cPlace)->DoCallback();
      cPlace++;
    };
}

GX_CB_ID GxCBList::AddCB(const CbVoidBase &rBaseCB)
{
  CbVoidBase *pCB = rBaseCB.Clone();
  cbList.push_back(pCB);
  return pCB;
}

void GxCBList::RemoveCB(GX_CB_ID idToRemove)
{
  //each ID (pointer) can only be in the list once
  list<CbVoidBase*>::iterator cPlace = cbList.begin();
  while( cPlace != cbList.end() )
    {
      if( *cPlace == idToRemove )
	{
	  cbList.erase(cPlace);
	  return;
	};

      cPlace++;
    };
}
