// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : IBInducer generates an IB categorizer.
                 It has nearest neighbor, editing, max epochs, normalization,
		    and weights options. Depending on the options, it will
		    generate different categorizers.
		 Before setting IB options, initialize() must be called.
  Comments     :
  Enhancements :
  Assumptions  :
  History      : Yeogirl Yun                                      7/4/95
                   Added copy constructor.
                 Yeogirl Yun                                      9/09/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <IBInducer.h>
#include <MLCStream.h>
#include <IBCategorizer.h>
#include <BagSet.h>
#include <CtrBag.h>
#include <GetOption.h>
#include <MEnum.h>

const MEnum normMethodEnum =
  MEnum("interquartile", InstanceBag::interquartile) <<
  MEnum("extreme", InstanceBag::extreme) <<
  MEnum("none", InstanceBag::none);

const MEnum voteMethodEnum =
  MEnum("equal",IBInducer::equal) <<
  MEnum("inverse-distance",IBInducer::inverseDistance);

const MEnum nnkValueEnum =
  MEnum("num-neighbors",IBInducer::numNeighbors) <<
  MEnum("num-distances",IBInducer::numDistances);


const MString KVAL_HELP =
  "NUM_NEIGHBORS means the number of nearest neighbors";

const MString EDIT_HELP =
  "EDITING does not store correctly categorized instances while "
  "iterating MAX_EPOCS times through the training set";

const MString EPOCH_HELP =
  "MAX_EPOCHS means the number of times data set is iterated to store "
  "the incorrectly categorized instances to the categorizer";

const MString NNK_HELP =
  "num-neighbors stores k nearest neighbors. it counts the same distant "
  "instances, while num-neighbors only counts different distant neighbors";

const MString NORM_HELP =
  "Normalize attribute values. interquartile takes 25/75 percent values "
  "as min/max. extreme is a usual normalization method";


const MString NEIGHBOR_HELP =
  "equal treats all the nearest instances equally. inverse-distance treats "
  "nearest instances in proportion to the inverse of the distance from "
  "query instance.";


/*****************************************************************************
  Description : Integrity check. maxEpoch should be greater than or
                  equal to 1.
  Comments    : 
*****************************************************************************/
void IBInducer::OK(int level) const
{
   (void)level;
   if (maxEpochs <= 0)
      err << "IBInducer::OK: max epochs value should be greater "
	     "than 0. The value assigned was " << maxEpochs << fatal_error;
   // Don't check weights size matching TS size because assign_bag
   //   can change this.
}

/******************************************************************************
  Description  : Constructor, destructor
  Comments     :
******************************************************************************/
IBInducer::IBInducer(const MString& dscr)
 : Inducer(dscr),
   categorizer(NULL),
   weights(NULL)
{
   initialize();
}

IBInducer::~IBInducer()
{
   clear();
}



/*****************************************************************************
  Description : Copy constructor. Don't copy TS and etc. Just initialize and
                  copy only log options.
  Comments    :
*****************************************************************************/
IBInducer::IBInducer(const IBInducer& source, CtorDummy)
   : Inducer(source, ctorDummy),
     categorizer(NULL),
     weights(NULL)
{
   initialize();
   ASSERT(FALSE); //@@ unimplemented
}   


/******************************************************************************
  Description  : Return TRUE iff the class has a valid categorizer.
  Comments     :
******************************************************************************/
Bool IBInducer::was_trained(Bool fatal_on_false) const
{
   if( fatal_on_false && categorizer == NULL )
      err << "IBInducer::was_trained: No categorizer, "
	     "Call train() to create categorizer" << fatal_error;
   return categorizer != NULL;
}




/******************************************************************************
  Description  : Returns the categorizer that the inducer has generated.
  Comments     :
******************************************************************************/
const Categorizer& IBInducer::get_categorizer() const
{
   was_trained(TRUE);
   return *categorizer;
}


/*****************************************************************************
  Description : Send the wieght vector to IBCategorizer.
  Comments    : 
*****************************************************************************/
void IBInducer::set_weights(const Array<Real>& weightsArray)
{
   delete weights;
   weights = new Array<Real>(weightsArray, ctorDummy);
   OK();
}



/*****************************************************************************
  Description : Sets the editing option. If editing option is FALSE the inducer
                  is IB1. If it is TRUE then the inducer can be IB2 or
		  something similar depending on maxEpochs.
		The editing option defaults to FALSE.
  Comments    : 
*****************************************************************************/
void IBInducer::set_editing(Bool edit)
{
   editing = edit;
   if (!editing) // since editing is off, set maxEpochs to 1
      set_editing_max_epochs(1);
   else // set maxEpochs to default value
      set_editing_max_epochs();
}



/*****************************************************************************
  Description : Sets the maxEpochs value in editing mode(IB2 or
                  something similar)
  Comments    : 
*****************************************************************************/
void IBInducer::set_editing_max_epochs(int num)
{
   maxEpochs = num;
   OK();
}



/******************************************************************************
  Description  : Trains IB categorizer and concept description.
                   Simply put instances in TS to CD.
  Comments     : 
******************************************************************************/


void IBInducer::train()
{
   OK();
   has_data();

   if (manualWeights && !weights) {
      weights = new Array<Real>(0, TS->num_attr(), 0);
      for (int i = 0; i < TS->num_attr(); i++) {
	 Mcout << "Enter weight for attribute " << i << "(" <<
	    TS->get_schema().attr_name(i) << "): " << flush;
	 Mcin >> weights->index(i);
	 Mcout.reset_pos_in_line();
      }
   }
   // normalize attributes
   if (normMethod == InstanceBag::interquartile) {
      LOG(4, "Interquartile is ON " << endl;);
      LOG(4, "Before normalization : " << endl;);
      IFLOG(4, TS->display(get_log_stream()););
      TS->normalize_bag(InstanceBag::interquartile);
      LOG(4, "After normalization : " << endl;);
      IFLOG(4, TS->display(get_log_stream(), TRUE););
   }
   else if (normMethod == InstanceBag::extreme) {
      LOG(4, "Extreme is ON " << endl;);
      LOG(4, "Before normalization : " << endl;);
      IFLOG(4, TS->display(get_log_stream()););
      TS->normalize_bag(InstanceBag::extreme);
      LOG(4,  "After normalization : " << endl;);
      IFLOG(4, TS->display(get_log_stream(), TRUE););      
   }
   else; // do nothing.
   
   MString str("IB Categorizer");

   delete categorizer;

   InstanceBag emptyBag(TS->get_schema());
   categorizer = new IBCategorizer(str, emptyBag);
   categorizer->set_log_options(get_log_options());
   categorizer->set_k_val(kVal);
   categorizer->set_neighbor_vote(vote);
   categorizer->set_nnk_value(nnkValue);   
   if(weights)
      categorizer->set_weights(*weights);
   
   Bool insertInLastEpoch = TRUE;  // to go first time through loop
   for (int epoch = 0; epoch < maxEpochs && insertInLastEpoch; epoch++)  {
      insertInLastEpoch = FALSE; 
      for (Pix pix = TS->first(); pix; TS->next(pix)) {
	 const InstanceRC& inst = TS->get_instance(pix);
	 if (editing == FALSE) 
	    categorizer->add_instance(inst);
	 else {
	    const AugCategory& catValue = categorizer->categorize(inst);
	    if (catValue.num() !=
		inst.label_info().get_nominal_val(inst.get_label())) {
	       categorizer->add_instance(inst);
	       insertInLastEpoch = TRUE;
	    }
	 }
      }
   }

}



/***************************************************************************
  Description : set number of nearest neighbors.
  Comments    :
***************************************************************************/
void IBInducer::set_k_val(int k)
{
   if (k <= 0)
      err << "IBInducer::set_k_val() : illegal k value : " << k 
	  << fatal_error;
   kVal = k;
}



/*****************************************************************************
  Description : Set interquartile option.
  Comments    : 
*****************************************************************************/
void IBInducer::set_norm_method(InstanceBag::NormalizationMethod method)
{
   normMethod = method;
}


/*****************************************************************************
  Description : Clears categorizer and weights pointers.
  Comments    :
*****************************************************************************/
void IBInducer::clear()
{
   delete categorizer;
   delete weights;
   weights = NULL;
   categorizer = NULL;
}



/*****************************************************************************
  Description : Initializes options.
  Comments    :
*****************************************************************************/
void IBInducer::init()
{
   set_editing();
   set_editing_max_epochs();
   set_k_val();
   set_norm_method();
   set_neighbor_vote();
   set_nnk_value();
   set_manual_weights();
}


/*****************************************************************************
  Description : Initialize all options into default values and delete weights. 
  Comments    :
*****************************************************************************/
void IBInducer::initialize()
{
   clear();
   init();
}



/*****************************************************************************
  Description : Returns the pointer to the categorizer. It gives up the
                  ownership.
  Comments    :
*****************************************************************************/
Categorizer* IBInducer::get_free_categorizer()
{
   was_trained(TRUE);
   Categorizer *ret = categorizer;
   categorizer = NULL;
   return ret;
}



/*****************************************************************************
  Description : Sets the options from environment variables.
  Comments    :
*****************************************************************************/
void IBInducer::set_user_options(const MString& preFix)
{
   set_k_val(get_option_int(preFix + "NUM_NEIGHBORS", 1,KVAL_HELP));
   Bool editOption = get_option_bool(preFix + "EDITING", FALSE, EDIT_HELP);
   set_editing(editOption);
   if (editOption)
      set_editing_max_epochs(get_option_int(preFix + "MAX_EPOCHS", 3,
					    EPOCH_HELP));
   set_nnk_value(get_option_enum(preFix + "NNKVALUE", nnkValueEnum,
				  IBInducer::numDistances, NNK_HELP, FALSE));

   set_norm_method(get_option_enum(preFix + "NORMALIZATION", normMethodEnum,
				   InstanceBag::extreme, NORM_HELP, FALSE));

   set_neighbor_vote(get_option_enum(preFix + "NEIGHBOR_VOTE",
		     voteMethodEnum, IBInducer::inverseDistance,
		     NEIGHBOR_HELP, FALSE));

   manualWeights = get_option_bool(preFix + "MANUAL_WEIGHTS", 
				     FALSE, "Input weights manually");
}   


/*****************************************************************************
  Description : Returns the pointer to the copy of IBInducer.
  Comments    :
*****************************************************************************/
Inducer* IBInducer::copy() const
{
   Inducer *ind = new IBInducer(*this, ctorDummy);
   return ind;
}
