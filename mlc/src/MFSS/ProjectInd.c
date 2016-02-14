// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/*****************************************************************************
  Description  : ProjectInd is an inducer that will prepare a projected
                   categorizer given a base inducer and a boolean mask
		   indicating the attributes to project.
  Assumptions  : 
  Comments     : 
  Complexity   :
  Enhancements : ProjectInd could have been derived from IncrInducer, so that
		    it would provide incremental functionality if the inducer
		    it is wrapping provides it.  However, the need to keep the
		    assigned data bag and the projected data bag made the
		    incremental del_instance O(n) complexity.  If the bag
		    functions would support quicker lookup, this option might
		    be worth it.  The code for these functions is included
		    in this file but commented out.
		 It isn't feasible to only use the wrapped inducer's data
		   (thus avoiding the O(n) penalty) because release_data
		   should give back what assign_data gets, and using the
		   wrapped inducer's data would result in a projected bag
		   being released.
  History      :
                 Robert Allen
                   Initial Revision(.c .h)                      1/27/95
*****************************************************************************/
#include <basics.h>
#include <MLCStream.h>
#include <ProjectInd.h>
#include <CtrBag.h>


RCSID("MLC++, $RCSfile: ProjectInd.c,v $ $Revision: 1.4 $")

/*****************************************************************************
  Description  :  The number of elements of the projection mask should be the
                    number of attributes in the data schema.
  Comments     :  
 *****************************************************************************/
void ProjectInd::OK(int level) const
{
   Inducer::OK(level);

   // if there is a wrapped inducer and it is a base inducer, there won't be
   // any TS on board to check the schema, so return.
   if ( has_wrapped_inducer(FALSE) &&
	! wrappedInducer->can_cast_to_inducer() )
	  return;
   
   if ( has_project_mask() &&
	has_data(FALSE) &&
	attrMask->size() != TS->get_schema().num_attr() )
      err << "ProjectInd::OK: Mask size (" << attrMask->size()
	  << ") does not match the number of attributes ("
	  << TS->get_schema().num_attr() << ") in the training set."
	  << fatal_error;
}


/*****************************************************************************
  Description  : Constructor with description MString.
  Comments     :  
*****************************************************************************/
ProjectInd::ProjectInd(const MString& description)
   : CtrInducer(description)
{
   wrappedInducer = NULL;
   attrMask = NULL;
   categorizer = NULL;
// shortSchema only needed for incremental enhancement
//   shortSchema = NULL;
}

/*****************************************************************************
  Description  : Deallocates members.
  Comments     :
*****************************************************************************/
ProjectInd::~ProjectInd()
{
   DBG(OK());
   delete categorizer;
   delete wrappedInducer;
   delete attrMask;
//      delete shortSchema;
}


/*****************************************************************************
  Description  : Set the wrapped Inducer.  Must be done before training.
                   Fatal error if wrapped Inducer already has data.
  Comments     : Gets ownership of wrapped Inducer.
*****************************************************************************/
void ProjectInd::set_wrapped_inducer(BaseInducer*& ind)
{
   if( ind->can_cast_to_inducer() && ind->has_data(FALSE) )
      err << "ProjectInd::set_wrapped_inducer: Wrapped inducer shouldn't "
	 "have data.  ProjectInd will assign projected data." << fatal_error;
   wrappedInducer = ind;
   ind = NULL;
}

/*****************************************************************************
  Description  : Get the wrapped inducer.  Fatal error if not set.
  Comments     : 
*****************************************************************************/
const BaseInducer& ProjectInd::get_wrapped_inducer() const
{
   has_wrapped_inducer(TRUE);
   return *wrappedInducer;
}


/*****************************************************************************
  Description  : Return and release ownership of the wrapped inducer.
		   Fatal error if no inducer.
  Comments     : 
*****************************************************************************/
BaseInducer* ProjectInd::release_wrapped_inducer()
{
   has_wrapped_inducer(TRUE);
   BaseInducer* retInd = wrappedInducer;
   wrappedInducer = NULL;
   return retInd;
}


/*****************************************************************************
  Description  : Return TRUE iff the class has an inducer.
  Comments     :
*****************************************************************************/
Bool ProjectInd::has_wrapped_inducer(Bool fatalOnFalse) const
{
   if( fatalOnFalse && wrappedInducer == NULL )
      err << "ProjectInd::has_wrapped_inducer: No inducer set.  Call "
	 "set_wrapped_inducer() first" << fatal_error;

   return ( wrappedInducer != NULL );
}


/*****************************************************************************
  Description  : Set the attribute projection mask.  Must be done before
                   training. Existing mask, if any, deleted.
  Comments     : 
*****************************************************************************/
void ProjectInd::set_project_mask(BoolArray*& attr)
{
   ASSERT(attr);
   if (attrMask)
      delete attrMask;
   attrMask = attr;
   DBG(OK());
   attr = NULL;
}


/*****************************************************************************
  Description  : Get the attribute projection mask.  Fatal error if not set.
  Comments     : 
*****************************************************************************/
const BoolArray& ProjectInd::get_project_mask() const
{
   has_project_mask(TRUE);
   return *attrMask;
}


/*****************************************************************************
  Description  : Return TRUE iff the class has an inducer.
  Comments     :
*****************************************************************************/
Bool ProjectInd::has_project_mask(Bool fatalOnFalse) const
{
   if( fatalOnFalse && wrappedInducer == NULL )
      err << "ProjectInd::has_project_mask: No mask set.  Call "
	 "set_project_mask() first" << fatal_error;

   return ( attrMask != NULL );
}


/*****************************************************************************
  Description  : Creates categorizer to handle projected subset of data.
  Comments     : 
*****************************************************************************/
void ProjectInd::train()
{
   has_wrapped_inducer();
   if (!wrappedInducer->can_cast_to_inducer())
      err << "ProjectInd::train(): wrapped inducer must be derived "
	 "from Inducer (not BaseInducer) in order to train." << fatal_error;
   Inducer& wInd = wrappedInducer->cast_to_inducer();
   has_project_mask();
   has_data();
   DBG(OK());
   delete categorizer;
   
// shortSchema only needed for incremental enhancement
//   delete shortSchema;
//   shortSchema = new SchemaRC(TS->get_schema().project(*attrMask));
   
   // project data from my TS and put into wrapped inducer
   InstanceBag* shortBag = TS->project(get_project_mask());
   
   // existing bag is returned by assign_data() & it should be deleted
   delete wInd.assign_data(shortBag);
   
   // wInd gets ownership of bag:
   ASSERT(shortBag == NULL);
   
   // Train wrapped inducer on projected data.
   wInd.train();
   
   // Copy categorizer from wrapped inducer.  Ownership of this copy may
   //   be released.
   Categorizer* newcat = wInd.get_categorizer().copy();
   
   // create ProjectCat
   categorizer = new ProjectCat(description(), get_project_mask(),
				TS->get_schema(), newcat);

   categorizer->set_log_level(get_log_level());
}


/*****************************************************************************
  Description  : When wrapped inducer is a BaseInducer, project files and
                   call wrapped inducer's train_and_test.  For wrapped
		   inducers derived from Inducer, call Inducer::train&test.
  Comments     :
*****************************************************************************/
Real ProjectInd::train_and_test(InstanceBag* trainingSet,
				const InstanceBag& testBag) 
{
   ASSERT(trainingSet != NULL && &testBag != NULL);
   has_wrapped_inducer();
   has_project_mask();
   
   Real acc;
   // use base class's function when possible so that updates to it will
   // be used by this:
   if (wrappedInducer->can_cast_to_inducer())
      acc = Inducer::train_and_test(trainingSet, testBag);

   else {
      // project training and test bags
      InstanceBag* shortTraining = trainingSet->project(get_project_mask());
      InstanceBag* shortTest = testBag.project(get_project_mask());
      
      acc = wrappedInducer->train_and_test(shortTraining, *shortTest);
   }
   return acc;
}


/*****************************************************************************
  Description  : Return TRUE iff the class has a valid categorizer.
  Comments     :
*****************************************************************************/
Bool ProjectInd::was_trained(Bool fatalOnFalse) const
{
   if( fatalOnFalse && categorizer == NULL )
      err << "ProjectInd::was_trained: No categorizer, "
	 "Call train() to create categorizer" << fatal_error;

   return ( categorizer != NULL );
}


/*****************************************************************************
  Description  : Returns the categorizer that the inducer has generated.
  Comments     :
*****************************************************************************/
const Categorizer& ProjectInd::get_categorizer() const
{
   was_trained(TRUE);
   return *categorizer;
}


/*****************************************************************************
  Description  : Returns the categorizer that the inducer has generated,
		   releasing ownership at the same time.
  Comments     :
*****************************************************************************/
Categorizer* ProjectInd::release_categorizer()
{
   was_trained(TRUE);
   Categorizer* retCat = categorizer;
   categorizer = NULL;
   return retCat;
}


/*****************************************************************************
  Description  : Cast to Inducer test. Return according to wrapped inducer.
  Comments     : Abort if no wrapped inducer.
*****************************************************************************/
Bool ProjectInd::can_cast_to_inducer() const
{
   has_wrapped_inducer(TRUE); 
   return wrappedInducer->can_cast_to_inducer();
}


/*****************************************************************************
  Description  : Cast to IncrInducer test. Return according to wrapped inducer.
  Comments     : IncrInducer not implemented. See header.
*****************************************************************************/
Bool ProjectInd::can_cast_to_incr_inducer() const
{
//   if (has_wrapped_inducer(FALSE))	// don't abort if no inducer
//      return wrappedInducer->can_cast_to_incr_inducer();
   return FALSE;
}


/*****************************************************************************
  Description  : Cast to IncrInducer. Checks wrapped inducer.  Fatal error if
                   wrapped inducer not incremental.
  Comments     : IncrInducer not implemented. See header.
*****************************************************************************/
// IncrInducer& ProjectInd::cast_to_incr_inducer()
// {
//   ASSERT(can_cast_to_incr_inducer());
//   return *this;
// }


/*****************************************************************************
  Description  : If wrapped inducer is incremental, add instance.  Otherwise
		   fatal error.
  Comments     : IncrInducer not implemented. See header.
*****************************************************************************/
// Pix ProjectInd::add_instance(const InstanceRC& instance)
// {
//   was_trained(TRUE);
//   ASSERT(shortSchema != NULL);
//   
//   if (! wrappedInducer->can_cast_to_incr_inducer()) 
//      err << "ProjectInd::add_instance: Cannot add instance to "
//	 "non-incremental wrapped inducer." << fatal_error;
//
//   Pix returnPixVal = TS->add_instance(instance);
//   wrappedInducer->cast_to_incr_inducer().add_instance(
//      instance.project(*shortSchema, *attrMask));
//
//   return returnPixVal;
// }

	 
/*****************************************************************************
  Description  : Delete an instance from training set of both outer and
                   wrapped inducer.
  Comments     : IncrInducer not implemented. See header.
  Comments     : If the pix does not point to an instance in the TS, the
                   call to TS->remove_instance(pix) will core dump.  If that
		   instance is found, it is an internal error for the wrapped
		   TS not to include the projected version of the instance.
*****************************************************************************/
// InstanceRC ProjectInd::del_instance(Pix& pix)
// {
//   was_trained(TRUE);
//   if (! wrappedInducer->can_cast_to_incr_inducer()) 
//      err << "ProjectInd::del_instance: Cannot remove instance from "
//	 "non-incremental wrapped inducer." << fatal_error;
//
//   InstanceRC instance = TS->remove_instance(pix);
//   InstanceRC shortInstance =
//      instance.project(*shortSchema, *attrMask);
//
//   Bool notFound = TRUE;
//   for (Pix shortPix=wrappedInducer->instance_bag().first();
//	shortPix && notFound;
//	wrappedInducer->instance_bag().next(shortPix) ) 
//      if (wrappedInducer->instance_bag().get_instance(shortPix)
//	  == shortInstance) {
//	 wrappedInducer->cast_to_incr_inducer().del_instance(shortPix); 
//	 notFound = FALSE;
//      }
//   if (notFound)
//      err << "ProjectInd::del_instance: Internal Error: instance to be "
//	 "deleted not found in wrapped inducer's training set." << fatal_error;
//   
//   return instance;
// }

 
/***************************************************************************
  Description : Prints a readable representation of the Cat to the
                  given stream.
  Comments    : 
***************************************************************************/
void ProjectInd::display(MLCOStream& stream,
			 const DisplayPref& dp) const
{
   if (stream.output_type() == XStream)
      err << "ProjectInd::display_struct: Xstream is not a valid "
          << "stream for this display_struct"  << fatal_error;

   if (dp.preference_type() != DisplayPref::ASCIIDisplay)
      err << "ProjectInd::display_struct: Only ASCIIDisplay is "
          << "valid for this display_struct"  << fatal_error;
      

   stream << "ProjectInd Inducer " << description() << endl;
   if ( has_wrapped_inducer(FALSE) ) {
      stream << "    Wrapped Inducer: " << get_wrapped_inducer().description()
	     << endl;
      if ( was_trained(FALSE) )
	 stream << "    Current Categorizer "  << endl;
	 get_categorizer().display_struct(stream, dp);
   }
   else
      stream << "No wrapped inducer set." << endl;
}



DEF_DISPLAY(ProjectInd);





