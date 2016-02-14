// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Provide operations for supporting counters for BagSets.
                 These are probably the most useful counters, so
                    they are always updated.
                 Counters are provided only for nominal attributes.
                    It is an error to call counters with a non-nominal
                    attribute. 
                 Value counts is a pointer array whose size is the number of
		   attributes.  Each element is a pointer to a 2D array
		   of integers.  The rows correspond to labels,
		   the column correspond to attribute values.
                 Attribute counts is an array where each element
                   points to an array of integers (as in Value counts).
                 The label counts is just an array of ints.
  Assumptions  : Label must be of nominal type.
  Comments     : All non-nominal attributes contain a NULL pointer,
                   instead of a pointer to the array of values.
  Complexity   : Constructing takes time O(lM) where l is the number of
                    attribute values for the label, and M is the number of
                    attributes values in ALL attributes.
                 Destruction takes time O(lm) where m is the number of
                    attributes.
                 add_instance, del_instance take time O(m).
  Enhancements : Should we make the functions here virtual?  Seems like
                   this is so basic, nobody would want to override them.
  History      : Brian Frasca					    4/13/94
		   Changed valueCounts to be an array of pointers to matrices.
		 Richard Long                                      10/21/93
                   Converted to use MLC++ Array template class
                 Ronny Kohavi                                       8/29/93
                   Initial revision (.c and .h)
***************************************************************************/

#include <basics.h>
#include <BagCounters.h>
#include <MLCStream.h>

RCSID("MLC++, $RCSfile: BagCounters.c,v $ $Revision: 1.37 $")

/***************************************************************************
  Description : Sanity check: Make sure the given attribute is of
                  Nominal type.
  Comments    : Private function.  Should only be called under DBG().
***************************************************************************/

DBG_DECLARE(
  void BagCounters::check_nominal(int attrNum) const
  {
     if (!schema.attr_info(attrNum).can_cast_to_nominal())
          err << "BagCounters::check_nominal: Given attribute number ("
              << attrNum << ") is not of nominal type" << fatal_error;
  }
)


/***************************************************************************
  Description : Update counters (either increment or decrement)
                  according to the given labelled instance.
  Comments    : We assume the label is Nominal here (checked in the
                  constructor).
                Update_instance is protected and for use by add_instance
                   and del_instance.
                Update counters is private, increment, decrement are public.
***************************************************************************/

void BagCounters::update_counters(const InstanceRC& instance, int delta)
{
   const SchemaRC schemaRC = instance.get_schema();
   // Check that the LabelledInstanceInfo matches the original one.
   DBGSLOW(schema.equal(instance.get_schema(), TRUE);
           ASSERT(delta == -1 || delta == +1));


   NominalVal labelVal =
      schemaRC.label_info().get_nominal_val(instance.get_label());
   labelCounts[labelVal] += delta;
   int numAttr = schemaRC.num_attr();

   for (int attrNum = 0; attrNum < numAttr; attrNum++) {
      // valueCounts is NULL iff attrCounts is NULL
      DBG(ASSERT((valueCounts[attrNum] == NULL) == 
		 (attrCounts[attrNum] == NULL))); 
      if (valueCounts[attrNum]) { // not NULL denoting non-nominal
         NominalVal attrVal =
	    schemaRC.attr_info(attrNum).get_nominal_val(instance[attrNum]);
         (*valueCounts[attrNum])(labelVal, attrVal) += delta;
         (*attrCounts[attrNum])[attrVal] += delta;
      }
   }
}


void BagCounters::add_instance(const InstanceRC& instance)
{
   update_counters(instance, +1);
}

      
void BagCounters::del_instance(const InstanceRC& instance)
{
   update_counters(instance,-1);
}


/***************************************************************************
  Description : OK() checks that counters add up to the same numbers
                   when added differently.  This checks that the structure
                   is maintained consistently.
                Adding adds up all value counters for a given label +
                  attribute. The sum over each attribute should be the
                  number of instances with that label.
                Adding attribute counts for each attribute should sum
                  up to the number of instances.
  Comments    : The returned value is the sum of all instances (by
                   summing up the label counters).  This may be used
                   for further debugging checks at higher levels, as
                   it is the total number of instances.
***************************************************************************/
int BagCounters::OK(int /*level*/) const
{
   int totalInstances = 0; // counter for all instances
   for (int labCnt = labelCounts.low(); labCnt <= labelCounts.high();
        labCnt++)
      totalInstances += labelCounts[labCnt];

   for (int attrNum = valueCounts.low(); attrNum <= valueCounts.high();
        attrNum++) {
      if (valueCounts[attrNum] != NULL) {
         Array2<int>& vc = *valueCounts[attrNum];
         for (int labelInt = vc.start_row(); labelInt <= vc.high_row();
              labelInt++) {
            int label_instances = label_count(labelInt);
            int sum = 0;
            for (int attrVal = vc.start_col(); attrVal <= vc.high_col();
					       attrVal++) {
	       // Amazingly, the sum was once OK but things were negative!
	       ASSERT(vc(labelInt, attrVal) >= 0);
               sum += vc(labelInt,attrVal);
	    }
            if (sum != label_instances)
               err << "BagCounters::OK: Mismatch in counters for label no "
                   << labelInt << ", attribute no " << attrNum << " ("
                   << sum << "!=" << label_instances << ')' << fatal_error;
         }
      }
   }
   for (int attr = attrCounts.low(); attr <= attrCounts.high(); attr++) {
      if (attrCounts[attr] != NULL) {
         Array<int>& vc = *attrCounts[attr];
         int sum = 0;
         for (int attrVal = vc.low(); attrVal <= vc.high(); attrVal++) {
	    ASSERT(vc[attrVal] >= 0);
            sum += vc[attrVal];
	 }
         if (sum != totalInstances)
            err << "BagCounters::OK: Mismatch in attribute counters for "
               "attribute number " << attr << ". total=" << totalInstances
               << " sum=" << sum <<fatal_error;
      }
   }
   return totalInstances;
}


/***************************************************************************
  Description : The constructor initializes the counters to zero.       
                Note that the labelCounts are automatic and need not
                   be deallocated.                                 
  Comments    : LabelledInstanceInfo is kept for debugging reasons.          
***************************************************************************/
BagCounters::BagCounters(const SchemaRC& aSchema)
   : valueCounts(0, aSchema.num_attr(), FALSE),
     attrCounts(0, aSchema.num_attr(), FALSE),
     labelCounts(UNKNOWN_CATEGORY_VAL, aSchema.num_label_values()+1, 0)
#  ifndef FAST
      // Note that we can't use DBG here because it has a comma and the
      // compiler thinks it's two arguments for the macro.
      , schema(aSchema) // keep for debugging purposes.
#  endif
{
   // For each attribute, we allocate the value array, and step through
   //   all label values and allocate the value array.
   // A NULL is assigned in both cases for non-nominal attributes.
   for (int attrNum = 0; attrNum < aSchema.num_attr(); attrNum++) {
      const AttrInfo& ai = aSchema.attr_info(attrNum);
      if (!ai.can_cast_to_nominal()) {
         attrCounts[attrNum] = NULL;
         valueCounts[attrNum] = NULL;
      } else {
         // Zeroing done by Array<> and Array2<> constructors
         int numAttrVals = ai.cast_to_nominal().num_values();
         attrCounts[attrNum] =
            new Array<int>(UNKNOWN_NOMINAL_VAL, numAttrVals+1, 0);
         valueCounts[attrNum] =
            new Array2<int>(UNKNOWN_CATEGORY_VAL, UNKNOWN_NOMINAL_VAL,
                            aSchema.num_label_values()+1, numAttrVals+1, 0);
      }
   }
}      
                                                   
      
/***************************************************************************
  Description : Copy constructor.
  Comments    :
***************************************************************************/
BagCounters::BagCounters(const BagCounters& source, CtorDummy dummyArg)
   : valueCounts(0, source.valueCounts.size(), FALSE),
     attrCounts(0, source.attrCounts.size(), FALSE),
     labelCounts(source.labelCounts, dummyArg)
#  ifndef FAST
      // Note that we can't use DBG_DECLARE here because it has a comma and the
      // compiler thinks it's two arguments for the macro.
      , schema(source.schema) // keep for debugging purposes.
#  endif
{
   for (int i = valueCounts.low(); i <= valueCounts.high(); i++) {
      if (source.valueCounts[i] == NULL)
	 valueCounts[i] = NULL;
      else 
	 valueCounts[i] = new Array2<int>(*source.valueCounts[i], ctorDummy) ;
   }

   for (i = attrCounts.low(); i <= attrCounts.high(); i++) {
      if (source.attrCounts[i] == NULL)
	 attrCounts[i] = NULL;
      else
	 attrCounts[i] = new Array<int>(*source.attrCounts[i], ctorDummy);
   }
   DBGSLOW(OK());
}



/***************************************************************************
  Description : Destructor
  Comments    : valueCounts and attrCounts elements are deleted by PtrArray
                   destructor.
***************************************************************************/
BagCounters::~BagCounters()
{
   DBGSLOW(OK());
}


/*****************************************************************************
   Description : Returns TRUE iff the two BagCounter's are equal.
   Comments    :
*****************************************************************************/
Bool BagCounters::operator==(const BagCounters& bagCtr) const 
{
   for (int i = valueCounts.low(); i <= valueCounts.high(); i++) {   
      // must cast to int or xor gives warning
      if (int(bagCtr.valueCounts[i] == NULL) ^ int(valueCounts[i] == NULL))
	 return FALSE;
      else if (bagCtr.valueCounts[i] != NULL && valueCounts[i] != NULL)
	 if (*bagCtr.valueCounts[i] != *valueCounts[i])
	       return FALSE;
   }

   if (labelCounts != bagCtr.labelCounts)
      return FALSE;

   for (i = attrCounts.low(); i <= attrCounts.high(); i++)
      if (int(bagCtr.attrCounts[i] == NULL) ^ int(attrCounts[i] == NULL))
	 return FALSE;
      else if (bagCtr.attrCounts[i] != NULL && attrCounts[i] != NULL)
          if (*attrCounts[i] != *bagCtr.attrCounts[i])
	     return FALSE;

   return TRUE;
}



/***************************************************************************
  Description : Return the value of a value counter, attribute counter,
                  or label counter.
  Comments    : 
***************************************************************************/
int BagCounters::val_count(NominalVal labelVal,
			   int attrNum, NominalVal attrVal) const
{
   DBG(check_nominal(attrNum));
   ASSERT(valueCounts[attrNum] != NULL);

   return (*valueCounts[attrNum])(labelVal,attrVal);
}


int BagCounters::attr_count(int attrNum,
                            NominalVal attrVal) const
{
   DBG(check_nominal(attrNum));
   return (*attrCounts[attrNum])[attrVal];
}


int BagCounters::label_count(NominalVal labelVal) const
{
   return labelCounts[labelVal];
}
   

/***************************************************************************
  Description : Count actual number of different values for attribute,
                   or label.
  Comments    :
***************************************************************************/
int BagCounters::attr_num_vals(int attrNum) const
{
   DBG(check_nominal(attrNum));
   Array<int>& ac = *attrCounts[attrNum];

   int num_vals = 0;
   for (int i = ac.low(); i <= ac.high(); i++)
      if (ac[i] != 0)
         num_vals++;

   return num_vals;
}


int BagCounters::label_num_vals() const
{
   int num_vals = 0;
   for (int i = labelCounts.low(); i <= labelCounts.high(); i++)
      if (labelCounts[i] != 0)
         num_vals++;

   return num_vals;
}

/***************************************************************************
  Description : Display the counter bag
  Comments    :
***************************************************************************/
void BagCounters::display(MLCOStream& stream) const
{
   stream << "Value counters:" << endl;
   for (int attrNum = valueCounts.low(); attrNum <= valueCounts.high();
        attrNum++) {
      stream << "Attribute " << attrNum << ":";
      if (valueCounts[attrNum] == NULL)
         stream << "  is not nominal." << endl;
      else {
         Array2<int>& vc = *valueCounts[attrNum];

         for (int labelInt = vc.start_row(); labelInt <= vc.high_row();
              labelInt++) {
            stream << "\n  Label " << labelInt
                   << ", label count=" << label_count(labelInt)
                   << "\n  Value counts:  ";
            for (int attrVal = vc.start_col(); attrVal <= vc.high_col();
                 attrVal++)
               stream << vc(labelInt,attrVal) << ", ";
            stream << endl;
         }
      }
   }

   stream << "Attribute counts" << endl;
   for (int attr = attrCounts.low(); attr <= attrCounts.high(); attr++) {
      stream << "Attribute " << attr << "  ";
      if (attrCounts[attr] == NULL)
         stream << "NULL" << endl;
      else {
         Array<int>& vc = *attrCounts[attr];
         for (int attrVal = vc.low(); attrVal <= vc.high(); attrVal++)
            stream << vc[attrVal] << ", ";
         stream << endl;
      }
   }
}

DEF_DISPLAY(BagCounters);


/***************************************************************************
  Description : The following functions allow direct access to the counters.
  Comments    :
***************************************************************************/
const PtrArray<Array2<int>*>& BagCounters::value_counts() const
{
   return valueCounts;
}

const Array<int>& BagCounters::label_counts() const
{
   return labelCounts;
}

const PtrArray<Array<int>*>& BagCounters::attr_counts() const
{
   return attrCounts;
}
