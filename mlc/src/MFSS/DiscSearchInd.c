// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Wrapper inducer for optimal discretization.
                 The feature selection is a best-first search from the initial
 		   state of no attributes (features).
  Assumptions  :
  Comments     : 
  Complexity   : Training is the number of states searched times the
                   estimation time per state.
  Enhancements :
  History      : Dan Sommerfield                                    6/12/95
                   Refit to new search framework
                 James Dougherty                                   01/26/95
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <DiscSearchInd.h>
#include <CtrInstList.h>
#include <BFSearch.h>
#include <HCSearch.h>
#include <SASearch.h>
#include <env_inducer.h>
#include <GetOption.h>
#include <DFInducer.h>
#include <DiscDispatch.h>


RCSID("MLC++, $RCSfile: DiscSearchInd.c,v $ $Revision: 1.8 $")


// direction option
const MEnum directionEnum =
  MEnum("forward", DiscSearchInducer::forward) <<
  MEnum("middle", DiscSearchInducer::middle) <<
  MEnum("backward", DiscSearchInducer::backward);
const MString directionHelp = "This option chooses the direction in which to "
  "search.  Forward causes the search to begin with an empty subset of "
  "features, while backward causes the search to begin with a full subset."
  " Middle starts from the discretization's heuristic values.";
const DiscSearchInducer::Direction defaultDirection =
  DiscSearchInducer::middle;

// use compound option
const MString useCompoundHelp = "This option specifies whether or not to "
  "combine information about generated states in the search in an attempt "
  "to generate a better state more quickly.";

// complexity penalty option
const MString complexityPenaltyHelp = "This option specifies a multiplier "
  "which determines how much complexity of a state hurts its fitness.";

//initial discretization value option
const MString discVectorHelp = "This option specifies the initial value"
  " for the discretization vector.";

//help for Discretization type
const MString discTypeHelp = "This option specifies which type of "
 "discretization method to use.";

const MString restrictHelp = "Allow only plus one or minus one from initial"
 " discretization vector";

//help for whether or not to do FSS
const MString doFSSHelp = "If true, this option causes the search to include "
 "feature subset selection on nominal attributes.  Otherwise, search states "
 "will always include all nominals.";


/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
DiscSearchInducer::DiscSearchInducer(const MString& description,
				     BaseInducer *ind)
   : SearchInducer(description, ind),
     direction(middle),
     middleRestricted(FALSE)
{     
   // establish global info
   globalInfo = create_global_info();
}


/***************************************************************************
  Description : Destructor.
  Comments    :
***************************************************************************/
DiscSearchInducer::~DiscSearchInducer()
{
}


/***************************************************************************
  Description : Gather options from the user
  Comments    :
***************************************************************************/
void DiscSearchInducer::set_user_options(const MString& prefix)
{
   // check global info
   has_global_info();
   ASSERT(globalInfo->class_id() == DISC_INFO);
   DiscInfo *discInfo = (DiscInfo *)globalInfo;

   SearchInducer::set_user_options(prefix);

   //Allocate Filter Inducer now to make sure we set up a DiscFilterInducer.
   // take the inducer provided to us through options and stuff it into the
   // new DiscFilterInducer.
   ASSERT(baseInducer);
   DiscFilterInducer *dfInducer = new DiscFilterInducer("Disc search filter");
   dfInducer->set_inducer(baseInducer);
   baseInducer = dfInducer;
   dfInducer->set_user_options_no_inducer("");
   
   // pick a starting point (direction)
   direction =
      get_option_enum(prefix + "DIRECTION", directionEnum,
		      defaultDirection,
		      directionHelp, TRUE);

   if (direction == middle)
      middleRestricted = get_option_bool(prefix + "RESTRICT_MIDDLE",
					      FALSE, restrictHelp, TRUE);


   //whether to do FSS or not
   Bool decision = get_option_bool(prefix + "DO_FSS", TRUE,
				   doFSSHelp, TRUE);
   discInfo->does_fss(decision);
}

   
/***************************************************************************
  Description : Display info
  Comments    :
***************************************************************************/
void DiscSearchInducer::display(MLCOStream& stream) const
{
   has_global_info();
   globalInfo->accEst.display_settings(stream);
   // stream << "Max expansions: " << maxExpansions << ", epsilon: " << epsilon
   //       << endl;
}

DEF_DISPLAY(DiscSearchInducer);


/***************************************************************************
  Description : Determine initial values for state
  Comments    : Side effect of building the global info's bounds.
***************************************************************************/
Array<int> *DiscSearchInducer::create_initial_info(InstanceBag*) 
{
   // compute global bounds for info
   has_global_info();
   ASSERT(globalInfo->class_id() == DISC_INFO);
   DiscInfo *discInfo = (DiscInfo *)globalInfo;
   ASSERT(globalInfo->trainList);
   const SchemaRC& schema = globalInfo->trainList->get_schema();

   Array<int>* initialSubset;
   ASSERT(globalInfo->inducer->class_id() == DF_INDUCER);
   DiscFilterInducer& dtf = ((class DiscFilterInducer&)
			     (*globalInfo->inducer));
   DiscDispatch& dispatcher = dtf.get_dispatcher();
   ASSERT(&dispatcher);
   
   // Initialize the first state
   if((direction == forward || direction == middle) && discInfo->does_fss())
      initialSubset = new Array<int>(0, globalInfo->trainList->num_attr(), 0);
   else
      initialSubset = new Array<int>(0, globalInfo->trainList->num_attr(), 1);

   if (direction == middle) {
      // @@ ugly trick to cause the disc vector to be erased.
      // @@ should really call free()
      dispatcher.set_disc_type(dispatcher.get_disc_type());
      //have the dispatcher build the discretizors 
      dispatcher.create_discretizors(*globalInfo->trainList);
      //Ask the discretizors for a good value
      for(int i = 0; i < dispatcher.discretizors()->size(); i++) {
	 if(dispatcher.discretizors()->index(i))
	    initialSubset->index(i)  
	       = dispatcher.discretizors()->index(i)->num_intervals_chosen();
      }
      if (middleRestricted)
	 discInfo->compute_bounds(schema, initialSubset);
      else
	 discInfo->compute_bounds(schema, NULL);	 
   } else { // not middle
      for(int i = 0; i < schema.num_attr(); i++)
	 if (schema.attr_info(i).can_cast_to_real())
	    initialSubset->index(i) = 1;
      discInfo->compute_bounds(schema, NULL);
   }
	 
   LOG(1, "Discretizors initial vector: " << endl);
   LOG(1, "[" << *initialSubset << "]" << endl);

   return initialSubset;
}


/***************************************************************************
  Description : Convert a state description to a Categorizer
  Comments    : This function is conceptually const, although it needs
                  to temporarily un-const members inside, so we need to
		  cast away constness.
***************************************************************************/
Categorizer *DiscSearchInducer::state_to_categorizer(
   const State<Array<int>, AccEstInfo>& state) const
{
   // Use global info's inducer as a DiscFilterInducer
   ASSERT(globalInfo);
   if ( DF_INDUCER != globalInfo->inducer->class_id())
      err << "DiscState::eval: baseInducer not a DFInducer." << fatal_error;   
   DiscFilterInducer& dtb = *((class DiscFilterInducer*)
				 globalInfo->inducer);
   dtb.set_disc(state.get_info());

   DiscSearchInducer *thisNC = (DiscSearchInducer *)this;
   dtb.assign_data(thisNC->TS);			// sets TS to NULL
   dtb.train();

   Categorizer *theCat = dtb.get_categorizer().copy();
   
   // restore state
   thisNC->TS = dtb.release_data();

   return theCat;
}


