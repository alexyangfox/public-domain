// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/***************************************************************************
  Description  : Builds a decision tree of which leafs are NB inducers.
                   It stops building decision tree based on accuracy estimation
                   at each node and its possible children.
  Comments     :
  Enhancements :
  History      : Yeogirl Yun                                       6/17/95
                   Initial Revision.
***************************************************************************/

#include <basics.h>
#include <ID3Inducer.h>
#include <AttrCat.h>
#include <ThresholdCat.h>
#include <BagSet.h>
#include <CtrBag.h>
#include <CatDTInducer.h>
#include <NaiveBayesCat.h>
#include <ConstCat.h>
#include <GetOption.h>
#include <MEnum.h>
#include <NaiveBayesInd.h>
#include <env_inducer.h>



const MString SAMPLING_RATE_HELP = "The rate, given a dataset, to be used as "
  "testset. The rest is used in training.";


Real CatDTInducer::defaultSamplingRate = 0.2;


/*****************************************************************************
  Description : Returns an weighted accuracy on a given array of bag pointers
                   by running on NaiveBayes on each bag weighted by its size.
  Comments    :
*****************************************************************************/
Real CatDTInducer::weighted_accuracy(BagPtrArray* bagArray,
				    int totalSize,
				    Real parentAcc) const
{
   Real totalAcc = 0;
   for (int i = 0; i < bagArray->size(); i++) {
      if (bagArray->index(i)->num_instances() == 1)
         // single instance cannot be cross-validated
	 totalAcc += parentAcc / totalSize;
      else if (!bagArray->index(i)->no_instances()) {
         Real weight =
	    (Real)bagArray->index(i)->num_instances()/(Real)totalSize;
	 Real acc =
	    raw_accuracy(bagArray->index(i));
         LOG(3, " weight on bag # " << i << ": " << weight << endl);
         LOG(3, " accuracy on bag # " << i << ": " << acc << endl);
	 totalAcc += weight * acc;
      }
   }

   LOG(3, " Returning weighted accuracy : " << totalAcc << endl);
   ASSERT(totalAcc <= 1 + REAL_EPSILON);
   return totalAcc;
}
	 
   

/*****************************************************************************
  Description : Returns an accuracy on a given bag by running NaiveBayes.
  Comments    :
*****************************************************************************/
Real CatDTInducer::raw_accuracy(InstanceBag* bag) const
{
   inducer->set_log_level(get_log_level() - 2);

   // some inducer require train set to be CtrInstanceList.
   CtrInstanceList trainList(bag->get_schema());
   for (Pix pix = bag->first(); pix; bag->next(pix))
      trainList.add_instance(bag->get_instance(pix));

   // const cast away because accEst isn't really a part of CatDTInducer,
   // but just exist for a programming convenience.
   return ((CatDTInducer *)this)->accEst.estimate_accuracy(*inducer,
							   trainList);
}
   


/*****************************************************************************
  Description : Build an attriubute categorizer
  Comments    :
*****************************************************************************/
Categorizer* CatDTInducer::build_attr_cat(int attrNum,
					 MStringArray*& catNames) const
{
   //@@ just make use of splitInfo_to_cat by faking SplitInfo.
   SplitInfo si;
   si.mutualInfo = 0.5; // dummy, should be greater than 0.
   si.attrNum = attrNum;
   const SchemaRC& schema = TS->get_schema();
   if (schema.attr_info(attrNum).can_cast_to_nominal())
      si.splitType = SplitInfo::nominalSplit;
   else if (schema.attr_info(attrNum).can_cast_to_real())
      si.splitType = SplitInfo::realThresholdSplit;
   else
      err << "CatDTInducer::build_attr_cat: unrecognized attribute type "
	 "for attribute # : " << attrNum << fatal_error;

   return splitInfo_to_cat(si, catNames);
}



/*****************************************************************************
  Description : Constructor. 
  Comments    :
*****************************************************************************/
CatDTInducer::CatDTInducer(const MString& dscr,
			   CGraph* aCgraph)
   : ID3Inducer(dscr, aCgraph),
     inducer(NULL)
{
   accEst.set_user_options("CatDT_");
   accEst.set_log_level(get_log_level() - 2);
}


CatDTInducer::CatDTInducer(const MString& dscr,
			   Inducer* ind,
			   const AccEstDispatch& accEstDispatch,
			   CGraph* aCgraph)
   : ID3Inducer(dscr, aCgraph)
{
   inducer = ind->copy();
   accEst.copy_options(accEstDispatch);
   accEst.set_log_level(get_log_level() - 2);
}



/*****************************************************************************
  Description : Just check whether inducer is properly initialized and call
                  ID3Inducer::train().
  Comments    :
*****************************************************************************/
void CatDTInducer::train()
{
   if (inducer == NULL)
      err << "CatDTInducer::train:cat inducer is not set. " << fatal_error;

   ID3Inducer::train();
}

/*****************************************************************************
  Description : Almost the same as ID3Inducer::best_split(). In this method,
                  we picks only nominal attrnbutes.
  Comments    :
*****************************************************************************/
Categorizer* CatDTInducer::best_split(MStringArray*& catNames) const
{
   catNames = NULL; // so caller won't try to delete anything if we
   // don't get to the allocation part.

   LOG(2, " The number of instances in TS :" << TS->num_instances() << endl);
   if (counters().label_num_vals() <= 1)
      return NULL; // if we have one label value, we're done.

   LOG(3, counters());
   int bestAttrNum = -1;
   Real bestAcc = -1;
   const SchemaRC& schema = TS->get_schema();      
   int numAttr = schema.num_attr();
   Categorizer *bestCat = NULL;

   Real parentAcc = raw_accuracy(TS);
   LOG(2, "Parent Acc : " << parentAcc << endl);
   LOG(2, " number of attributes : " << numAttr << endl);
   for (int attrNum = 0; attrNum < numAttr; attrNum++) {
      Categorizer* cat = build_attr_cat(attrNum, catNames);
      BagPtrArray* bagArray = TS->split(*cat);
      Real acc = weighted_accuracy(bagArray, TS->num_instances(),
				   parentAcc);
      LOG(3, " Weighted Accuracy on attribute # " << attrNum << " : " <<
	  acc << endl); 
      if (acc > bestAcc) {
	 LOG(3, " Updating the best accuracy from : " << bestAcc << " to : "
	     << acc << endl);
	 delete bestCat;
	 bestAcc = acc;
	 bestCat = cat;
	 bestAttrNum = attrNum;
      }
      else
	 delete cat;
      delete bagArray;
   }

   if (raw_accuracy(TS) >= bestAcc) { // parent has greater accuracy.
      LOG(2, "Returning NULL since parent accuracy is >=" << endl);
      return NULL;
   }
   else {
      ASSERT(bestCat != NULL);
      ASSERT(bestAttrNum != -1);
      ASSERT(bestAcc != -1);
      LOG(2, "Returning bestCat with weighted accuracy : " <<
	  bestAcc << " on attribute # " << bestAttrNum << endl);
      // just rebuild catNames.
      build_attr_cat(bestAttrNum, catNames);      
      bestCat->build_distr(TS_with_counters());
      return bestCat;
   }
}


/*****************************************************************************
  Description : Copy options. Add CatDT options.
  Comments    : Note that accEstDispatch and inducer are not being copied
                  because there's a special ctor for that.
*****************************************************************************/
void CatDTInducer::copy_options(const CatDTInducer& inducer)
{
   ID3Inducer::copy_options(inducer);
   set_sampling_rate(inducer.get_sampling_rate());
}

   
/***************************************************************************
  Description : Create an Inducer for recursive calls.
  Comments    : Since TDDTInducer is an abstract class, it can't
                  do the recursive call.
***************************************************************************/
TDDTInducer* CatDTInducer::create_subinducer(const MString& dscr,
					    CGraph& aCgraph) const
{
   CatDTInducer *ind = new CatDTInducer(dscr, inducer->copy(), accEst,
					&aCgraph);

   ind->copy_options(*this);
   ind->set_level(get_level() + 1);   
   return ind;
}



/***************************************************************************
  Description : Create a leaf categorizer which is NBCategorizer.
  Comments    : 
***************************************************************************/
Categorizer* CatDTInducer::create_leaf_categorizer(int numInstances,
						   Category preferredMajority) const

{
   // numInstances == 0 is passed from TDDTInducer.  It means
   // that the child would have had 0 instances, but we actually have the
   // parent bag as TS_with_counters() in order to compute majority;
   // thus checking label_num_vals() <= 1 is not enough
   if (numInstances == 0 ||
      TS_with_counters().counters().label_num_vals() <= 1) {
      LOG(2, "just creating a const categorizer" << endl);
      return TDDTInducer::create_leaf_categorizer(numInstances, preferredMajority);
   }

   LOG(2, "creating leaf categorizer (num instances : " <<
       numInstances << ")" << endl);

   MString nbName = "NaiveBayesInd";
   set_log_level(get_log_level() - 1);

   inducer->set_log_level(get_log_level() - 1);

   // We give temporary ownership of TS to nb, then release it
   InstanceBag* tempTS = TS;
   delete inducer->assign_data(tempTS);
   inducer->train();
   ASSERT(TS == inducer->release_data());

   Categorizer *categorizer = inducer->get_categorizer().copy();
   if (categorizer)
      categorizer->build_distr(TS_with_counters());         

   return categorizer;
}


/*****************************************************************************
  Description : Sets the samplingRate which should be in range [0,1].
  Comments    :
*****************************************************************************/
void CatDTInducer::set_sampling_rate(Real minVal)
{
   if (minVal < 0 || minVal > 1)
      err << "CatDTInducer::set_SAMPLING_RATE: minVal is out of range [0,1] "
	     "The passed value was : " << minVal << fatal_error;

   samplingRate = minVal;
}



/*****************************************************************************
  Description : Sets the inducer. Gets the ownership.
  Comments    :
*****************************************************************************/
void CatDTInducer::set_cat_inducer(Inducer *& ind)
{
   if (ind == NULL)
      err << "CatDTInducer::set_cat_inducer: trying to set NULL inducer"
	 << fatal_error;
   inducer = ind;
   ind = NULL;
}


/*****************************************************************************
  Description : Prompts the user for the options or gets the options from
                  environment variables.
  Comments    :
*****************************************************************************/
void CatDTInducer::set_user_options(const MString& preFix)
{
   set_sampling_rate(
      get_option_real_range(preFix + "SAMPLING_RATE",
			    defaultSamplingRate,			    
			    0.0, 1.0,
			    SAMPLING_RATE_HELP,
			    FALSE));
   BaseInducer *ind = env_inducer(preFix + "CAT_DT_SUB_");
   if (!ind->can_cast_to_inducer())
      err << "CatDTInducer::set_user_options: " << preFix + "CAT_INDUCER" <<
	 " must be castable to Inducer" << fatal_error;

   Inducer *temp = ind->cast_to_inducer().copy();
   set_cat_inducer(temp);
}




