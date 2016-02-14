// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : External Inducer interfacing Pebls inducer.
  Assumptions  : shell must find the Pebls program in the path.
  Comments     : Every train_and_test() call will write two files (one
                   configuration file, one train/test file) to the current
		   directory and run Pebls program and delete
		   them after getting accuracy from its output file.
  Complexity   :
  Enhancements :
  History      : Yeogirl Yun                                   6/12/95
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <PeblsInducer.h>
#include <InstList.h>
#include <mlcIO.h>  // for get_temp_file_name
#include <CatTestResult.h>
#include <convDisplay.h>
#include <LogOptions.h>
#include <GetOption.h>
#include <MEnum.h>
#include <PeblsInducer.h>
#include <ConstInducer.h>


const MString PGM_NAME_HELP = "the name of Pebls program";
const MString NEAREST_NEIGHBORS_HELP = "the number of nearest neighbors";
const MString VOTING_SCHEME_HELP = "this parameter defines how the "
  "located K nearest neighbors are used to define the class of the test "
  "instance. In MAJORITY mode, each neighbor gets one vote. The test "
  "instance is assigned the same class as the most frequently occuring "
  "class among the K nearest neighbors. In WEIGHTED_DISTANCE mode, "
  "each neighbor contributes a vote towards its particular class "
  "inversely proportional to its distance.";
const MString DISCRETIZATION_LEVELS_HELP = "the number of levels between "
  "two real values in continue feature values";

MString PeblsInducer::defaultPgmName("pebls");
int PeblsInducer::defaultDiscretizationLevels = 10;
int PeblsInducer::defaultNearestNeighbors = 1;
PeblsVotingScheme PeblsInducer::defaultVotingScheme = MAJORITY;


const MEnum votingSchemeEnum =
  MEnum("majority", MAJORITY) <<
  MEnum("weighted-distance", WEIGHTED_DISTANCE);


/*****************************************************************************
  Description : Parse the output of Pebls program.
                Currently, it finds the Percent Correct at the average only
		  output.
  Comments    :
*****************************************************************************/

static Real parse_output(const MString& fileName)
{
   MLCIStream file(fileName);
   
   MString line;
   MString lastAccuracy;
   int position = -1;
   while (!file.eof()) {
      line.get_line(file, '\n');
      GLOBLOG(2, line << endl);
      if (line.contains("PERCENT"))
	 position = line.index("PERCENT");
      else if (line.contains("TOTAL")) 
	 lastAccuracy = line;
   }

   
   GLOBLOG(2, "The last accuracy line : " << lastAccuracy << endl);
   if (lastAccuracy == "" || position == -1 ||
       position >= lastAccuracy.length())
      err << "Internal Error - Can't parse Pebls program output\n"
	 "See file " << fileName << fatal_error;

   Real acc  = lastAccuracy.substring(position,
				      sizeof("PERCENT") - 2).real_value();
   ASSERT(acc >= 0 && acc <= 100);

   GLOBLOG(2, " The accuracy is " << acc << "%" << endl);   
   return acc / 100; // percent
}


/***************************************************************************
  Description : run Pebls and return accuracy given train and test sets.
  Comments    :
***************************************************************************/
static Real runPEBLS(const MString& pgmName,
		     int discretizationLevels,
		     int nearestNeighbors,
		     PeblsVotingScheme votingScheme,
		     const InstanceList& trainList,
		     const InstanceList& testList)
{
   MString fileStem = get_temp_file_name();
   MString datafile = fileStem + ".data";
   MString pcffile = fileStem + ".pcf";   
   MString outfile = fileStem + ".out";

   MLCOStream outDatafile(datafile);
   MLCOStream outPcffile(pcffile);   
   MLCOStream outOutfile(outfile);

   // now create Pebls files.
   // first create data file containig data and test instances.
   display_list_pebls(trainList, outDatafile, "TRAIN");
   display_list_pebls(testList, outDatafile, "TEST");

   // now create pcf file
   // write out options.
   const SchemaRC& schema = trainList.get_schema();
   outPcffile << "data_file=" << fileStem << ".data" << endl;
   outPcffile << "data_format=STANDARD" << endl;
   int numLabels = schema.num_label_values();
   outPcffile << "classes=" << numLabels << endl;
   outPcffile << "class_names=";
   for (int i = FIRST_CATEGORY_VAL; i < numLabels + FIRST_CATEGORY_VAL; i++)
      outPcffile << schema.category_to_label_string(i) << " ";
   outPcffile << endl;
   int numAttrs = schema.num_attr();
   outPcffile << "features=" << numAttrs << endl;
   for (i = 1; i <= numAttrs; i++) {
      outPcffile << "feature_values " << i << " = ";
      if (schema.attr_info(i-1).can_cast_to_nominal()) { // nominal attribute
	 for (int j = 0; j < schema.num_attr_values(i-1); j++)
	    outPcffile << schema.attr_info(i-1).cast_to_nominal().
	       get_value(FIRST_NOMINAL_VAL + j) << " ";
	 outPcffile << endl;
      } 
      else 
	 outPcffile << "FLOAT" << " "
		    << schema.attr_info(i-1).cast_to_real().get_min() << " "
		    << schema.attr_info(i-1).cast_to_real().get_max() << " "
		    << discretizationLevels << endl;
   }
   outPcffile << "training_mode=SPECIFIED_GROUP" << endl;
   outPcffile << "nearest_neighbor=" << nearestNeighbors << endl;
   outPcffile << "nearest_voting=";
   if (votingScheme == MAJORITY)
      outPcffile << "MAJORITY" << endl;
   else if (votingScheme == WEIGHTED_DISTANCE)
      outPcffile << "WEIGHTED_DISTANCE" << endl;
   else
      err << "PeblsInducer.c::runPEBLS: unknown voting scheme value : " <<
	 votingScheme << fatal_error;
			 
   outDatafile.close();
   outPcffile.close();   
   outOutfile.close();

   // now execute PEBLS.
   MString command = pgmName + " " + pcffile + " > " + outfile;
      
   // run it.
   if (system(command) != 0) 
      err <<  "PeblsInducer::train_and_test: fail to run Pebls program as "
	   << command << fatal_error;

   Real acc = parse_output(outfile);
   // clean up.
   remove_file(datafile);
   remove_file(outfile);
   remove_file(pcffile);   

   return acc;
}



/*****************************************************************************
  Description : Constructor/Destructor.
                Intialize optoins.
  Comments    :
*****************************************************************************/
PeblsInducer::PeblsInducer(const MString& description, 
			   const MString& thePgmName)
     : BaseInducer(description),
       pgmName(thePgmName)
{
   set_pgm_name(defaultPgmName + " ");

   set_discretization_levels(defaultDiscretizationLevels);
   set_nearest_neighbors(defaultNearestNeighbors);
   set_voting_scheme(defaultVotingScheme);
}

PeblsInducer::~PeblsInducer()
{
}
   


/***************************************************************************
  Description : Train and test by running Pebls's PEBLS program.
  Comments    :
***************************************************************************/
Real PeblsInducer::train_and_test(InstanceBag* trainingSet,
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

   trainList.normalize_bag(InstanceBag::extreme);
   int orgTrainSize = trainList.num_instances();
   trainList.remove_inst_with_unknown_attr();
   if (orgTrainSize != trainList.num_instances())
      err << "PeblsInducer.c::runPEBLS: train set contains unknown instances"
	  << fatal_error;
   int orgTestSize = testList.num_instances();
   testList.remove_inst_with_unknown_attr();
   if (orgTestSize != testList.num_instances())
      err << "PeblsInducer.c::runPEBLS: test set contains unknown instances"
	  << fatal_error;

   if (trainList.get_schema().num_attr() == 0) {
      // since Pebls do not handle 0-attribute instances, we run const
      // inducer here for completeness. Esp. this is necesary for FSS.
      LOG(1, " 0-attribute instances detected. Running const inducer."
	  << endl);
      ConstInducer constInd("const inducer");
      return constInd.train_and_test(&trainList, testList);
   }      
      
   return runPEBLS(get_pgm_name(),
		   get_discretization_levels(),
		   get_nearest_neighbors(),
		   get_voting_scheme(),
		   trainList,
		   testList);
}


/*****************************************************************************
  Description : Set user options.
  Comments    :
*****************************************************************************/
void PeblsInducer::set_user_options(const MString& preFix)
{
   set_pgm_name(get_option_string(preFix + "PEBLS_PGM_NAME",
				  defaultPgmName,
				  PGM_NAME_HELP,
				  TRUE));
   set_discretization_levels(get_option_int(preFix +
					    "PEBLS_DISCRETIZATION_LEVELS",
					    defaultDiscretizationLevels,
					    DISCRETIZATION_LEVELS_HELP,
					    TRUE));
   set_nearest_neighbors(get_option_int(preFix +
					"PEBLS_NEAREST_NEIGHBORS",
					defaultNearestNeighbors,
					NEAREST_NEIGHBORS_HELP,
					TRUE));
   set_voting_scheme(get_option_enum(preFix +
				     "PEBLS_VOTING_SCHEME",
				     votingSchemeEnum,
				     defaultVotingScheme,
				     VOTING_SCHEME_HELP,
				     TRUE));
}
