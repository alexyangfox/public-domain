// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : T2 discretizor. Use T2 to discretize attributes.
                 We use a variant of T2 that just does one level.
		 T2 is a dynamic programming discretizor that provably
		   minimizes the number of errors for the given number
		   of intervals.  See Maas in Colt-94, pages 67-75.
  Assumptions  :
  Comments     : 
  Complexity   : 
  Enhancements :
  History      : Ronny Kohavi                                      12/7/95
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <GetOption.h>
#include <T2Disc.h>
#include <LogOptions.h>
#include <mlcIO.h>

RCSID("MLC++, $RCSfile: T2Disc.c,v $ $Revision: 1.5 $")

const MString T2="T2-int ";
const Real THRESH_EPSILON = STORED_REAL_EPSILON*10;

void T2Discretizor::run_t2(const InstanceBag& bag, int numIntervals)
{
   MString tmpFileStem(get_temp_file_name());
   
   MLCOStream namesStream(tmpFileStem + ".names");
   MLCOStream dataStream(tmpFileStem + ".data");

   MString cmd=T2+"-i "+MString(numIntervals, 0)+ " -f " + tmpFileStem;
   LOG(2, "Running T2 as " << cmd << endl);

   bag.display_names(namesStream, TRUE, "run_t2 generated this file");
   dataStream << bag;

   namesStream.close();
   dataStream.close();

   // run the program
   MString execLine(cmd + " > " + tmpFileStem + ".out");
   if (system(execLine)) 
      err << "T2 program returned bad status.  Line executed was\n  "
            << execLine << fatal_error;

   LOG(2, "Output .int file is" << endl);
   IFLOG(2, system("cat " + tmpFileStem + ".int"));

   MLCIStream threshFile(tmpFileStem + ".int");
   DynamicArray<Real> thresholds(0);
   int line = 1;
   skip_white_comments(threshFile, line);
   while (threshFile.peek() != EOF) {
      threshFile >> thresholds[thresholds.size()];
      // Intervals in T2 are <, >=.  Ours are <=, >
      thresholds[thresholds.size() -1] -= THRESH_EPSILON;
      LOG(3, "Threshold " << thresholds.size() << " is " <<
	   thresholds[thresholds.size() - 1] << endl);
      skip_white_comments(threshFile, line);
   }
   IFLOG(3, if (thresholds.size() == 0)
	 get_log_stream() << "No thresholds" << endl);
   create_real_thresholds(thresholds);
   remove_file(tmpFileStem + ".names");
   remove_file(tmpFileStem + ".data");
   remove_file(tmpFileStem + ".out");
   remove_file(tmpFileStem + ".int");
}
      
      


/***************************************************************************
  Description : Builds the thresholds array. 
  Comments    : Protected, pure virtual
***************************************************************************/
void T2Discretizor::build_thresholds(const InstanceBag& bag)
{
   int intervals = numIntervals;
   // heuristic is the number of categories + 1 (that's the T2 default).
   if (intervals == 0)
      intervals = bag.num_categories() + 1;

   BoolArray attrMask(0, bag.num_attr(), 0);
   attrMask[attrNum] = 1;   
   InstanceBag *projBag = bag.project(attrMask);
   run_t2(*projBag, intervals);
   delete projBag;
}

/***************************************************************************
  Description : T2Discretizor constructor
  Comments    :
***************************************************************************/
T2Discretizor::T2Discretizor(int attrNum, const SchemaRC& schema,
			     int theNumIntervals)
   :RealDiscretizor(attrNum, schema),
    numIntervals(theNumIntervals)
{
}

/***************************************************************************
  Description : Copy constructor 
  Comments    :
***************************************************************************/
T2Discretizor::T2Discretizor(const T2Discretizor& source,
				       CtorDummy  dummyArg )
   :RealDiscretizor(source, dummyArg),
    numIntervals(source.numIntervals)
{
}


/***************************************************************************
  Description : operator== 
  Comments    :
***************************************************************************/
Bool T2Discretizor::operator==(const RealDiscretizor& src)
{
   if (class_id() == src.class_id())
      return (*this) == (const T2Discretizor&) src;
   return FALSE;
}

Bool T2Discretizor::operator==(const T2Discretizor& src)
{
   return equal_shallow(src) && (numIntervals == src.numIntervals);
}



/***************************************************************************
  Description : Makes a clone of the object.
  Comments    :
***************************************************************************/
RealDiscretizor* T2Discretizor::copy()
{
   RealDiscretizor *nd = new T2Discretizor(*this, ctorDummy);
   DBG(ASSERT(nd != NULL));
   return nd;
}


/***************************************************************************
  Description :  Returns an Array<RealDiscretizor*> of T2Discretizor*
                  using the array of numIntervals in order to generate the
		  discretized Schema and Bag. This routine can be used in
		  conjunction with discretize_bag(). (see RealDiscretizor.c)
  Comments    : 
***************************************************************************/
PtrArray<RealDiscretizor*>*
create_t2_discretizors(LogOptions& logOptions,
		       const InstanceBag& bag,
		       const Array<int>& numInterVals)
{
   SchemaRC schema = bag.get_schema();
   PtrArray<RealDiscretizor*>* t2d
      = new PtrArray<RealDiscretizor*>(schema.num_attr());
   
   for(int k = 0; k < schema.num_attr(); k++){
      const AttrInfo& ai = schema.attr_info(k);
      if ( ai.can_cast_to_nominal() )
	 t2d->index(k) = NULL;
      else if ( ai.can_cast_to_real()){
	 T2Discretizor* T2Disc =
	    new T2Discretizor(k, schema, numInterVals.index(k));
	 T2Disc->set_log_options(logOptions);
	 T2Disc->set_log_level(max(0, logOptions.get_log_level() - 1));
	 t2d->index(k) = T2Disc;
	 t2d->index(k)->create_thresholds(bag);
      }
      else 
	 err << "Unrecognized attribute type" << fatal_error;
   }
   return t2d;
}


/***************************************************************************
   Description : Returns an Array<RealDiscretizor*> of T2Discretizor*
                  used to generate the discretized Schema and Bag. Used in
		  conjunction with discretize_bag(). (see RealDiscretizor.c)
  Comments    : Non Array version, use this if the discretization 
                  requires all RealDiscretizor's to have the same number
		  of intervals.
***************************************************************************/
PtrArray<RealDiscretizor*>* create_t2_discretizors(LogOptions& logOptions,
						     const InstanceBag& bag,
						     int numInterVals)
{
   Array<int> intervals(0, bag.get_schema().num_attr(), numInterVals);
   return create_t2_discretizors(logOptions, bag, intervals);
}
