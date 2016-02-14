// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : External Inducer interfacing OC1 inducer.
  Assumptions  : shell must find the OC1 program in the path.
  Comments     : Every train_and_test() call will write two files (one
                   configuration file, one train/test file) to the current
		   directory and run OC1 program and delete
		   them after getting accuracy from its output file.
  Complexity   :
  Enhancements :
  History      : Yeogirl Yun                                   6/12/95
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <OC1Inducer.h>
#include <InstList.h>
#include <mlcIO.h>  // for get_temp_file_name
#include <CatTestResult.h>
#include <convDisplay.h>
#include <LogOptions.h>
#include <GetOption.h>
#include <MEnum.h>
#include <OC1Inducer.h>
#include <ConstInducer.h>
#include <string.h>
#include <ctype.h>

const MString PGM_NAME_HELP = "the name of OC1 program";
const MString RANDOM_SEED_HELP = "random seed to initialize OC1 program";
const MString AXIS_PARALLEL_ONLY_HELP = "Set true, OC1 consider only "
   "axis-parallel tests at nodes";
const MString CART_LINEAR_COMBINATION_MODE_HELP = "Set true, OC1 "
   "executes CART's deterministic perturbation algorithm";
const MString PRUNING_HELP = "The proportion of training file used to "
   "prune the tree that is built from the remaining part of training file";
const MString REDIRECT_STRING_HELP = "You can take either '2>&1 >' or '>&'. "
   "if you use sh, then use '2>&1 >' otherwise '>&'. These are used to "
   "redirect all the output of OC1 program to get an accuracy.";

MString OC1Inducer::defaultPgmName("mktree");
MString OC1Inducer::defaultRedirectString("2>&1 >");
Bool OC1Inducer::defaultAxisParallelOnly = FALSE;
Bool OC1Inducer::defaultCartLinearCombinationMode = FALSE;
Real OC1Inducer::defaultPruningRate = 0.1;
unsigned int OC1Inducer::defaultRandomSeed = 7258789;

/*****************************************************************************
  Description : Parse the output of OC1 program.
                Currently, it finds accuracy line from OC1 program output.
  Comments    :
*****************************************************************************/

static Real parse_output(const MString& fileName)
{
   MLCIStream file(fileName);

   const MString token = "accuracy = ";
   
   MString line;
   MString lastAccuracy;
   while (!file.eof()) {
      line.get_line(file, '\n');
      GLOBLOG(2, line << endl);
      if (line.contains(token)) {
	 lastAccuracy = line;
	 break;
      }
   }

   GLOBLOG(2, "The last accuracy line : " << lastAccuracy << endl);
   if (lastAccuracy == "" )
      err << "Internal Error - Can't parse OC1 program output\n"
	 "See file " << fileName << fatal_error;

   MString subString = lastAccuracy.substring(10, 6);
   Real acc  = subString.real_value();
   ASSERT(acc >= 0 && acc <= 100);

   GLOBLOG(2, " The accuracy is " << acc << "%" << endl);   
   return acc / 100; // percent
}


/***************************************************************************
  Description : run OC1 and return accuracy given train and test sets.
  Comments    :
***************************************************************************/
static Real runOC1(const MString& pgmName,
		   unsigned int randomSeed,
		   Bool axisParallelOnly,
		   Bool cartLinearCombinationMode,
		   Real pruningRate,
		   const MString& redirectString,
		   const InstanceList& trainList,
		   const InstanceList& testList)
{
   MString fileStem = get_temp_file_name();
   MString datafile = fileStem + ".train";
   MString testfile = fileStem + ".test";   
   MString outfile = fileStem + ".out";

   MLCOStream outDatafile(datafile);
   MLCOStream outTestfile(testfile);   
   MLCOStream outOutfile(outfile);

   // now create OC1 files.
   // first create data file containig data and test instances.
   display_list_OC1(trainList, outDatafile);
   display_list_OC1(testList, outTestfile);

   // now create pcf file
   // write out options.
   const SchemaRC& schema = trainList.get_schema();
   MString flags = "-c" + MString((int)testList.get_schema().
				  num_label_values(), 0) + " ";
   if (axisParallelOnly)
      flags += "-a ";
   if (cartLinearCombinationMode)
      flags += "-K " ;
   flags += "-p" + MString(pruningRate,2)+ " ";
   flags += "-s" + MString((long)randomSeed,0) + " ";
   flags += "-t" + fileStem + ".train "; 
   flags += "-T" + fileStem + ".test ";

   GLOBLOG(2, " OC1 flags : " << flags << endl);
   outDatafile.close();
   outTestfile.close();   
   outOutfile.close();

   // now execute OC1.
   MString command = pgmName + " " + flags + " " + redirectString + " "
      + outfile;
      
   // run it.
   if (system(command) != 0) 
      err <<  "OC1Inducer::train_and_test: fail to run OC1 program as "
	  << command << fatal_error;

   Real acc =parse_output(outfile);
   // clean up.
   remove_file(datafile);
   remove_file(outfile);

   return acc;
}



/*****************************************************************************
  Description : Constructor/Destructor.
                Intialize optoins.
  Comments    :
*****************************************************************************/
OC1Inducer::OC1Inducer(const MString& description, 
		       const MString& thePgmName)
   : BaseInducer(description),
     pgmName(thePgmName)
{
   set_pgm_name(defaultPgmName + " ");
   set_seed(defaultRandomSeed);
   set_axis_parallel_only_opt(defaultAxisParallelOnly);
   set_cart_linear_combination_mode(defaultCartLinearCombinationMode);
   set_pruning_rate(defaultPruningRate);
   set_redirect_string(defaultRedirectString);
}


OC1Inducer::~OC1Inducer()
{
}


/*****************************************************************************
  Description : set the pruning rate.
  Comments    :
*****************************************************************************/
void OC1Inducer::set_pruning_rate(Real val)
{
   if (val < 0 || val > 1) 
      err << "OC1Inducer::set_pruning_rate : pruning rate must be in "
	 "[0,1]. The set value was : " << val << fatal_error;

   pruningRate = val;
}


/*****************************************************************************
  Description : Set the redirect string.
  Comments    :
*****************************************************************************/
void OC1Inducer::set_redirect_string(const MString& val)
{
   if (!val.contains("2>&1 >") && !val.contains(">&"))
      err << "OC1Inducer::set_redirect_string : unknown redirect string "
	 "set : " << val << " Use either '2>&1 >' or '>&' " << fatal_error;
   redirectString = val;
}
   
/***************************************************************************
  Description : Train and test by running OC1's OC1 program.
  Comments    :
***************************************************************************/
Real OC1Inducer::train_and_test(InstanceBag* trainingSet,
 			        const InstanceBag& testBag)
{
   // we don't want to take ownership of the given InstanceBag, so
   // we add instances instead of using InstanceList constructor.
   InstanceList trainList(trainingSet->get_schema());
   for (Pix pix = trainingSet->first(); pix; trainingSet->next(pix))
      trainList.add_instance(trainingSet->get_instance(pix));

   InstanceList testList(testBag.get_schema());
   for (pix = testBag.first(); pix; testBag.next(pix))
      testList.add_instance(testBag.get_instance(pix));

   if (trainList.get_schema().num_attr() == 0) {
      // since OC1 do not handle 0-attribute instances, we run const
      // inducer here for completeness. Esp. this is necesary for FSS.
      LOG(1, " 0-attribute instances detected. Running const inducer."
	  << endl);
      ConstInducer constInd("const inducer");
      return constInd.train_and_test(&trainList, testList);
   }      

   LOG(2,
       " program name : " << get_pgm_name() << endl <<
       " seed : " << get_seed() << endl);

   if (get_axis_parallel_only_opt())
      LOG(2, " axis parallel : true" << endl);
   else
      LOG(2, " axis parallel : false" << endl);

   if (get_cart_linear_combination_mode())
      LOG(2, " cart linear : true" << endl);
   else
      LOG(2, " cart linear : false" << endl);
   LOG(2, " pruning rate : " << get_pruning_rate() << endl);

   return runOC1(get_pgm_name(),
		 get_seed(),
		 get_axis_parallel_only_opt(),
		 get_cart_linear_combination_mode(),
		 get_pruning_rate(),
		 get_redirect_string(),
		 trainList,
		 testList);
}


/*****************************************************************************
  Description : Set user options.
  Comments    :
*****************************************************************************/
void OC1Inducer::set_user_options(const MString& preFix)
{
   set_pgm_name(get_option_string(preFix + "OC1_PGM_NAME",
				  defaultPgmName,
				  PGM_NAME_HELP,
				  TRUE));
   set_seed(get_option_int(preFix + "OC1_SEED",
			   defaultRandomSeed,
			   RANDOM_SEED_HELP,
			   FALSE));
   set_axis_parallel_only_opt(
      get_option_bool(preFix + "OC1_AXIS_PARALLEL_ONLY",
		      defaultAxisParallelOnly,
		      AXIS_PARALLEL_ONLY_HELP,
		      FALSE));
   set_cart_linear_combination_mode(
      get_option_bool(preFix + "OC1_CART_LINEAR_COMBINATION_MODE",
		      defaultCartLinearCombinationMode,
		      CART_LINEAR_COMBINATION_MODE_HELP,
		      FALSE));
   set_pruning_rate(
      get_option_real_range(preFix + "OC1_PRUNING_RATE",
			    defaultPruningRate,
			    0, 1, PRUNING_HELP,
			    FALSE));
   set_redirect_string(
      get_option_string(preFix + "OC1_REDIRECT_STRING",
			defaultRedirectString,
			REDIRECT_STRING_HELP,
			FALSE));
}
