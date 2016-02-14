// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : StateSpaceSearch is the base class for search methods.
                   There are three class hierarchies in the search engine
		   classes.  The first is the Search heirarchy, of which
		   this class is the base.  The derived classes BFSearch,
		   HCSearch, and SASearch implement specific search methods
		   based on this framework.  The second in the State
		   hierarchy, which defines properties of a single state
		   in the search space.  AccEstState, CompState, and other
		   state classes for specific operations are derived from
		   State.  A related class StateSpace stores a graph of
		   States which is used by the Search heirarchy.  The last
		   is the SearchInducer heirarchy.  SearchInducer abstracts
		   commonalities in a Wrapper Inducer based on performing
		   a search using another Inducer.
  Assumptions  :
  Comments     :
  Complexity   : Depends on space being searched.
  Enhancements :
  History      : Dan Sommerfield                                   12/12/94
                   Initial revision (.c)
***************************************************************************/
#include <basics.h>
#include <BFSearch.h>
#include <DblLinkList.h>
#include <GetOption.h>
#include <SSSearch.h>

// RCSID("MLC++, $RCSfile: SSSearch.c,v $ $Revision: 1.7 $")


// Option information.  Must be #define's instead of const declarations
// becuase this file is templated.
#define DEFAULT_SHOW_REAL_ACC bestOnly
#define DEFAULT_MAX_EVALS 0
#define SHOW_REAL_ACC_HELP "This option specifies when to compute and " \
  "show the real accuracy for states.  Always computes a real accuracy " \
  "for every state in the graph.  BestOnly computes the real accuracy " \
  "whenever a node is proclaimed the current best node.  FinalOnly " \
  "computes real accuracy only for the final node returned by the " \
  "search, and never never computes real accuracy.  The never option " \
  "must be used if there is no way to determine real accuracy."
#define MAX_EVALS_HELP "This option specifies an upper bound on the " \
  "number of evaluations a search is allowed to perform.  If the number " \
  "of nodes expanded during the search exceeds this value times the " \
  "number of attributes in the data, then the search will terminate " \
  "early.  Choosing a value of 0 specifies no limit on evaluations."

enum {always, bestOnly, finalOnly, never};
const MEnum showRealAccEnum =
   MEnum("always", always) <<
   MEnum("best-only", bestOnly) <<
   MEnum("final-only", finalOnly) <<
   MEnum("never", never);


/***************************************************************************
  Description : Sets user options common to all search methods
  Comments    : 
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
void StateSpaceSearch<LocalInfo, GlobalInfo>::set_user_options(
   const MString& prefix)
{
   set_show_real_accuracy(
      get_option_enum(prefix + "SHOW_REAL_ACC", showRealAccEnum,
		      get_show_real_accuracy(), SHOW_REAL_ACC_HELP, TRUE));
   // don't prompt for eval limit because it is specific to whatever
   // is using the StateSpaceSearch (e.g. FSS)
}

/***************************************************************************
  Description : Resets all options to their default values
  Comments    : 
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
void StateSpaceSearch<LocalInfo, GlobalInfo>::set_defaults()
{
   set_show_real_accuracy(DEFAULT_SHOW_REAL_ACC);
   set_eval_limit(DEFAULT_MAX_EVALS);
}


