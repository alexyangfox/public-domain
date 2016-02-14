// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/******************************************************************************
  Description  : TableInducer implements a simple database-like
                   categorizer, namely TableCategorizer. If a novel
		   instance is found in the table of TableCategorizer,
		   the novel instance is categorized as the label of
		   the found instance( or majority instance) in the table.
  Assumptions  : 
  Comments     : 
  Complexity   : 
  Enhancements :
  History      : Yeogirl Yun
                   Added del/add/find_instance methods        10/10/94
                 Yeogirl Yun
                   Initial Revision(.c .h)                     5/01/94
******************************************************************************/

#include <basics.h>
#include <MLCStream.h>
#include <TableInducer.h>
#include <TableCat.h>
#include <CtrBag.h>

/******************************************************************************
  Description  : Constructor with description MString and majority
                    boolean value. 
  Comments     :  
******************************************************************************/
TableInducer::TableInducer(const MString& description,
			   Bool majority)
   : IncrInducer(description)
{
   set_majority_on_unknown(majority);
   categorizer = NULL;
}





/******************************************************************************
  Description  : Deallocates categorizer.
  Comments     :
******************************************************************************/
TableInducer::~TableInducer()
{
   delete categorizer;
}




/******************************************************************************
  Description  : Return TRUE iff the class has a valid categorizer.
  Comments     :
******************************************************************************/
Bool TableInducer::was_trained(Bool fatal_on_false) const
{
   if( fatal_on_false && categorizer == NULL )
      err << "TableInducer::was_trained: No categorizer, "
	     "Call train() to create categorizer" << fatal_error;
   return categorizer != NULL;
}


/******************************************************************************
  Description  : Returns the categorizer that the inducer has generated.
  Comments     :
******************************************************************************/
const Categorizer& TableInducer::get_categorizer() const
{
   was_trained(TRUE);
   return *categorizer;
}


/******************************************************************************
  Description  : Deletes the given instance pointed by pix from the training
                   set and from the TableCat hash table.
		 Returns the deleted instance.
		 Majority category is updated. Since TableInducer uses 
		   CtrInstanceBag, majority_category() will take O(1).
  Comments     : 
******************************************************************************/
InstanceRC TableInducer::del_instance(Pix& pix)
{
   was_trained(TRUE);
   InstanceRC irc = TS->remove_instance(pix);
   categorizer->del_instance(irc);
   if (majorityOnUnknown)
      categorizer->set_default_category(
	 TS_with_counters().majority_category());
  
   return irc;
}


/******************************************************************************
  Description  : Inserts the given instance into the training set and from the
                   TableCat hash table.
		 Returns the pix to the inserted instance.
		 Majority category is updated. Since TableInducer uses 
		   CtrInstanceBag, majority_category() will take O(1).
  Comments     : 
******************************************************************************/
Pix TableInducer::add_instance(const InstanceRC& inst)
{
   was_trained(TRUE);
   Pix returnPixVal = TS->add_instance(inst);
   categorizer->add_instance(inst);
   if (majorityOnUnknown)
      categorizer->set_default_category(
	 TS_with_counters().majority_category());

   return returnPixVal;
}



/******************************************************************************
  Description  : Trains TableCategorizer simply by sending instances
                   in TS to the table in TableCategorizer with default
		   category.
  Comments     : 
******************************************************************************/
void TableInducer::train()
{
   has_data();
   DBG(OK());
   delete categorizer;
   
   if(get_majority_on_unknown() ) 
      categorizer =
	 new TableCategorizer(*TS, TS_with_counters().majority_category(),
			      "Table Categorizer");
   else
      categorizer = new TableCategorizer(*TS, UNKNOWN_CATEGORY_VAL,
					 "Table Categorizer");
}
   


/******************************************************************************
  Description  : Cast to IncrInducer functions.
  Comments     : 
******************************************************************************/
Bool TableInducer::can_cast_to_incr_inducer() const
{
   return TRUE;
}

IncrInducer& TableInducer::cast_to_incr_inducer()
{
   return *this;
}








