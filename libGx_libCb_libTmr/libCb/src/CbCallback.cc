#include <libCb/CbCallback.hh>

CbVoidBase::CbVoidBase(void)
{}

CbVoidBase::~CbVoidBase(void)
{}

void CbVoidBase::DoCallback(void)
{}

CbVoidBase* CbVoidBase::Clone(void) const
{
  return (CbVoidBase*) new CbVoidBase;
}

CbVoidPlain::CbVoidPlain(void (*pFunction)(void))
{
  pFun = pFunction;
}

CbVoidPlain::~CbVoidPlain(void)
{
  pFun = 0;
}

void CbVoidPlain::DoCallback(void)
{
  (*pFun)();
}

CbVoidBase* CbVoidPlain::Clone(void) const
{
  return (CbVoidBase*) new CbVoidPlain(pFun);
}

// ******************* start CbVoid **********************

CbVoidFO::CbVoidFO(void)
{
  pCB = new CbVoidBase();
}

CbVoidFO::CbVoidFO(const CbVoidBase &rCB)
{
  pCB = rCB.Clone();
}

CbVoidFO::CbVoidFO(const CbVoidFO &rhs)
{
  pCB = (rhs.pCB)->Clone();
}

CbVoidFO::~CbVoidFO(void)
{
  delete pCB;
  pCB = 0;
}

void CbVoidFO::operator()(void) const
{
  pCB->DoCallback();
}

void CbVoidFO::Reset(void)
{
  delete pCB;
  pCB = new CbVoidBase();
}

void CbVoidFO::Assign(const CbVoidBase &rCB)
{
  delete pCB;
  pCB = rCB.Clone();
}

CbVoidBase* CbVoidFO::CloneCurrentCallback(void) const
{
  return pCB->Clone();
}
