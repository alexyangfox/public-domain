// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _SASearch_h
#define _SASearch_h 1

#include <SSSearch.h>
#include <SANode.h>
#include <DynamicArray.h>
#include <MRandom.h>

template <class LocalInfo, class GlobalInfo>
class SASearch : public StateSpaceSearch<LocalInfo, GlobalInfo> {
   RAND_OPTIONS;
   int numNonImprovingExpansions;
   int maxNumberOfNonImprovingExpansions;
   Real minIncreaseForImprovingExpansion;
   int maxEvaluations;
   int minExpEvaluations; // minimum evals before expanding a node
   int numReEvaluations;  // total number of reevaluations
   Real initialLambda;
   DynamicArray<SANode> nodeList;

   virtual void display_nodelist(MLCOStream& stream = Mcout) const;
   virtual void reeval_node(int nodeNum, GlobalInfo *gInfo,
			    NodePtr& bestRetired,
			    StateSpace< State<LocalInfo, GlobalInfo> >& graph);
protected:
   virtual Bool terminate();
public:
   SASearch();
   virtual ~SASearch() {}
   virtual void set_defaults();
   virtual void set_user_options(const MString& stem);
   
   virtual void set_max_non_improving_expansions(int maxExpansions);
   virtual int  get_max_non_improving_expansions() const;
   virtual void set_min_expansion_improvement(Real minImprovement);
   virtual Real get_min_expansion_improvement() const;
   virtual void set_max_evaluations(int maxEval);
   virtual int  get_max_evaluations() const;
   virtual void set_min_exp_evaluations(int maxEval);
   virtual int  get_min_exp_evaluations() const;
   virtual void set_initial_lambda(Real initLambda);
   virtual Real get_initial_lambda() const;

   virtual const State<LocalInfo, GlobalInfo>&
      search(State<LocalInfo, GlobalInfo>* initialState, GlobalInfo *gInfo,
	     const MString& displayFileName = emptyString);
};

#endif
