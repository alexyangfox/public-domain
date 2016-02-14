// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _SSSearch_h
#define _SSSearch_h 1

#include <basics.h>
#include <State.h>
#include <StateSpace.h>

template <class LocalInfo, class GlobalInfo> 
class StateSpaceSearch {
   LOG_OPTIONS;
   NO_COPY_CTOR(StateSpaceSearch);

protected:
   StateSpace< State<LocalInfo, GlobalInfo> >* graph;
   int numExpansions;
   int evalLimit;
   virtual Bool terminate() = 0;
public:
   enum ShowRealAccuracy { always, bestOnly, finalOnly, never };
   
   StateSpaceSearch() { graph = NULL; showRealAccuracy = bestOnly;}
   virtual ~StateSpaceSearch() { delete graph; }
   virtual void set_user_options(const MString& prefix);
   virtual void set_defaults();
   virtual const State<LocalInfo, GlobalInfo>&
      search(State<LocalInfo, GlobalInfo>* initialState, GlobalInfo *gInfo,
	     const MString& displayFileName = emptyString) = 0;

   // Accessor functions.
   virtual const StateSpace< State<LocalInfo, GlobalInfo> >& get_graph() const
      { return *graph; }
   virtual int get_num_expansions() const { return numExpansions; }

   virtual void set_show_real_accuracy(ShowRealAccuracy sra) {
      showRealAccuracy = sra; }
   virtual ShowRealAccuracy get_show_real_accuracy() const {
      return showRealAccuracy; }

   virtual void set_eval_limit(int newEvalLimit) {
      evalLimit = newEvalLimit; }
   virtual int get_eval_limit(void) {
      return evalLimit; }

protected:
   ShowRealAccuracy showRealAccuracy;
   
};

#endif

