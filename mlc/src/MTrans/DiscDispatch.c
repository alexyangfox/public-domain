// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : DiscDispatch is the high-level interface to the MLC++
                   discretization utilities. A discdispatch can be used to
		   discretize a dataset and it's associated testbag as well
		   as modify how the discretization behaves.
		 In order to set the parameters such as the discretization 
		   algorithm used, the number of bins, etc. you must either
		   call set_user_options() on an instance of a dispatcher or
		   you can modify the default values provided via the
		   programmable interfaces provided.
		 In order to use the discretization dispatcher you must
		   first call DiscDispatch::create_discretizors() on your
		   instance of a dispatcher passing the DATA bag as the
		   actual. Successive calls to this routine recreates the
		   discretizors used to discretize the continuous attributes
		   in the bag. Once this method has been called, you can
		   request your dispatcher to discretize the TEST bag by
		   calling DiscDispatch::discretize_bag().
  Assumptions  :
  Comments     : Note that when discretize_bag() is invoked the schema of
                   the bag used as an actual parameter must be the same
		   as the schema of the bag used when create_discretizors()
		   is called.
		 Changes DO NOT take affect to the dispatcher until
		   create_discretizors() is called. This means that if you
		   change the discretization type, initial vector value, or
		   any of the programmable parameters on the dispatcher you
		   must call create_discretizors() again.
		 The default value of 0 (zero) for the initial discretization
		   vector activates heuristics specific to a discretization
		   type. The exception to this is the OneR discretizor.
		 IT IS VERY IMPORTANT that the user realize that when they
		   invoke create_discretizors() all references to either
		   the discretization vector or the discretizors become
		   null and void. This means that if you create a reference
		   to the discretizors (via. discretizors()) or the
		   discretization vector (via. disc_vect()) and then
		   invoke create_discretizors() again, a segmentation
		   violation or generally cursed code is imminent.
  Complexity   :
  Enhancements : Add other discretization methods once they become available.
  History      : James F. Dougherty                                 02/27/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <DiscDispatch.h>


RCSID("MLC++, $RCSfile: DiscDispatch.c,v $ $Revision: 1.18 $")

const MEnum discretizationTypeEnum = MEnum("1r" , DiscDispatch::oneR) <<
                                   MEnum("bin" , DiscDispatch::binning) <<
                                   MEnum("entropy", DiscDispatch::entropy) <<
                                   MEnum("c4.5-disc", DiscDispatch::c45Disc)<<
                                   MEnum("t2-disc", DiscDispatch::t2Disc);

const MEnum binEnum = MEnum("Algo-heuristic", DiscDispatch::AlgoHeuristic) <<
                      MEnum("Fixed-value", DiscDispatch::FixedValue) <<
                      MEnum("MDL", DiscDispatch::MDL);

const MString discretizorHelp = "Use this option to select the type of"
" discretization method you would like: \"bin\" (for uniform binning), "
" \"1r\" for Holte's OneR discretization technique, or \"entropy\" "
" for Entropy discretization, \"c4.5-disc\" for C4.5 discretization,"
" or \"t2-disc\" for T2 (dynamic programming) discretization.";

const MString binHelp = "Select the way to determine the number of intervals. "
  " Algo-heuristic leaves it to the specific algorithm.  Fixed-value "
  " allows you to specify a single value.  MDL uses the entropy MDL "
  "heuristic.";

const MString binningHelp = "Choose the number of equally spaced ranges"
" or \"bins\" you would like to generate.";

const MString oneRHelp = "Choose the minimum number of instances per "
"label. This is also known as the \"Small\" parameter.";

const MString entropyHelp = "Choose the minSplit parameter.";
const MString initValHelp = "Choose the initial value for the number of"
"intervals (binning /entropy), or the minimum number of instances per label."
"Specifying zero defaults to a heuristic for Binning and Entropy. ";



/***************************************************************************
  Description : Constructor
  Comments    :
***************************************************************************/
DiscDispatch::DiscDispatch()
   :discVect(NULL),
    schema(NULL),
    disc(NULL),
    discType(DiscDispatch::entropy),     // defaults to binning discretization
    binHeuristic(DiscDispatch::AlgoHeuristic),
    minSplit(1),                         // minSplit is 1
    initVal(0),                          // <--- for heuristics to kick in
    minInstPerLabel(OneR::defaultSmall)  // This is 5 or 6 (see OneR.c)
{}

/***************************************************************************
  Description : Display vector
  Comments    :
***************************************************************************/

void DiscDispatch::display(MLCOStream& stream) const
{
   for(int i = 0; i < disc->size(); i++) {
      if(disc->index(i))
	 stream << disc->index(i)->num_intervals_chosen();
      else
	 stream << "*";
      if (i < disc->size()-1)
	 stream << ", ";
   }
}

// helper function for displaying more info
void display_thresholds(const PtrArray<RealDiscretizor*>& disc,
			MLCOStream& stream)
{
   for (int i = 0; i < disc.size(); i++)
      if(disc.index(i))
	 stream << *disc.index(i) << endl;
}

DEF_DISPLAY(DiscDispatch)

/***************************************************************************
  Description : Sets up the discretizor's and the discVect from environment
                  variables.
  Comments    :
***************************************************************************/
void DiscDispatch::set_user_options(const MString& prefix)
{
   discType = get_option_enum(prefix + "DISC_TYPE", discretizationTypeEnum,
			      DiscDispatch::entropy, discretizorHelp,
			      TRUE);

   if (discType != DiscDispatch::c45Disc)
      binHeuristic = get_option_enum(prefix + "DISC_NUM_INTR",
	   binEnum, DiscDispatch::AlgoHeuristic, binHelp, TRUE);
				     
   
   if (discType == DiscDispatch::entropy)
      minSplit = get_option_int( prefix + "MIN_SPLIT",
				 EntropyDiscretizor::defaultMinSplit,
				 entropyHelp);

   if (discType == DiscDispatch::oneR)
      minInstPerLabel = get_option_int("MIN_INST",            
				       oneRHelp);        

   if (discType != DiscDispatch::oneR && discType != DiscDispatch::c45Disc &&
       binHeuristic == DiscDispatch::FixedValue) 
      initVal = get_option_int( prefix + "INITIAL_VAL",
				0, initValHelp);
}

/***************************************************************************
  Description : set bin heuristic
  Comments    :
***************************************************************************/
void DiscDispatch::set_bin_heuristic(NumBins h)
{
   binHeuristic = h;
   free();
}


/***************************************************************************
  Description : Sets the discretization vector. Acquires ownership.
  Comments    : Reinvoke create_discretizors() for changes to take effect.
***************************************************************************/
void DiscDispatch::set_disc_vect(Array<int>*& aDiscVect)
{
   if (!aDiscVect)
      err << "DiscDispatch::set_disc_vect: you can't give me a null vector."
	  << fatal_error;
   free();
   discVect = aDiscVect;
   aDiscVect = NULL;
}

/***************************************************************************
  Description : Returns a reference to the discretization parameter vector.
  Comments    : See warning above.
***************************************************************************/
const Array<int>* DiscDispatch::disc_vect()
{
   OK();
   return discVect;
}

/***************************************************************************
  Description : Returns a reference to the discretizors
  Comments    : See warning above.
***************************************************************************/
PtrArray<RealDiscretizor*>* DiscDispatch::discretizors()
{
   OK();
   return disc;
}


/***************************************************************************
  Description : Sets the discretization type for the dispatcher.
  Comments    : Invoke create_discretizors for this to come into effect.
***************************************************************************/
void DiscDispatch::set_disc_type(DiscretizationType discType)
{
   free();
   DiscDispatch::discType = discType;
}


/***************************************************************************
  Description : Invariant validation method. Object OK after
                  create_discretizors is called.
  Comments    :
***************************************************************************/
void DiscDispatch::OK(int /* level */) const
{
//   It's fine to have these be set to NULL now.
//   if ( schema == NULL || disc == NULL || discVect == NULL)
//      err << "DiscDispatch::OK(): create_discretizors not called."
//	  << fatal_error;
}

/***************************************************************************
  Description : Sets the initial value of the discretization vector
  Comments    :
***************************************************************************/
void DiscDispatch::set_initial_val(int newVal)
{
   if (get_bin_heuristic() != FixedValue)
      err << "DiscDispatch::set_initial_val: bin heuristic must be FixedValue"
          << fatal_error;
   free();
   initVal = newVal;
}

/***************************************************************************
  Description : Calls discretize_bag() to discretize the bag. If the bag
                 passed to the dispatcher does not have the same schema as
		 the one that is used in create_discretizors(), it is an
		 error.
  Comments    : See RealDiscretizor.c
***************************************************************************/
InstanceBag* DiscDispatch::discretize_bag(const InstanceBag& bagToMakeDiscrete)
{
   OK();
   if (*schema != bagToMakeDiscrete.get_schema()){
      Mcout << "DiscDispatch: create_bag: bad bag passed. Schema's not the "
	    << "same. Either recreate your discretizors or pass correct bag. "
	    << endl << "Schema dump: ";
      schema->display();
      err << "DiscDispatch::discretize_bag: fatal error,"
	  << fatal_error;
   }	  
   return ::discretize_bag(bagToMakeDiscrete, disc);
}


/***************************************************************************
  Description : Creates the discretizors and makes the dispatcher usable.
  Comments    : 
***************************************************************************/
void DiscDispatch::create_discretizors(const InstanceBag& sourceBag)
{
   delete schema; //new bag
   schema = new SchemaRC(sourceBag.get_schema());
   allocate(sourceBag);
}


/***************************************************************************
  Description : Returns a deep copy of the discretizors.
  Comments    :
***************************************************************************/
PtrArray<RealDiscretizor*>* DiscDispatch::disc_copy()
{
   OK();
   PtrArray<RealDiscretizor*>* newDisc
      = new PtrArray<RealDiscretizor*>(disc->size());
   for(int i = 0; i < disc->size(); i++)
      if( disc->index(i) )
	 newDisc->index(i) = disc->index(i)->copy();
   return newDisc;
}


/***************************************************************************
  Description : Assign the discretization vector based on heuristic.
  Comments    : private function.
***************************************************************************/

void DiscDispatch::set_disc_vector(const InstanceBag& bag)
{
   if (!discVect) {
      if (binHeuristic == AlgoHeuristic)
	 discVect = new Array<int>(0, bag.get_schema().num_attr(), 0);
      else if (binHeuristic == FixedValue) {
	 ASSERT(initVal >= 0);
	 discVect = new Array<int>(0, bag.get_schema().num_attr(), initVal);
      } else if (binHeuristic == MDL) {
	 discVect = new Array<int>(0, bag.get_schema().num_attr(), 0);
         disc = create_Entropy_discretizors(logOptions, bag, 
			   EntropyDiscretizor::defaultMinSplit, *discVect);
	 LOG(2, "Recommended bins by MDL heuristic: " << *this << endl);
	 for(int i = 0; i < disc->size(); i++) {
	    if(disc->index(i))
	       discVect->index(i) = disc->index(i)->num_intervals_chosen();
	    else
	       discVect->index(i) = -1; // ignored
	 }
	 delete disc;
      }
      else
	 ASSERT(FALSE);
   }
}

/***************************************************************************
  Description : Builds discretizors based on the type, will automatically
                  create discretization parameter vector if not initialized.
  Comments    : private method
***************************************************************************/


void DiscDispatch::allocate(const InstanceBag& bag)
{
   LogOptions logOptions(get_log_options());
   logOptions.set_log_level(get_log_level() - 1);
   delete disc; disc = NULL; //new discretizors
   Bool newDiscVect = (discVect == NULL);
   if (discVect && discVect->size() != bag.num_attr()) 
      err << "DiscDispatch::allocate: bad discVect size: " << discVect->size()
	 << " Bag has " << bag.num_attr() << " attributes" << fatal_error;

   //OneR has no default heuristic so we use SMALL default if 0
   if (discType == oneR){ 
      initVal = minInstPerLabel;
      if (initVal == 0){
	 initVal = OneR::defaultSmall;
	 LOG(1, "DiscDispatch::allocate; 0 specified for 1R initial value."
	     << "Using (" << initVal << ")." << endl);
      }
      set_disc_vector(bag);
      // It could be that the array was set, but with zeros, which
      //   for oneR is bad.  In these cases we reset it.
      if (discVect->min() <= 0){
	 if (discVect->max() > 0)
	    err << "DiscDispatch::allocate: OneR does not support 0 "
	           " intervals for some attributes but not for all"
	        << fatal_error;
         discVect->init_values(initVal);
	 LOG(1, "DiscDispatch::alocate; 0 bin specified in 1R initial vector."
	     << "Using (" << *discVect << ")." << endl);
      }
      ASSERT(disc == NULL);
      disc = create_OneR_discretizors(logOptions, bag, *discVect);
   } // Binning uses the K * Log(N) default heuristic if initVal is zero
   else if(discType == binning) {
      set_disc_vector(bag);
      disc = create_binning_discretizors(logOptions, bag, *discVect);
   } // Use MDL default heuristic if initVal is zero
   else if(discType == entropy) {
      set_disc_vector(bag);
      disc = create_Entropy_discretizors(logOptions, bag, minSplit, *discVect);
   } else if (discType == c45Disc) {
      set_disc_vector(bag);  // values are ignored, but must be set.
      disc = create_c45_discretizors(logOptions, bag);
   } else if (discType == t2Disc) {
      set_disc_vector(bag);
      disc = create_t2_discretizors(logOptions, bag, *discVect);
   } else
      err <<"DiscDispatch::allocate: unknown discretization" << fatal_error;

   if (newDiscVect) { // If we built it, let's destroy it.
      delete discVect;
      discVect = NULL;
   }
   LOG(1,  "Discretization vector:" << *this << endl);
   IFLOG(3, display_thresholds(*disc, get_log_stream()));
}


/***************************************************************************
  Description : Cleans up.
  Comments    : private method
***************************************************************************/
void DiscDispatch::free()
{
   delete discVect;
   discVect = NULL;
   delete disc;
   disc = NULL;
   delete schema;
   schema = NULL;
}




