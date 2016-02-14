// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : OneR Inducer.
                 OneR is an inducer that classifies instances based on a single
                   discretized attribute.  It picks the attribute that
		   minimizes the resubstitution error (error on the training
		   set). A more complete discription of 1-Level decision trees
		   can be found in "Very Simple Classification Rules Perform
		   Well on Most Commonly Used DataSets" by R. Holte.
  
  Assumptions  :
  Comments     :
  Complexity   : O(time-to-discretize-the-bag + m(project on single attr +
                   catTestresult(train-set, train-set))) where m is the
                   number of attributes.
  Enhancements :
  History      : James Dougherty                                    12/20/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <OneRInducer.h>
#include <CtrInstList.h>
#include <BoolArray.h>

RCSID("MLC++, $RCSfile: OneRInducer.c,v $ $Revision: 1.5 $")

/***************************************************************************
  Description : Constructor
  Comments    :
***************************************************************************/
OneRInducer::OneRInducer(const MString& description, int small)
 : BaseInducer(description),
   minInstPerLabel(small)
{
   if(minInstPerLabel <=0)
      err << "OneRInducer::OneRInducer: Minimum number of instances per "
	  << "label must be > 0 value chosen is "
	  << minInstPerLabel << fatal_error;
}


/***************************************************************************
  Description : Discretizes the training and test bags. For each attribute
                  in the training bag, it projects on this attribute. It then
		  runs a table inducer on the projected training set and gets
		  the accuracy via CatTestResult using the training set
		  as the test set. The accuracy returned is the accuracy of
		  a table inducer built from the attribute with the minimum
		  resubstitution error, and tested on the test set.
   Comments    :
***************************************************************************/
Real OneRInducer::train_and_test(InstanceBag* orgTrainingSet,
				 const InstanceBag& testbag)
{
   CtrInstanceList trainingSet(orgTrainingSet->get_schema());
  
   // @@ Copy the bag to a ctrbag.  This is a temporary fix
   // @@   until the bags are collapsed.  The TableInducer needs
   // @@   a ctrbag, and since we're a baseinducer, a non-ctr
   // @@   is assumed by u_inducer
   for(Pix p = orgTrainingSet->first(); p; orgTrainingSet->next(p))
      trainingSet.add_instance(orgTrainingSet->get_instance(p));

   LogOptions logOptions(get_log_options());
   logOptions.set_log_level(max(0, get_log_level() - 1));
   PtrArray<RealDiscretizor*>* discretizors
      = create_OneR_discretizors(logOptions, trainingSet,minInstPerLabel);

   InstanceBag* discTrainBag = discretize_bag(trainingSet, discretizors);
   InstanceBag* discTestBag = discretize_bag(testbag, discretizors);
   const SchemaRC schema = discTrainBag->get_schema();

   int bestAttrNum = -1;
   Real bestAccuracy = 0.0;
   TableInducer inducer("OneR");
   inducer.set_log_level(max(0, get_log_level() - 3));
   
   for(int attrNum = 0 ; attrNum < schema.num_attr(); attrNum++){
      BoolArray attrMask(0,schema.num_attr(),FALSE);
      attrMask[attrNum] = TRUE;
      InstanceBag* projBag = discTrainBag->project(attrMask);
      Real acc = inducer.train_and_test(projBag, *projBag);

      LOG(2, "Resubstitution accuracy for attribute " << attrNum
	  << '(' << schema.attr_name(attrNum) << ") is "
	  << MString(acc*100, 4) << '%' << endl);

      if(discretizors->index(attrNum))
	 LOG(2, discretizors->index(attrNum)->num_intervals_chosen()
	     << " intervals chosen for this attribute." << endl);
      
      if (acc > bestAccuracy){ //changing to >= sometimes gives better results
	 bestAccuracy = acc;
	 bestAttrNum = attrNum;
      }
      delete projBag;
   }

   ASSERT(bestAttrNum != -1);
   BoolArray attrMask(0,schema.num_attr(), FALSE);
   attrMask[bestAttrNum] = TRUE;
   InstanceBag* projTrainBag = discTrainBag->project(attrMask);
   InstanceBag* projTestBag = discTestBag->project(attrMask);
   Real acc = inducer.train_and_test(projTrainBag, *projTestBag);
      
   LOG(1, "Best attribute is " << bestAttrNum 
       << " (" << schema.attr_name(bestAttrNum) << ')' 
       << " with accuracy "
       << MString(bestAccuracy*100, 4) << '%' << endl);

   if(discretizors->index(bestAttrNum))
      LOG(1, discretizors->index(bestAttrNum)->num_intervals_chosen()
	  << " intervals chosen for this attribute." << endl);

   delete projTrainBag;
   delete projTestBag;
   delete discTrainBag;
   delete discTestBag;
   delete discretizors;
   return acc;
}















