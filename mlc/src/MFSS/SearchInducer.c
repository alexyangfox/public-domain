// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Wrapper inducer for search-based induction methods.
  Assumptions  :
  Comments     : 
  Complexity   : Training is the number of states searched times the
                   estimation time per state.
  Enhancements :
  History      : Dan Sommerfield                                  (5/21/95)
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <SearchInducer.h>
#include <CtrInstList.h>
#include <BFSearch.h>
#include <HCSearch.h>
#include <SASearch.h>

#include <env_inducer.h>
#include <GetOption.h>

RCSID("MLC++, $RCSfile: SearchInducer.c,v $ $Revision: 1.11 $")


// search method information
const MEnum searchMethodEnum =
  MEnum("best-first", SearchInducer::bestFirst) <<
  MEnum("hill-climbing", SearchInducer::hillClimbing) <<
  MEnum("simulated-annealing", SearchInducer::simulatedAnnealing);
const MString searchMethodHelp = "This options specifies the search method "
  "to use to find the best feature subset.";
const SearchInducer::SearchMethod defaultSearchMethod =
    SearchInducer::bestFirst;

// dot file name option
const MString dotFileNameHelp = "This option specifies the file that will "
  "receive output for the graphical representation of the search, which "
  "can be displayed by dotty.";

// evaluation limit option
const MString evalLimitHelp = "This option limits the number of nodes "
  "which may be evaluated during the search to the value of this option "
  "times the number of attributes in the data.  The this option is set "
  "to 0, then there is no limit on evaluations.";
const int defaultEvalLimit = 0;


/***************************************************************************
  Description : Constructor.  We cannot create the global info here because
                  the identity of any derived class in not known within
		  this constructor.  Therefore, derived class constructors
		  MUST create the global info.
  Comments    : 
***************************************************************************/
SearchInducer::SearchInducer(const MString& description, BaseInducer *ind)
   : CtrInducer(description),
     dotFileName(description + ".dot"),
     evalLimit(defaultEvalLimit),
     ssSearch(NULL),
     searchMethod(defaultSearchMethod),
     categorizer(NULL),
     baseInducer(ind),
     globalInfo(NULL),
     finalState(NULL)
{
}


/***************************************************************************
  Description : Destructor.
  Comments    :
***************************************************************************/
SearchInducer::~SearchInducer()
{
   has_global_info();
   delete baseInducer;
   delete globalInfo->trainList;
   delete globalInfo->testList;
   delete globalInfo;
   delete ssSearch;
   delete categorizer;
}

/***************************************************************************
  Description : Verify that the global info was correctly created by the
                  constructor
  Comments    :
***************************************************************************/
Bool SearchInducer::has_global_info(Bool fatalOnFalse) const
{
   if(fatalOnFalse && !globalInfo)
      err << "SearchInducer::has_global_info: global_info should have "
	 "been set by constructor" << fatal_error;
   return (globalInfo != NULL);
}

/***************************************************************************
  Description : Read all options from the user
  Comments    :
***************************************************************************/
void SearchInducer::set_user_options(const MString& prefix)
{
   // make sure the global info exists
   has_global_info();
  
   // if no inducer, use an option
   if(!baseInducer)
      baseInducer = env_inducer(prefix);

   // create a modified prefix without the final underscore (if there is one)
   MString modPrefix;
   if(prefix.length() && prefix[prefix.length()-1] == '_')
      modPrefix = prefix.substring(0, prefix.length()-1);
   else
      modPrefix = prefix;

   // read dot file name here using modified-prefix.dot as the
   // default name of the file.
   dotFileName =
      get_option_string(prefix + "DOT_FILE", modPrefix + ".dot",
			dotFileNameHelp, FALSE);
   
   // read search method here
   searchMethod =
      get_option_enum(prefix + "SEARCH_METHOD", searchMethodEnum,
		      defaultSearchMethod,
		      searchMethodHelp, FALSE);

   evalLimit =
      get_option_int(prefix + "EVAL_LIMIT", defaultEvalLimit,
		     evalLimitHelp, TRUE);

   switch(searchMethod) {
      case bestFirst:
	 ssSearch = new BFSearch<Array<int>, AccEstInfo>();
	 break;
      case hillClimbing:
	 ssSearch = new HCSearch<Array<int>, AccEstInfo>();
	 break;
      case simulatedAnnealing:
	 ssSearch = new SASearch<Array<int>, AccEstInfo>();
	 break;
      default:
	 ASSERT(FALSE);
   }
   
   // set all options in the search and accuracy estimator
   ssSearch->set_user_options(prefix);       
   globalInfo->set_user_options(prefix);

   LOG(1, *this);
}

   
/***************************************************************************
  Description : Display info
  Comments    :
***************************************************************************/
void SearchInducer::display(MLCOStream& stream) const
{
   has_global_info();
   globalInfo->accEst.display_settings(stream);
}


DEF_DISPLAY(SearchInducer);


/***************************************************************************
  Description : Search and return the final state reached
  Comments    : 
***************************************************************************/
const State<Array<int>, AccEstInfo> &
    SearchInducer::search(InstanceBag* trainingSet)
{

   has_global_info();
   if(!ssSearch)
      err << "SearchInducer::search: search method has not been set " <<
	 fatal_error;

   // set the inducer
   globalInfo->inducer = baseInducer;
   if(!globalInfo->inducer)
      err << "SearchInducer::search: must call set_user_options prior to "
	 "search if inducer is not set" << fatal_error;

   // set up global info for search
   delete globalInfo->trainList;
   globalInfo->trainList = &(trainingSet->clone()->cast_to_instance_list());
   
   // lower the inducer's log level.
   globalInfo->inducer->set_log_level(get_log_level() - 5);

   // copy log level to the search
   ssSearch->set_log_level(get_log_level()); 
   globalInfo->accEst.set_log_level(get_log_level() - 4);

   // Initialize the first state
   Array<int> *initialInfo = create_initial_info(trainingSet);
   AccEstState *initialState = create_initial_state(initialInfo, *globalInfo);
   initialState->set_log_level(get_log_level());

   // multiply eval limit by number of attributes in data
   ssSearch->set_eval_limit(evalLimit * globalInfo->trainList->num_attr() );

   // Search
   const State<Array<int>, AccEstInfo>& finalState =
      ssSearch->search(initialState, globalInfo, dotFileName);

   LOG(2, "Search node:" << finalState << endl);
   return finalState;
}


/***************************************************************************
  Description : Find best attributes and create categorizer.
  Comments    : 
***************************************************************************/
void SearchInducer::train()
{
   has_data();
   DBG(OK());

   has_global_info();

   // give a nice error message if attempting to use test-set as an
   // accuracy estimator
   if(globalInfo->accEst.get_accuracy_estimator() ==
      AccEstDispatch::testSet)
      err << "SearchInducer::train: accuracy estimation method may not "
	 "be test-set if using train()" << fatal_error;
   
   // set the inducer
   globalInfo->inducer = baseInducer;
   if(!globalInfo->inducer)
      err << "SearchInducer::train: must call set_user_options prior to "
	 "train if inducer is not set" << fatal_error;
   
   if (globalInfo->inducer->can_cast_to_inducer() == FALSE)
      err << "SearchInducer::train: wrapped inducer must be derived from "
	 "Inducer to use train()" << fatal_error;

   // test data not available so force the SHOW_REAL_ACCURACY option to
   // never for this search.
   StateSpaceSearch<Array<int>, AccEstInfo>::ShowRealAccuracy
      oldOption = ssSearch->get_show_real_accuracy(); 
   ssSearch->set_show_real_accuracy(
      StateSpaceSearch<Array<int>, AccEstInfo>::never);

   // use the final state of the search to build a categorizer
   finalState = (State<Array<int>, AccEstInfo> *)&search(TS);
   categorizer = state_to_categorizer(*finalState);

   // restore the previous value of SHOW_REAL_ACCURACY
   ssSearch->set_show_real_accuracy(oldOption);
}


/*****************************************************************************
  Description  : Return TRUE iff the class has a valid categorizer.
  Comments     :
*****************************************************************************/
Bool SearchInducer::was_trained(Bool fatalOnFalse) const
{
   if( fatalOnFalse && categorizer == NULL )
      err << "SearchInducer::was_trained: No categorizer, "
	 "Call train() to create categorizer" << fatal_error;
   
   return ( categorizer != NULL );
}


/*****************************************************************************
  Description  : Returns the categorizer that the inducer has generated.
  Comments     :
*****************************************************************************/
const Categorizer& SearchInducer::get_categorizer() const
{
   was_trained(TRUE);
   return *categorizer;
}


/***************************************************************************
  Description : Find best attributes and run test.
  Comments    :
***************************************************************************/
Real SearchInducer::train_and_test(InstanceBag* trainingSet,
				const InstanceBag& testList)
{
   // if we're simulating a full Inducer, use its train_and_test instead
   if(can_cast_to_inducer())
      return Inducer::train_and_test(trainingSet, testList);

   ASSERT(trainingSet != NULL && &testList != NULL);
   has_global_info();

   // set up test data
   delete globalInfo->testList;
   globalInfo->testList = &(testList.clone()->cast_to_instance_list());

   Real acc = search(trainingSet).get_real_accuracy();
   return acc;
}



/***************************************************************************
  Description : Train and test from dumped files
  Comments    :
***************************************************************************/
Real SearchInducer::train_and_test_files(const MString& fileStem,
				      const MString& namesExtension,
				      const MString& dataExtension,
				      const MString& testExtension)
{
   CtrInstanceList trainList("", fileStem + namesExtension,
			    fileStem + dataExtension);
   CtrInstanceList testList ("", fileStem + namesExtension,
			      fileStem + testExtension);
   Real acc = train_and_test(&trainList, testList);
   return acc;
}


/***************************************************************************
  Description : Cast to inducer if base can cast
  Comments    :
***************************************************************************/
Bool SearchInducer::can_cast_to_inducer() const
{
   has_global_info();

   globalInfo->inducer = baseInducer;
   ASSERT(globalInfo->inducer);
   Bool canCast = globalInfo->inducer->can_cast_to_inducer();
   if (!canCast)
      LOG(2, "SearchInducer wrapping around base inducer");
   else {
      canCast = ssSearch->get_show_real_accuracy() == 
   	        StateSpaceSearch<BoolArray, AccEstInfo>::never;
      if (!canCast)
	 LOG(2, "SearchInducer show accuracy != never");
   }
   if (canCast)
      LOG(2, "SearchInducer simulating Inducer" << endl);
   else
      LOG(2, ". Simulating BaseInducer" << endl);

   return canCast;
}


/***************************************************************************
  Description : Default state_to_categorizer function aborts.
  Comments    : Not pure virtual so search inducers like C45AP which always
                  wrap around base inducers do not have to define this
		  function.
***************************************************************************/
Categorizer *SearchInducer::state_to_categorizer (
   const State<Array<int>, AccEstInfo>&) const
{
   err << "SearchInducer::state_to_categorizer: should never be called"
      << fatal_error;
   return NULL;
}

/***************************************************************************
  Description : Return final state.  Must exist.
  Comments    : 
***************************************************************************/

const State<Array<int>, AccEstInfo>& SearchInducer::get_final_state()
{
   if (finalState == NULL)
      err << "SearchInducer::get_final_state: finalState is NULL";

   return *finalState;
}
