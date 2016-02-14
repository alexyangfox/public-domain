// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : C45 discretizor. Use C4.5 to discretize attributes.
                 The idea is to show C4.5 a single attribute and ask it
		    to form intervals.  While most discretization methods
                    grow and stop, C4.5 grows and prunes.  It will also
		    determine the number of intervals automatically.
  Assumptions  :
  Comments     : 
  Complexity   : Whatever C4.5 takes.
  Enhancements :
  History      : Ronny Kohavi                                      12/1/95
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <GetOption.h>
#include <C45Disc.h>
#include <ThresholdCat.h>
#include <LogOptions.h>

RCSID("MLC++, $RCSfile: EntropyDisc.c,v $ $Revision: 1.13 $")


// C4.5 doesn't prune the reals enough, so -c1 seems to do much better
MString C45Discretizor::defaultC45Flags("-u -c1 -f %s ");

/***************************************************************************
  Description : Builds the thresholds array. 
  Comments    : Protected, pure virtual
***************************************************************************/
Array<Real>* c45_thresholds(const LogOptions& logOptions,
			    const CatGraph& catGraph)
{
   const CGraph& cgraph = catGraph.get_graph();
   DynamicArray<Real>* thresholds = new DynamicArray<Real>(0);
   ASSERT(thresholds->size() == 0);

   NodePtr nodePtr;
   forall_nodes(nodePtr, cgraph) {
      const Categorizer& cat = catGraph.get_categorizer(nodePtr);
      if (cat.class_id() == CLASS_CONST_CATEGORIZER)
         ASSERT(cgraph.outdeg(nodePtr) == 0); // skip this, it's a leaf
      else if (cat.class_id() == CLASS_THRESHOLD_CATEGORIZER) {
	 // safe cast
	 const ThresholdCategorizer& tc = (ThresholdCategorizer&)cat;
	 int attrNum = tc.attr_num();
	 ASSERT(attrNum == 0); // we're only doing one attribute
         Real threshold = tc.threshold();
	 FLOG(2, "Seeing threshold " << threshold << endl);
	 thresholds->index(thresholds->size()) = threshold;
      }
      else if (cat.class_id() == CLASS_ATTR_CATEGORIZER) 
         err << "c45_thresholds:: unexpected attribute categorizer"
	     << fatal_error;
      else
	 err << "c45_thresholds Unrecognized node type " << cat.class_id()
	     << fatal_error;
   }
   return thresholds;
}


void C45Discretizor::build_thresholds(const InstanceBag& sourceBag)
{
   BoolArray attrMask(0, sourceBag.num_attr(), FALSE);
   attrMask[attrNum] = TRUE;
   InstanceBag* trainSet = sourceBag.project(attrMask);
   InstanceBag testSet(trainSet->get_schema()); // empty
   Pix bagPix = trainSet->first();
   if (bagPix == NULL)
      err << "No instances in sourceBag" << fatal_error;
   
   InstanceRC instance = trainSet->get_instance(bagPix);
   testSet.add_instance(instance); // have just two instances to test on.
   testSet.add_instance(instance); // zero creates division by zero problems
                                   //   in c4.5, one instance causes variance
                                   //   problems in log-level 2
   
   (void)ind.train_and_test(trainSet, testSet);
   MLCIStream prunedTreeStream(ind.file_stem() + ".tree");
   DecisionTree *prunedTree = read_c45_tree(prunedTreeStream,
					    trainSet->get_schema());
   
   Array<Real>* threshs = c45_thresholds(get_log_options(), *prunedTree);
   LOG(1, threshs->size() << " intervals chosen for attribute " <<
       descr << endl);
   threshs->sort();
   create_real_thresholds(*threshs);

   delete trainSet;
   delete prunedTree;
   delete threshs;
   DBG(OK());
}


/***************************************************************************
  Description : C45Discretizor constructor
  Comments    :
***************************************************************************/
C45Discretizor::C45Discretizor(int attrNum, const SchemaRC& schema)
   :RealDiscretizor(attrNum, schema),
    ind("C45Discretizor")
{
   MString c45Flags = get_option_string("DISC_C45_FLAGS", defaultC45Flags);
   ind.set_log_level(get_log_level() - 2);
   ind.set_pgm_flags(c45Flags);
}

C45Discretizor::C45Discretizor(int attrNum, const SchemaRC& schema,
			       const MString& c45Flags)
   :RealDiscretizor(attrNum, schema),
    ind("C45Discretizor", c45Flags)
{
   ind.set_log_level(get_log_level() - 2);
}

/***************************************************************************
  Description : Copy constructor 
  Comments    :
***************************************************************************/
C45Discretizor::C45Discretizor(const C45Discretizor& source,
				       CtorDummy  dummyArg )
   :RealDiscretizor(source, dummyArg),
    ind(source.ind, ctorDummy)
{
}


/***************************************************************************
  Description : operator== 
  Comments    :
***************************************************************************/
Bool C45Discretizor::operator==(const RealDiscretizor& src)
{
   if (class_id() == src.class_id())
      return (*this) == (const C45Discretizor&) src;
   return FALSE;
}

Bool C45Discretizor::operator==(const C45Discretizor& src)
{
   return equal_shallow(src);
}



/***************************************************************************
  Description : Makes a clone of the object.
  Comments    :
***************************************************************************/
RealDiscretizor* C45Discretizor::copy()
{
   RealDiscretizor *nd = new C45Discretizor(*this, ctorDummy);
   DBG(ASSERT(nd != NULL));
   return nd;
}


/***************************************************************************
  Description :  Returns an Array<RealDiscretizor*> of C45Discretizor*
                  using the array of numIntervals in order to generate the
		  discretized Schema and Bag. This routine can be used in
		  conjunction with discretize_bag(). (see RealDiscretizor.c)
  Comments    : 
***************************************************************************/
PtrArray<RealDiscretizor*>*
create_c45_discretizors(LogOptions& logOptions, const InstanceBag& bag)
{
   // The following block should be static MString c45Flags = ...
   //  but cfront chokes on it with
   // "C45Disc.c", line 172: __0__R437: Identifier is undeclared.
   static Bool firstTime = TRUE;
   MString c45Flags;
   if (firstTime) {
      c45Flags = get_option_string("DISC_C45_FLAGS",
				       C45Discretizor::defaultC45Flags);
      firstTime = FALSE;
   }

   SchemaRC schema = bag.get_schema();
   PtrArray<RealDiscretizor*>* c45d
      = new PtrArray<RealDiscretizor*>(schema.num_attr());
   
   for(int k = 0; k < schema.num_attr(); k++){
      const AttrInfo& ai = schema.attr_info(k);
      if ( ai.can_cast_to_nominal() )
	 c45d->index(k) = NULL;
      else if ( ai.can_cast_to_real()){
	 C45Discretizor* c45Disc = new C45Discretizor(k, schema, c45Flags);
	 c45Disc->set_log_options(logOptions);
	 c45Disc->set_log_level(logOptions.get_log_level() - 1);
	 c45d->index(k) = c45Disc;
	 c45d->index(k)->create_thresholds(bag);
      }
      else 
	 err << "Unrecognized attribute type" << fatal_error;
   }
   return c45d;
}

