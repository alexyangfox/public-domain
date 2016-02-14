// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Special Inducer which discretizes the data/test set before
                   invoking an inducer via. env_inducer().
  Assumptions  : 
  Comments     : Can be used standalone for discretization of datasets before
                   running an inducer or called automatically by DiscInducer.

		 If you call get_dispatch() and do not return the dispatcher
		   (or set one) a subsequent call to train_and_test() will
		   fail. Just return the dispatcher with set_disc().
		   
  Complexity   :
  Enhancements :
  History      : James Dougherty                                02/14/95
                   Initial revision (.h,.c)
***************************************************************************/


#include <basics.h>
#include <DFInducer.h>
#include <DiscDispatch.h>
#include <DiscCat.h>
#include <env_inducer.h>


RCSID("MLC++, $RCSfile: DFInducer.c,v $ $Revision: 1.14 $")

/***************************************************************************
  Description : Constructor
  Comments    : 
***************************************************************************/
DiscFilterInducer::DiscFilterInducer(const MString& descr)
   : CtrInducer(descr),
     categorizer(NULL),
     discVect(NULL),
     inducer(NULL),
     dispatcher(new DiscDispatch())
{}


/***************************************************************************
  Description :  Destructor
  Comments    :
***************************************************************************/
DiscFilterInducer::~DiscFilterInducer()
{
 delete inducer;
 delete discVect;
 delete dispatcher;
 delete categorizer;
}


/***************************************************************************
  Description : Trains the inducer using the wrapped inducer we get
                  from env_inducer().
  Comments    :
***************************************************************************/
void DiscFilterInducer::train()
{
   has_data();

   delete categorizer;
   categorizer = NULL;
   
   if (!inducer || !dispatcher)
      err << "DiscFilterInducer::train: must call set_user_options first."
	  << fatal_error;

   // Make the discretizors using the training set, discretize TS
   dispatcher->set_log_level(get_log_level());
   dispatcher->create_discretizors(*TS);
   InstanceBag* discTSBag = dispatcher->discretize_bag(*TS);

   SchemaRC discSchema
      = gen_discretized_schema(*TS, dispatcher->discretizors());

   // Release old inducer's data and assign current training set.
   delete inducer->assign_data(discTSBag);
   inducer->set_log_level(get_log_level());
   inducer->cast_to_inducer().train();

   Categorizer* cat = inducer->cast_to_inducer().get_categorizer().copy();
   PtrArray<RealDiscretizor*>* disc = dispatcher->disc_copy();
   categorizer = new DiscCat(MString("DiscCat"), disc, discSchema, cat);
   categorizer->set_log_level(get_log_level());

   ASSERT(disc == NULL); // Sanity check
   ASSERT(cat == NULL);  // they should be owned ...
}



/***************************************************************************
  Description : Returns true if the categorizer has not been allocated,
                  aborting if fatalOnFalse.
  Comments    :
***************************************************************************/
Bool DiscFilterInducer::was_trained(Bool fatalOnFalse) const
{
   if (fatalOnFalse && !categorizer) {
      err << "DiscFilterInducer::was_trained: -not trained and fatalOnFalse."
	  << fatal_error;
      return FALSE;
   } else 
      return categorizer != NULL;
}


/***************************************************************************
  Description : Returns the discretizing categorizer.
  Comments    :
***************************************************************************/
const Categorizer& DiscFilterInducer::get_categorizer() const
{
   was_trained(TRUE);
   return *categorizer;
}


/***************************************************************************
  Description : Sets the discretization vector which the inducer uses to
                  discretize its training and testing data.
  Comments    :
***************************************************************************/
void DiscFilterInducer::set_disc(const Array<int>& sourceDiscVect)
{
   delete discVect;
   discVect = new Array<int>(sourceDiscVect, ctorDummy);

   if(!dispatcher)
      err << "DiscFilterInducer::set_disc: no dispatcher." << fatal_error;
   dispatcher->set_disc_vect(discVect);
}


/***************************************************************************
  Description : Discretizes the bags, trains and tests the inducer.
  Comments    :
***************************************************************************/
Real DiscFilterInducer::train_and_test(InstanceBag* trainingSet,
				       const InstanceBag& testBag)
{
   if (!inducer || !dispatcher)
      err << "DiscFilterInducer::train_and_test: must call set_user_"
	  << "options first or verify_inducer/dispatcher" << fatal_error;

   dispatcher->set_log_level(get_log_level());
   dispatcher->create_discretizors(*trainingSet);
   InstanceBag* discTrainBag = dispatcher->discretize_bag(*trainingSet);
   InstanceBag* discTestBag = dispatcher->discretize_bag(testBag);

   inducer->set_log_level(get_log_level());
   Real acc = inducer->train_and_test(discTrainBag, *discTestBag);

   delete discTrainBag;
   delete discTestBag;
   return acc;
}

/***************************************************************************
  Description : Displays the categorizer on the stream.
  Comments    :
***************************************************************************/
void DiscFilterInducer::display_struct(MLCOStream& stream,
					const DisplayPref& dp) const
{
   if (stream.output_type() != XStream &&
	dp.preference_type() == DisplayPref::ASCIIDisplay)
      stream << "Discretizing Filter Inducer " << description() << endl;
   // Just pass this down the line.
   get_categorizer().display_struct(stream, dp);
}

/***************************************************************************
  Description : Returns true if the wrapped inducer can cast to inducer.
  Comments    :
***************************************************************************/
Bool DiscFilterInducer::can_cast_to_inducer() const
{
   if(!inducer)
      err << "DiscFilterInducer::can_cast_to_inducer: call train first"
	  << fatal_error;
   return inducer->can_cast_to_inducer();
}



/*****************************************************************************
  Description : Allocates a dispatcher and sets it up from the environment.
  Comments    :
*****************************************************************************/
void DiscFilterInducer::set_user_options_no_inducer(const MString& prefix)
{
   ASSERT(dispatcher != NULL);
   dispatcher->set_user_options(prefix + "DISCF_");
}


/*****************************************************************************
  Description : Gets the inducer from the environment. Allocates a dispatcher
                  and sets it up from the environment.
  Comments    :
*****************************************************************************/
void DiscFilterInducer::set_user_options(const MString& prefix)
{
   if (!inducer) {
      inducer = env_inducer("DISCF_");
      ASSERT(inducer != NULL); 
   }

   ASSERT(dispatcher != NULL);
   dispatcher->set_user_options(prefix + "DISCF_");
}

/***************************************************************************
  Description : Acquires ownership of the pointer to the inducer and uses
                  it as the wrapped inducer.
  Comments    :
***************************************************************************/
void DiscFilterInducer::set_inducer(BaseInducer*& baseInd)
{
   inducer = baseInd;
   baseInd = NULL;
}

/***************************************************************************
  Description : Returns the inducer. The caller obtains ownership.
  Comments    :
***************************************************************************/
BaseInducer* DiscFilterInducer::get_inducer() 
{
   BaseInducer* returnedInducer = inducer;
   inducer = NULL;
   return returnedInducer;
}
   
/***************************************************************************
  Description : Sets the dispatcher to handle the preprocessing.
  Comments    : Returns the old dispatcher.
***************************************************************************/
DiscDispatch* DiscFilterInducer::set_dispatch(DiscDispatch*& dispatcher)
{
   if (!dispatcher)
      err << "DiscFilterInducer::set_dispatch: NULL dispatcher."
	  << fatal_error;
   DiscDispatch* d = DiscFilterInducer::dispatcher;
   DiscFilterInducer::dispatcher = dispatcher;
   dispatcher = NULL;
   return d;
}

/***************************************************************************
  Description : Returns our dispatcher, caller obtains ownership
  Comments    : You must return the dispatcher to the inducer in order for
                  it to perform train_and_test.
***************************************************************************/
DiscDispatch* DiscFilterInducer::get_dispatch()
{
   DiscDispatch* returnedDispatcher = dispatcher;
   dispatcher = NULL;
   return returnedDispatcher;
}
