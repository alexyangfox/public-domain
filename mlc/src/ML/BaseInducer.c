// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : A base class for inducers.
                 The main reason for this base class, which we don't expect
   		   users to use much, is that ExternalInducers do not build a
		   data structure in memory, and thus we must provide a
		   composite operation train_and_test.
		 The two derived classes that will actually be used by users
  		   are Inducer and ExternalInducer.
  Assumptions  : 
  Comments     : 
  Complexity   : Inducer::read_data() has the complexity of 
                   InstanceList(const MString&, const MString&,
		                        const MString&)
  Enhancements : 
  History      : Yeogirl Yun                                         7/4/95
                   Added copy constructor.                
                 Ronny Kohavi                                       10/5/93
                   Initial revision (.c, .h) based on Inducer.*
***************************************************************************/

#include <basics.h>
#include <BaseInducer.h>
#include <CtrInstList.h>

RCSID("MLC++, $RCSfile: BaseInducer.c,v $ $Revision: 1.9 $")


/***************************************************************************
  Description : Assign the given data to the training set.
  Comments    : The old training set is returned.
                Gets ownership of newTS.
		Protected method.
		See comments for assign_data().
***************************************************************************/
InstanceBag* BaseInducer::assign_bag(InstanceBag*& newTS)
{
   InstanceBag* oldTS = TS;
   TS = newTS;
   newTS = NULL;
   DBG(OK());
   return oldTS;
}


/*****************************************************************************
  Description : Copy constructor. Copies logging option. Set TS to NULL.
  Comments    :
*****************************************************************************/
BaseInducer::BaseInducer(const BaseInducer& source, CtorDummy)
   : dscr(source.dscr)

{
   set_log_options(source.get_log_options());
   TS = NULL;
}

/***************************************************************************
  Description : Deallocates training set.
  Comments    :
***************************************************************************/
BaseInducer::~BaseInducer()
{
  DBG(OK());
  delete TS;
}


/***************************************************************************
  Description : Reads the training set.  The file name suffices if the data
                  file and names file have suffixes ".data" and ".names"
		  respectively.  If the files have a different prefix, give an
		  empty file name, and give the full file name as suffix. 
		Allocates space for training set.  
                Calls InstanceList constructor that reads files.
  Comments    : Should be called before any other methods that
                   requires the training set.
                Overwrites old data if called more than once.
***************************************************************************/
void BaseInducer::read_data(const MString& file, 
			    const MString& namesExtension, 
			    const MString& dataExtension)
{
   delete TS;
   TS = new InstanceList(file, namesExtension, dataExtension);
}


/***************************************************************************
  Description : The following methods just cast the pointer to a
                   InstanceBag* and call assign_bag.  They
		   are necessary to set the given pointer to NULL.
  Comments    : assign_data() methods are non-virtual.  To change
                   their behavior, override assign_bag().  The reason
		   that they are not virtual is that CtrInducer needs
		   to have a counter bag.  It seemed that the simple
		   thing to do would just be to override
		   assign_data(InstanceBag*&).
		   This did not work because redefining only that
		   method hid all of the other methods.  This led to
		   the problem of passing a temporary to a non-const
		   parameter.  Rather than redefine all four methods
		   in CtrInducer, we chose to override one protected
		   method: assign_bag().
***************************************************************************/
#define ASSIGN_BAG(type)                                \
InstanceBag* BaseInducer::assign_data(type*& newTS)     \
{                                                       \
   InstanceBag* bag = newTS;                            \
   newTS = NULL;                                        \
   return assign_bag(bag);                              \
}
ASSIGN_BAG(InstanceBag);
ASSIGN_BAG(InstanceList);
ASSIGN_BAG(CtrInstanceBag);
ASSIGN_BAG(CtrInstanceList);

/***************************************************************************
  Description : Release the bag, giving the caller ownership of it.
  Comments    :
***************************************************************************/

InstanceBag* BaseInducer::release_data()
{
   InstanceBag* bag = TS;
   TS = NULL;

   return bag;
}



/***************************************************************************
  Description : Return TRUE iff the class has a valid training set.
  Comments    :
***************************************************************************/

Bool BaseInducer::has_data(Bool fatalOnFalse) const
{
   if (fatalOnFalse && TS == NULL)
      err << "BaseInducer::has_data: Training data has not been set"
	  << fatal_error;
   return TS != NULL;
}


/***************************************************************************
  Description : Returns an instance bag corresponding to the training set.
  Comments    : 
***************************************************************************/
const InstanceBag& BaseInducer::instance_bag() const
{
   has_data(TRUE);  // checks that TS is allocated
   return *TS;
}

/***************************************************************************
  Description : Convenience function that reads the test file in.
  Comments    :
***************************************************************************/

Real BaseInducer::train_and_test_files(const MString& fileStem,
				       const MString& namesExtension,
				       const MString& dataExtension,
				       const MString& testExtension)
{
   InstanceList trainList("", fileStem + namesExtension,
			    fileStem + dataExtension);
   InstanceList testList ("", fileStem + namesExtension,
			      fileStem + testExtension);
   return train_and_test(&trainList, testList);
}
   
/***************************************************************************
  Description : Cast to inducer functions
  Comments    :
***************************************************************************/

Bool BaseInducer::can_cast_to_inducer() const
{
   return FALSE;
}

Inducer& BaseInducer::cast_to_inducer() 
{
   err << "BaseInducer::cast_to_inducer: Cannot cast" << fatal_error;
   return (Inducer&)(*(Inducer *)NULL_REF);
}


/******************************************************************************
  Description  : Cast to IncrInducer functions.
  Comments     : 
******************************************************************************/
Bool BaseInducer::can_cast_to_incr_inducer() const
{
   return FALSE;
}

IncrInducer& BaseInducer::cast_to_incr_inducer()
{
   err << "BaseInducer::cast_to_inducer: Cannot cast" << fatal_error;
   return (IncrInducer&)(*(IncrInducer *)NULL_REF);
}
   






