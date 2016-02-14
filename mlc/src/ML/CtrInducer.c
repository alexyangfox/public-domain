// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The Counter Inducer class provides the same capabilities
                   as Inducer, but also supports counter operations.
  Assumptions  : 
  Comments     : 
  Complexity   : CtrInducer::read_data() has the complexity of 
                   LabelledInstanceList::read_files()
  Enhancements : 
  History      : Ronny Kohavi                                       9/18/93
		   Initial revision (.h)
***************************************************************************/

#include <basics.h>
#include <CtrInducer.h>
#include <CtrInstList.h>

RCSID("MLC++, $RCSfile: CtrInducer.c,v $ $Revision: 1.18 $")


/***************************************************************************
  Description : Allow assign_bag() only for CtrInstanceBag and
                  CtrInstanceList.
  Comments    : Gets ownership of newTS.
                Error checking done by cast_to_ctr_instance_bag().
		Protected method.
***************************************************************************/
InstanceBag* CtrInducer::assign_bag(InstanceBag*& newTS)
{
   if (newTS != NULL) {
      TSWithCounters = &newTS->cast_to_ctr_instance_bag();
      DBG(TSWithCounters->OK());
   }
   else
      TSWithCounters = NULL;

   return Inducer::assign_bag(newTS);
}


/***************************************************************************
  Description : Perform Inducer::OK, check that TSWithCounters matches TS.
  Comments    :
***************************************************************************/

void CtrInducer::OK(int /*level*/) const
{
   // Note that the equivalence below does not compare the bits in the
   //   pointers.  If first has to change TSWithCounters by the delta
   //   of a LabelledInstanceList inside it.
   ASSERT(TSWithCounters == TS);
   Inducer::OK();
}


/***************************************************************************
  Description : Constructor, destructor
  Comments    : 
***************************************************************************/
CtrInducer::CtrInducer(const MString& dscr) : Inducer(dscr)
{
   TSWithCounters = NULL;
}

CtrInducer::CtrInducer(const CtrInducer& source, CtorDummy)
   : Inducer(source, ctorDummy)
{
   TSWithCounters = NULL;
}
   
CtrInducer::~CtrInducer()
{
   DBG(OK());
}

/***************************************************************************
  Description : Read the training set.  See Inducer.c
  Comments    : Here we just force the training set to have the counters.
***************************************************************************/
void CtrInducer::read_data(const MString& file, const MString& namesExtension, 
   			   const MString& dataExtension)
{
   if (!TS) 
      ASSERT(!TSWithCounters);
   delete TSWithCounters;
   TSWithCounters = new CtrInstanceList(file, namesExtension,
					    dataExtension);
   // Note that TS will get a pointer to the base class
   //    inside TSWithCounters.
   TS = TSWithCounters;
   DBG(OK());
}

/***************************************************************************
  Description : Release the bag, giving the caller ownership of it.
  Comments    :
***************************************************************************/

InstanceBag* CtrInducer::release_data()
{
   TSWithCounters = NULL;
   return Inducer::release_data();
}
  


/***************************************************************************
  Description : Return the CtrInstanceList, BagCounter.
  Comments    : 
***************************************************************************/
const CtrInstanceBag& CtrInducer::TS_with_counters() const 
{
   if (TSWithCounters == NULL)
      err << "CtrInducer::TS_with_counters(): Training data has not "
	     "been set" << fatal_error;
   return *TSWithCounters;
}

const BagCounters& CtrInducer::counters() const
{
   return TS_with_counters().counters();
}

