// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Binning Real Discretizor class.
                 The Binning Real Discretizor takes a real attribute and
		   discretizes it into uniform size bins according to the min
		   and max values in the instancebag.
		 Use the function create_binning_discretizors() to generate
		   an array of binning discretizors for discretization of
		   a bag via the function discretize_bag() (see
		   RealDiscretizor.{h.c})
  Assumptions  :
  Comments     :
  Complexity   : BinningRealDiscretizor::create_thresholds() takes
                  O(numBins + normalize_attr + num_distinct values)
  Enhancements :
  History      : James Dougherty                                      12/11/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <BinningDisc.h>

RCSID("MLC++, $RCSfile: BinningDisc.c,v $ $Revision: 1.15 $")

const Real binProportion = 2.0;


/***************************************************************************
  Description : Builds the thresholds array.
  Comments    : Should be the first method called in this class, other than
                  OK. Protected, pure virtual
***************************************************************************/
void BinningRealDiscretizor::build_thresholds(const InstanceBag& bag)
{
   //logically const
   ((class InstanceBag&)bag).normalize_attr(attrNum, InstanceBag::extreme);
   rai->set_max(bag.attr_info(attrNum).cast_to_real().get_max());
   rai->set_min(bag.attr_info(attrNum).cast_to_real().get_min());
   Real maxVal = rai->get_max();
   Real minVal = rai->get_min();

   if (0 == numBins){ // heuristic
      int ndv = max(1,num_distinct_vals(bag, attrNum));
      numBins = int(max(1.0, binProportion * log_bin(ndv) + 0.5));
   }
   
   Real binSize =  (maxVal - minVal) / numBins;
   // If maxVal == minVal, there is only one bin (-INF, +INF)
   if(minVal == maxVal){
      LOG(1, "Attribute \"" << descr << "\" has only one value, " 
	    << "using only one bin." << endl);
      numBins = 1;
   }
   
   int numThresholds = numBins - 1;
   thresholds = new DynamicArray<RealAttrValue_>(numThresholds);
   for(int i = 0; i < numThresholds; i++)
      rai->set_real_val(thresholds->index(i), minVal + (i + 1) * binSize);
   LOG(1, num_intervals_chosen() << " intervals chosen for attribute "
          << descr << endl);
  DBG(OK());
}


/***************************************************************************
  Description : Constructor for a binning real discretizor.
  Comments    : 
***************************************************************************/
BinningRealDiscretizor::BinningRealDiscretizor(int attrNum, int nBins,
					       const SchemaRC& schema)
   : RealDiscretizor(attrNum, schema),
     numBins(nBins)
{
   if (numBins < 0)
      err << "BinningRealDiscretizor::BinningRealDiscretizor: "
	  << "number of bins (" << numBins
	  << ") is negative. " << fatal_error;
}


/***************************************************************************
  Description : Copy constructor
  Comments    :
***************************************************************************/
BinningRealDiscretizor::BinningRealDiscretizor(const BinningRealDiscretizor&
					       source, CtorDummy dummyArg)
   :RealDiscretizor(source, dummyArg)
{
   numBins = source.numBins;
}
					       

/***************************************************************************
  Description : operator ==
  Comments    :
***************************************************************************/

Bool BinningRealDiscretizor::operator ==(const RealDiscretizor& source)
{
   if (class_id() == source.class_id())
      return (*this) == (class BinningRealDiscretizor&) source;
   return FALSE;
}

Bool BinningRealDiscretizor::operator ==(const BinningRealDiscretizor& source)
{
   return equal_shallow(source) && numBins == source.numBins;
}


/***************************************************************************
  Description : Returns a deep copy of the object
  Comments    :
***************************************************************************/
RealDiscretizor* BinningRealDiscretizor::copy()
{
   return new BinningRealDiscretizor(*this, ctorDummy);
}


/***************************************************************************
  Description : Returns an Array<RealDiscretizor*> of BinningRealDiscretizor*
                  with binCounts[i] bins for each. 
		  Used to generate the discretized Schema and Bag and in
		  conjunction with discretize_bag() (see RealDiscretizor.c)
  Comments    : 
***************************************************************************/
PtrArray<RealDiscretizor*>*
create_binning_discretizors(LogOptions& logOptions,
			    const InstanceBag& bag,
			    const Array<int>& binCounts)
{
   SchemaRC schema = bag.get_schema();
   PtrArray<RealDiscretizor*>* brd
      = new PtrArray<RealDiscretizor*>(schema.num_attr());

   for(int k = 0; k < schema.num_attr(); k++){
      const AttrInfo& ai = schema.attr_info(k);
      if ( ai.can_cast_to_nominal() )
	 brd->index(k) = NULL;
      else if ( ai.can_cast_to_real()){
	 BinningRealDiscretizor* realDisc =
	    new BinningRealDiscretizor(k, binCounts.index(k), schema);
	 realDisc->set_log_options(logOptions);
	 realDisc->set_log_level(max(0, logOptions.get_log_level() - 1));
	 brd->index(k) = realDisc;
	 brd->index(k)->create_thresholds(bag);
      }
      else 
	 err << "Unrecognized attribute type" << fatal_error;
   }
   return brd;
}


/***************************************************************************
  Description : Returns an Array<RealDiscretizor*> of BinningRealDiscretizor*
                  with the same number of bins for each.
		  Used to generate the discretized Schema and Bag and in
		  conjunction with discretize_bag() (see RealDiscretizor.c)
  Comments    : 
***************************************************************************/
PtrArray<RealDiscretizor*>*
create_binning_discretizors(LogOptions& logOptions,
			    const InstanceBag& bag, int binCount)
{
   Array<int> binCounts(0, bag.get_schema().num_attr(), binCount);
   return create_binning_discretizors(logOptions, bag, binCounts);
}
