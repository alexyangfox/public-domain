// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The Inducer class "induces" a concept from a labelled
                   training set (supervised learning).
                 An Inducer is really an "Internal inducer," that is,
		   one that we have a categorizer for (as opposed to
		   the BaseInducer which may be external.
		 The main routines added to BaseInducer are train()
		   predict() (for a single instance), and get_categorizer().
		 Train_and_test() is implemented here in terms of the above.
  Assumptions  : 
  Comments     : Subclassed by many inducers -- see appropriate .c
                   files (e.g. TableInducer, ConstInducer).
  Complexity   : See BaseInducer and derived classes.
		   display_struct(), predict() have the complexity of the
  		     appropriate categorizer.
  Enhancements : 
  History      : Yeogirl Yun                                         74/95
                   Added copy() and copy constructor.
                 Ronny Kohavi                                       10/7/94
                   Rewrote as subclass of BaseInducer.
		 Richard Long                                       8/17/93
                   Initial revision (.c)
		 Ronny Kohavi                                       6/23/93
		   Initial revision (.h)
***************************************************************************/

#include <basics.h>
#include <Inducer.h>
#include <Categorizer.h>
#include <CatTestResult.h>
#include <CtrInstList.h>

RCSID("MLC++, $RCSfile: Inducer.c,v $ $Revision: 1.43 $")


/*****************************************************************************
  Description : Copy constructor. 
  Comments    :
*****************************************************************************/
Inducer::Inducer(const Inducer& source, CtorDummy)
   : BaseInducer(source, ctorDummy)
{}

Real Inducer::train_and_test(InstanceBag* trainingSet,
			     const InstanceBag& testBag) 
{
   DBG_DECLARE(InstanceBag* callersSet = trainingSet;)
      
   InstanceBag *oldList = assign_data(trainingSet);
   train();
   
   CatTestResult results(get_categorizer(),
			 instance_bag().cast_to_instance_list(), testBag);
   LOG(1, "Inducer::train_and_test for inducer " << description() <<
       endl << results);

   trainingSet = assign_data(oldList); // get back ownership of user's set

   DBG(ASSERT(trainingSet == callersSet));

   return results.accuracy();
}



/***************************************************************************
  Description : Convenience functions.
  Comments    :
***************************************************************************/


AugCategory Inducer::predict(const InstanceRC& instance) const
{
   return get_categorizer().categorize(instance);
}
   
void Inducer::display_struct(MLCOStream& stream, const DisplayPref& dp) const
{
   get_categorizer().display_struct(stream, dp);
}

/***************************************************************************
  Description : Cast to inducer functions
  Comments    :
***************************************************************************/

Bool Inducer::can_cast_to_inducer() const
{
   return TRUE;
}

Inducer& Inducer::cast_to_inducer() 
{
   // Note that a class may override can_cast_to_inducer(), so this
   //   ensures consistency and avoids the need to override cast_to_inducer().
   //   This happens in wrapper inducers that may or may not wrap around
   //   a real inducer.
   if (!can_cast_to_inducer())
      err << "Inducer " << description() << " (class id " << class_id()
	  << ") cannot be cast into an inducer" << fatal_error;

   return (Inducer&)(*this);
}


/*****************************************************************************
  Description : Returns the copy of inducer. Derived inducers should implement
                  correct version of this function.
  Comments    :
*****************************************************************************/
Inducer *Inducer::copy() const
{
   err << "Inducer::copy(): derived classes has no implemented copy "
      "method. " << fatal_error;

   return (Inducer *)NULL;
}



/*****************************************************************************
  Description : Get AttrOrder object.
  Comments    :
*****************************************************************************/
AttrOrder& Inducer::get_attr_order_info()
{
   err << "This inducer does not support AttrOrder. Please use the inducers "
      "such as ListHOODGE or TableCascade." << fatal_error;
   return *(AttrOrder *)(NULL_REF); 
}
