// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _State_h
#define _State_h 1

#include <basics.h> // for Real
#include <DblLinkList.h>
#include <LogOptions.h>
#include <StateSpace.h>
#include <AccEstDispatch.h>

// These cannot be a const because this is a template file.
#define NOT_EVALUATED -1
#define MIN_EVALUATION -1

template <class LocalInfo, class GlobalInfo>
class State {
   LOG_OPTIONS;
   NO_COPY_CTOR(State);

   Bool nodeNotCached;
   NodePtr nodePtr;
   
protected:
   LocalInfo* stateInfo; // State specific info (owned by the State).
   const GlobalInfo& globalInfo; // Shared by all states.
   Real fitness;	 // Result of eval() function.
   Real stdDev;          // standard deviation of fitness.
   int localEvalNum;	 // This should be updated in the derived class.
   MString description;  // ASCII description (used in graph).
   MString graphOptions; // Optional parameters when printing graph.
   Real complexity;      // Complexity metric for this state.
   int evalCost;         // total cost of evaluations so far
   
public:
   State(LocalInfo*& initStateInfo, const GlobalInfo& gI);
   virtual ~State();

   virtual void evaluate_states(GlobalInfo *gInfo,
				StateSpace<State<LocalInfo, GlobalInfo> >*
				graph,
				DblLinkList<State<LocalInfo, GlobalInfo>*>*
				states,
				Bool computeReal = TRUE);
   
   virtual Real eval(GlobalInfo*, Bool computeReal = TRUE,
		     Bool computeEstimated = TRUE) = 0;
   virtual DblLinkList<State*>* gen_succ(GlobalInfo*,
	      StateSpace< State<LocalInfo, GlobalInfo> > *,
					 Bool computeReal = TRUE) = 0;

   // Accessor functions.
   virtual const LocalInfo& get_info() const
      { ASSERT(stateInfo); return *stateInfo; }
   virtual Real get_fitness() const { return fitness; }
   virtual Real get_real_accuracy() const { return -1; }
   virtual Real get_std_dev() const { return stdDev; }
   virtual void set_info(LocalInfo*& newStateInfo);
   virtual void set_fitness(Real newFitness);
   virtual void set_eval_num(int evalNum);
   virtual int get_eval_num() const;
   virtual int get_eval_cost() const { return evalCost; }
   virtual NodePtr get_node(StateSpace< State<LocalInfo, GlobalInfo> >*);
   virtual void set_node(NodePtr n) {
      nodeNotCached = FALSE; nodePtr = n; }

   // Functions which determine output in graph.
   virtual const MString& get_description() const { return description; }
   virtual void set_description(const MString& newDescription);
   virtual const MString& get_graph_options() const { return graphOptions; }
   virtual void set_graph_options(const MString& newGraphOptions);

   // This does anything state specific (i.e. specific to the derived classes)
   // for the final state (e.g. set_graph_options).
   virtual void set_final_state() {}
   
   virtual Bool operator==(const State& compareState) const;

   virtual void display_info(MLCOStream& stream = Mcout) const = 0;
   virtual void display_stats(MLCOStream& stream = Mcout) const = 0;
   virtual void display_for_graph(MLCOStream& stream = Mcout) const = 0;
   virtual void display(MLCOStream& stream = Mcout) const;
};

// Declare operator<< for State.  Cannot use DECLARE_DISPLAY because
// the macro won't parse the two-argument template properly.
template <class LocalInfo, class GlobalInfo>
MLCOStream& operator<<(MLCOStream& s, const State<LocalInfo, GlobalInfo>& c);


// base class for information global to all states
class StateGlobalInfo {
public:
   int numNodesInGraph, numNodesExpanded;

   StateGlobalInfo()
     : numNodesInGraph(0), numNodesExpanded(0) { }
   virtual ~StateGlobalInfo() { }

   virtual int class_id() const = 0;
};

#endif


