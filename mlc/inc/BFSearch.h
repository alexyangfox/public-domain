// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _BFSearch_h
#define _BFSearch_h 1

#include <SSSearch.h>

template <class LocalInfo, class GlobalInfo>
class BFSearch : public StateSpaceSearch<LocalInfo, GlobalInfo> {
   int maxNonImprovingExpansions;
   Real minExpansionImprovement;
   
   int numNonImprovingExpansions;
   NodePtr bestNode;
   DblLinkList<NodePtr> openList, closedList;
protected:
   virtual Bool terminate();
public:
   BFSearch() { set_defaults(); }
   virtual ~BFSearch() {}

   virtual void set_user_options(const MString& stem);
   virtual void set_defaults();

   virtual void set_max_non_improving_expansions(int maxNIE)
      { maxNonImprovingExpansions = maxNIE; }
   virtual void set_min_expansion_improvement(Real minEI)
      { minExpansionImprovement = minEI; }
   virtual const State<LocalInfo, GlobalInfo>&
      search(State<LocalInfo, GlobalInfo>* initialState, GlobalInfo *gInfo,
	     const MString& displayFileName = emptyString);
};

#endif


