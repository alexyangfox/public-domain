// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _HCSearch_h
#define _HCSearch_h 1

#include <SSSearch.h>

template <class LocalInfo, class GlobalInfo>
class HCSearch : public StateSpaceSearch<LocalInfo, GlobalInfo> {
   Real bestEval, prevBestEval;
   NodePtr bestNode, prevBestNode;
protected:
   virtual Bool terminate();
public:
   HCSearch() {}
   virtual ~HCSearch() {}
   virtual const State<LocalInfo, GlobalInfo>&
      search(State<LocalInfo, GlobalInfo>* initialState, GlobalInfo *,
	     const MString& displayFileName = emptyString);
};

#endif
