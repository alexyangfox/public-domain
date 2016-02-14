// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : Converts data/test files to continuous files and run
                   sub
  Comments     :
  Enhancements : Support train/test separately.
  History      : Yeogirl Yun                                     7/4/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <CtrInstList.h>
#include <InstList.h>
#include <CFInducer.h>
#include <convDisplay.h>
#include <mlcIO.h>
#include <env_inducer.h>

Normalization ContinFilterInducer::defaultNormMethod = extreme;
Conversion ContinFilterInducer::defaultConvMethod = local;

/*****************************************************************************
  Description : Constructor. Destructor.
  Comments    :
*****************************************************************************/
ContinFilterInducer::ContinFilterInducer(const MString& dscr)
   : BaseInducer(dscr)
{
   continInducer = NULL;
   normMethod = defaultNormMethod;
   convMethod = defaultConvMethod;
}


ContinFilterInducer::~ContinFilterInducer()
{
   delete continInducer;
}


/*****************************************************************************
  Description : set CF sub inducer. Takes ownership.
  Comments    :
*****************************************************************************/
void ContinFilterInducer::set_cf_sub_inducer(BaseInducer*& ind)
{
   if (continInducer != NULL)
      delete continInducer;

   continInducer = ind;
   ind = NULL;
}
      


/*****************************************************************************
  Description : Train and test. Converts data/test/names files according to
                  opitons and read them again and run sub inducer's train
		  and test. 
  Comments    :
*****************************************************************************/
Real ContinFilterInducer::train_and_test(InstanceBag* trainSet,
					 const InstanceBag& testSet)
{
   MString tempStem = get_temp_file_name();
   MString dataFile = tempStem + ".data";
   MString testFile = tempStem + ".test";
   MString namesFile = tempStem + ".names";

   MLCOStream dataFileOut(dataFile);
   MLCOStream testFileOut(testFile);
   MLCOStream namesFileOut(namesFile);   
   
   InstanceList trainBag(trainSet->get_schema());
   for (Pix pix = trainSet->first(); pix; trainSet->next(pix))
      trainBag.add_instance(trainSet->get_instance(pix));
      
   InstanceList testBag(testSet.get_schema());
   for (pix = testSet.first(); pix; testSet.next(pix))
      testBag.add_instance(testSet.get_instance(pix));

   if (get_norm_method() == extreme)
      trainBag.normalize_bag(InstanceBag::extreme);

   Array2<Real>* continStats = NULL;
   if (get_norm_method() == normalDist)
      continStats = create_continuous_stats(trainBag, normalDist);

   // write out the files.
   display_list_rep(testBag,
		    trainBag.get_schema(),
		    testFileOut,
		    get_conv_method(),
		    get_norm_method(),
		    ", ",
		    ", ",
		    ". ",
		    continStats);
		    
   display_list_rep(trainBag,
		    trainBag.get_schema(),
		    dataFileOut,
		    get_conv_method(),
		    get_norm_method(),
		    ", ",
		    ", ",
		    ". ",
		    continStats);

   display_names(trainBag.get_schema(),
		 namesFileOut,
		 get_conv_method(),
		 "");

   // close files.
   dataFileOut.close();
   testFileOut.close();
   namesFileOut.close();

   // read the files again.
   CtrInstanceList newTrainSet(tempStem, ".names", ".data");
   CtrInstanceList newTestSet(tempStem, ".names", ".test");

   // now call sub inducer' train_and_test.
   if (continInducer == NULL)
      err << "ContinFilterInducer::train_and_test: CF_INDUCER is not set"
	 << fatal_error;
   
   Real acc = continInducer->train_and_test(&newTrainSet, newTestSet);

   remove_file(dataFile);
   remove_file(testFile);
   remove_file(namesFile);

   if (continStats)
      delete continStats;

   return acc;
}


/*****************************************************************************
  Description : Set user options. Reuse help strings and MEnum definitions in
                  convDisplay.h.
  Comments    :
*****************************************************************************/
void ContinFilterInducer::set_user_options(const MString& preFix)
{
   BaseInducer *ind = env_inducer(preFix + "CF_");
   set_cf_sub_inducer(ind);

   set_conv_method(get_option_enum(preFix + "CONVERSION",
				   conversionEnum,
				   defaultConvMethod,
				   conversionHelp,
				   TRUE));

   set_norm_method(get_option_enum(preFix + "NORMALIZATION",
				   normalizeEnum,
				   defaultNormMethod,
				   normalizeHelp,
				   TRUE));
}
				   
