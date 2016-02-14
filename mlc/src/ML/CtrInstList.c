// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Inherits the methods of InstanceLists and
                   CtrInstanceBags
  Assumptions  : 
  Comments     : 
  Complexity   : The complexity of remove_front() is the same as for
                    CtrInstanceBag::and remove_instance()
  Enhancements : 
  History      : Richard Long                                       9/13/93
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <CtrInstList.h>

RCSID("MLC++, $RCSfile: CtrInstList.c,v $ $Revision: 1.20 $")


/***************************************************************************
  Description : Reads the Schema from "names" file and instances 
                  from "data" file.
		See read_names() and read_data() for file format and
		  other details.
  Comments    : This method cannot simply call the corresponding
		  CtrInstanceBag constructor because virtual methods
		  do not work in constructors and the wrong versions
		  of read_names() and read_data() would be called.
***************************************************************************/
CtrInstanceList::CtrInstanceList(const MString& file,
				 const MString& namesExtension,
				 const MString& dataExtension)
          : CtrInstanceBag(), InstanceList()
{
   read_names(file + namesExtension);
   read_data(file + dataExtension);
}

/***************************************************************************
  Description : Constructors.  Destructor.
  Comments    :
***************************************************************************/
// Get ownership of info; will own instances
CtrInstanceList::CtrInstanceList(const SchemaRC& schemaRC)
   : InstanceBag(), InstanceList(), CtrInstanceBag()
{
   set_schema(schemaRC);
   new_counters();
}


CtrInstanceList::CtrInstanceList(const CtrInstanceList& source, 
				 CtorDummy /* dummyArg */)
     : InstanceBag(source, ctorDummy),
       InstanceList(),
       CtrInstanceBag(source, ctorDummy) 
{
}   

/***************************************************************************
  Description : Takes ownership of bag.
                The List has the same contents as the Bag, but there is
		   now a fixed order to the instances.
  Comments    :
***************************************************************************/
CtrInstanceList::CtrInstanceList(CtrInstanceBag*& bag)
                 : InstanceList(bag->get_schema()),
		   CtrInstanceBag(bag->get_schema())
{
   copy(bag);
   ASSERT(bag == NULL);
}


CtrInstanceList::~CtrInstanceList()
{}


/***************************************************************************
  Description : Override the default behavior of InstanceList
                  to update the counters.
  Comments    :
***************************************************************************/
void CtrInstanceList::read_names(const MString& file)
{
   InstanceList::read_names(file);
   new_counters();
}


InstanceRC CtrInstanceList::remove_front()
{
   if (no_instances())
      err << "CtrInstanceList::remove_front: list is empty"
	  << fatal_error;
   Pix pix = first();
   return CtrInstanceBag::remove_instance(pix);   
}


/***************************************************************************
  Description : Returns a reference to a CtrInstanceList.
  Comments    :
***************************************************************************/
CtrInstanceList& CtrInstanceList::cast_to_ctr_instance_list()
{
   return *this;
}

const CtrInstanceList&
CtrInstanceList::cast_to_ctr_instance_list() const
{
   return *this;
}


/***************************************************************************
  Description : Returns a pointer to a new CtrInstanceList with the
                   given schema.
  Comments    : 
***************************************************************************/
InstanceBag* CtrInstanceList::create_my_type(const SchemaRC& schemaRC) const
{
   return new CtrInstanceList(schemaRC);
}


/***************************************************************************
  Description : Returns a pointer to a counter list that has the same contents
                   as this list, with a random ordering of the instances.
  Comments    : mrandom and index default to NULL.
                The MRandom parameter allows for duplication of results.
***************************************************************************/
CtrInstanceList* CtrInstanceList::shuffle(MRandom* mrandom,
					  InstanceBagIndex* index) const
{
   return &shuffle_(mrandom, index)->cast_to_ctr_instance_list();
}
