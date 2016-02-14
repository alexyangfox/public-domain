#ifndef CBCALLBACK_INCLUDED
#define CBCALLBACK_INCLUDED

//function object plugin
class CbVoidBase
{
public:
  CbVoidBase(void);
  virtual ~CbVoidBase(void);

  virtual void DoCallback(void);

  virtual CbVoidBase* Clone(void) const;
};

template <class T>
class CbVoidMember : public CbVoidBase
{
public:
  CbVoidMember(T* pObject, void (T::*pFunction)(void) ) :
    CbVoidBase(), pObj(pObject), pFun(pFunction) {};
  virtual ~CbVoidMember(void){};

  virtual void DoCallback(void)
  {
    (pObj->*pFun)();
  }

  virtual CbVoidBase* Clone(void) const
  {
    return (CbVoidBase*) new CbVoidMember<T>(pObj, pFun);
  }

private:
  T* pObj;
  void (T::*pFun)(void);
};

template <class T, class OBJ>
class CbVoidMemberObj : public CbVoidBase
{
public:
  CbVoidMemberObj(T* pObject, void (T::*pFunction)(OBJ), OBJ tcbObj) :
    CbVoidBase(), pObj(pObject), pFun(pFunction), cbObj(tcbObj) {};
  virtual ~CbVoidMemberObj(void){};

  virtual void DoCallback(void)
  {
    (pObj->*pFun)(cbObj);
  }

  virtual CbVoidBase* Clone(void) const
  {
    return (CbVoidBase*) new CbVoidMemberObj<T,OBJ>(pObj, pFun, cbObj);
  }

private:
  T* pObj;
  void (T::*pFun)(OBJ);
  OBJ cbObj;
};

template <class OBJ>
class CbVoidPlainObj : public CbVoidBase
{
public:
  CbVoidPlainObj(void (*pFunction)(OBJ), OBJ tcbObj) :
    CbVoidBase(), pFun(pFunction), cbObj(tcbObj) {};
  virtual ~CbVoidPlainObj(void){};

  virtual void DoCallback(void)
  {
    (*pFun)(cbObj);
  };

  virtual CbVoidBase* Clone(void) const
  {
    return (CbVoidBase*) new CbVoidPlainObj<OBJ>(pFun, cbObj);
  };

private:
  void (*pFun)(OBJ);
  OBJ cbObj;
};

class CbVoidPlain : public CbVoidBase
{
public:
  CbVoidPlain(void (*pFunction)(void));
  virtual ~CbVoidPlain(void);

  virtual void DoCallback(void);

  virtual CbVoidBase* Clone(void) const;

private:
  void (*pFun)(void);
};

template <class ARG>
class CbOneBase
{
public:
  CbOneBase(void){};
  virtual ~CbOneBase(void){};
  
  virtual void DoCallback(ARG){};

  virtual CbOneBase<ARG>* Clone(void) const
  {
    return (CbOneBase<ARG>*) new CbOneBase<ARG>;
  };
};
  
template <class ARG, class T>
class CbOneMember : public CbOneBase<ARG>
{
public:
  CbOneMember(T*pObject, void (T::*pFunction)(ARG) ) :
    CbOneBase<ARG>(), pObj(pObject), pFun(pFunction) {};
  virtual ~CbOneMember(void){};

  virtual void DoCallback(ARG arg)
  {
    (pObj->*pFun)(arg);
  };
  
  virtual CbOneBase<ARG>* Clone(void) const
  {
    return (CbOneBase<ARG>*) new CbOneMember<ARG,T>(pObj, pFun);
  };

private:
  T* pObj;
  void (T::*pFun)(ARG);
};

template <class ARG, class T, class OBJ>
class CbOneMemberObj : public CbOneBase<ARG>
{
public:
  CbOneMemberObj(T*pObject, void (T::*pFunction)(ARG, OBJ), OBJ tcbObj ) :
    CbOneBase<ARG>(), pObj(pObject), pFun(pFunction), cbObj(tcbObj) {};
  virtual ~CbOneMemberObj(void){};

  virtual void DoCallback(ARG arg)
  {
    (pObj->*pFun)(arg, cbObj);
  };
  
  virtual CbOneBase<ARG>* Clone(void) const
  {
    return (CbOneBase<ARG>*) new CbOneMemberObj<ARG,T,OBJ>(pObj, pFun, cbObj);
  };

private:
  T* pObj;
  void (T::*pFun)(ARG, OBJ);
  OBJ cbObj;
};

template <class ARG>
class CbOnePlain : public CbOneBase<ARG>
{
public:
  CbOnePlain( void (*pFunction)(ARG arg) ) :
    CbOneBase<ARG>(), pFun(pFunction) {};
  virtual ~CbOnePlain(void){};
  
  virtual void DoCallback(ARG arg)
    {
      (*pFun)(arg);
    };
     
  virtual CbOneBase<ARG>* Clone(void) const
    {
      return (CbOneBase<ARG>*) new CbOnePlain<ARG>(pFun);
    };

private:
  void (*pFun)(ARG);
};

class CbVoidFO //funcion object callback object
{
public:
  CbVoidFO(void);
  CbVoidFO(const CbVoidBase &rCB);
  CbVoidFO(const CbVoidFO &rhs);
  ~CbVoidFO(void);

  void operator()(void) const;
  void Reset(void); //add to others.
  void Assign(const CbVoidBase &rCB);
  CbVoidBase* CloneCurrentCallback(void) const; //add to others

private:
  CbVoidBase *pCB;
};

template <class ARG>
class CbOneFO //callback object
{
public:
  CbOneFO(void)
  {
    pCB = new CbOneBase<ARG>;
  };
  CbOneFO(const CbOneBase<ARG> &rCB)
  {
    pCB = rCB.Clone();
  };
  CbOneFO(const CbOneFO<ARG> &rhs)
  {
    pCB = rhs.pCB->Clone();
  };
  ~CbOneFO(void)
  {
    delete pCB;
    pCB = 0;
  };

  void operator()(ARG arg1) const
  {
    pCB->DoCallback(arg1);
  };

  void Assign(const CbOneBase<ARG> &rCB)
  {
    delete pCB;
    pCB = rCB.Clone();
  };

private:
  CbOneBase<ARG>* pCB;
};

template <class ARG1, class ARG2>
class CbTwoBase
{
public:
  CbTwoBase(void){};
  virtual ~CbTwoBase(void){};

  virtual CbTwoBase* Clone(void) const
  {
    return new CbTwoBase;
  };

  virtual void DoCallback(ARG1, ARG2){};
};

template <class ARG1, class ARG2>
class CbTwoPlain : public CbTwoBase<ARG1, ARG2>
{
public:
  CbTwoPlain( void(*ptFun)(ARG1 arg1, ARG2 arg2) ) :
    CbTwoBase<ARG1,ARG2>(), pFun(ptFun)
  {};
  virtual ~CbTwoPlain(void){};

  virtual CbTwoBase<ARG1,ARG2>* Clone(void) const
  {
    return (CbTwoBase<ARG1,ARG2>*) new CbTwoPlain<ARG1,ARG2>(pFun);
  };

  virtual void DoCallback(ARG1 arg1, ARG2 arg2)
  {
    *pFun(arg1,arg2);
  };
private:
  void (*pFun)(void);
};

template <class ARG1, class ARG2, class T>
class CbTwoMember : public CbTwoBase<ARG1, ARG2>
{
public:
  CbTwoMember(T* pObject, void (T::*ptFun)(ARG1, ARG2) ) :
    CbTwoBase<ARG1,ARG2>(), pObj(pObject), pFun(ptFun)
  {};
  virtual ~CbTwoMember(void){};

  virtual CbTwoBase<ARG1,ARG2>* Clone(void) const
  {
   return (CbTwoBase<ARG1,ARG2>*) new CbTwoMember<ARG1,ARG2,T>(pObj, pFun);
  };

  virtual void DoCallback(ARG1 arg1, ARG2 arg2)
  {
    (pObj->*pFun)(arg1, arg2);
  };

private:
  T *pObj;
  void(T::*pFun)(ARG1, ARG2);
};

template <class ARG1, class ARG2>
class CbTwoFO //a callback
{
public:
  CbTwoFO(void)
  {
    pCB = new CbTwoBase<ARG1, ARG2>;
  };
  CbTwoFO(const CbTwoBase<ARG1,ARG2> &rCB)
  {
    pCB = rCB.Clone();
  };
  ~CbTwoFO(void)
  {
    delete pCB;
    pCB = 0;
  };

  void operator()(ARG1 arg1, ARG2 arg2) const
    {
      pCB->DoCallback(arg1, arg2);
    };
  void Assign(const CbTwoBase<ARG1,ARG2> &newCB)
  {
    delete pCB;
    pCB = newCB.Clone();
  };

private:
  CbTwoBase<ARG1,ARG2> *pCB;
};

#endif //CBCALLBACK_INCLUDED
