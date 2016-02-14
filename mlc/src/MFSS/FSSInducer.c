// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Wrapper inducer for automatic feature subset selection.
                 The feature selection is a best-first search from the initial
 		   state of no attributes (features).
  Assumptions  :
  Comments     : 
  Complexity   : Training is the number of states searched times the
                   estimation time per state.
  Enhancements :
  History      : Dan Sommerfield                                     5/24/95
                   Fit into new (SearchInducer) framework
                 Ronny Kohavi                                       10/25/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <FSSInducer.h>
#include <CtrInstList.h>
#include <BFSearch.h>
#include <HCSearch.h>
#include <SASearch.h>

#include <env_inducer.h>
#include <GetOption.h>

RCSID("MLC++, $RCSfile: FSSInducer.c,v $ $Revision: 1.18 $")


// direction option
const MEnum directionEnum =
  MEnum("forward", FSSInducer::forward) <<
  MEnum("backward", FSSInducer::backward);
const MString directionHelp = "This option chooses the direction in which to "
  "search.  Forward causes the search to begin with an empty subset of "
  "features, while backward causes the search to begin with a full subset.";
const FSSInducer::Direction defaultDirection = FSSInducer::forward;

/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
FSSInducer::FSSInducer(const MString& description, BaseInducer *ind)
   : SearchInducer(description, ind),
     direction(defaultDirection)
{
   // establish gloabal info (required)
   globalInfo = create_global_info();
}


/***************************************************************************
  Description : Get extra options from the user
  Comments    :
***************************************************************************/
void FSSInducer::set_user_options(const MString& prefix)
{
   SearchInducer::set_user_options(prefix);

   // also pick a starting point (direction)
   direction =
      get_option_enum(prefix + "DIRECTION", directionEnum,
		      defaultDirection,
		      directionHelp, TRUE);
}

   
/***************************************************************************
  Description : Display info
  Comments    :
***************************************************************************/
void FSSInducer::display(MLCOStream& stream) const
{
   SearchInducer::display(stream);
   //@@ ...
}


DEF_DISPLAY(FSSInducer);

/***************************************************************************
  Description : Create the initial state information
  Comments    : All TRUE for backward searches, all FALSE for forward.
***************************************************************************/
Array<int> *FSSInducer::create_initial_info(InstanceBag*)
{
   has_global_info();
   switch(direction) {
      case forward:
	 return new Array<int>(0, globalInfo->trainList->num_attr(), 0);
      case backward:
	 return new Array<int>(0, globalInfo->trainList->num_attr(), 1);
      default:
	 ASSERT(FALSE);
	 return NULL;
   }
}


/***************************************************************************
  Description : Convert a state description to a Categorizer
  Comments    : This function is conceptually const, although it needs
                  to temporarily un-const members inside, so we need to
		  cast away constness.
***************************************************************************/
Categorizer *FSSInducer::state_to_categorizer(
   const State<Array<int>, AccEstInfo>& state) const
{
   BoolArray *attrMask = new BoolArray(state.get_info(), ctorDummy);
   FSSInducer *thisNC = (FSSInducer *)this;
   
   // knock out any data owned by the globalInfo's inducer
   InstanceBag *oldData = thisNC->globalInfo->inducer->release_data();

   // use attribute mask to create categorizer:
   ProjectInd projInd("ProjectInd for FSS Categorization");
   projInd.set_wrapped_inducer(thisNC->globalInfo->inducer);

   projInd.set_project_mask(attrMask);
   delete projInd.assign_data(thisNC->TS);		 // sets TS to NULL
   projInd.train();

   Categorizer *theCat = projInd.release_categorizer();

   // restore state
   thisNC->globalInfo->inducer = projInd.release_wrapped_inducer();
   ASSERT(globalInfo->inducer);
   delete thisNC->globalInfo->inducer->assign_data(oldData);
   thisNC->TS = projInd.release_data();

   return theCat;
}



