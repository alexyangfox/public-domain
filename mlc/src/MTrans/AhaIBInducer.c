// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : External Inducer interfacing Aha's IB inducers.
  Assumptions  : shell must find the Aha program in the path.
  Comments     : Every train_and_test() call will write three aha files
                   to the current directory and run aha program and delete
		   them after getting accuracy from its output file.
  Complexity   :
  Enhancements :
  History      : Yeogirl Yun                                   5/30/95
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <AhaIBInducer.h>
#include <InstList.h>
#include <mlcIO.h>  // for get_temp_file_name
#include <CatTestResult.h>
#include <convDisplay.h>
#include <LogOptions.h>
#include <GetOption.h>
#include <MEnum.h>


const MString PGM_NAME_HELP = "the name of Aha's IBL program";
const MString PGM_FLAGS_HELP = "the flags of Aha's IBL program";
const MString IBL_SEED_HELP = "the random seed to give IBL program";
const MString IB_CLASS_HELP = "the Aha's ibl algorithms. Namely ib1, ib2, "
                              "ib3, and ib4";
const MString STORE_ALL_HELP = "Stores all the instances.";

MString AhaIBInducer::defaultPgmName("ibl");
MString AhaIBInducer::defaultPgmFlags(" "); // by deault no flags. ibl will
				// set default values
				// automatically
IBClass AhaIBInducer::defaultIBClass = ib1;
unsigned int AhaIBInducer::defaultRandomSeed = 7258789;
Bool AhaIBInducer::defaultStoreAll = TRUE;


const MEnum ibClassEnum =
  MEnum("ib1", ib1) <<
  MEnum("ib2", ib2) <<
  MEnum("ib3", ib3) <<
  MEnum("ib4", ib4);


/*****************************************************************************
  Description : Parse the output of Aha's IBL program.
                Currently, it finds the last Te-accuracy statement of its
		  output file.
  Comments    :
*****************************************************************************/
static Real parse_output(const LogOptions& logOptions, const MString& fileName)
{
   MLCIStream file(fileName);
   
   MString line;
   MString lastAccuracy;
   while (!file.eof()) {
      line.get_line(file, '\n');
      FLOG(3, line << endl);
      if (line.contains("FATAL ERROR")) 
	 Mcerr << "\n AhaIBInducer::parse_output: Warning - "
	    "Aha's IBL program issued the following portion of message:\n"
	    << line << endl;
      if (line.contains("Te-Accuracy")) 
	 lastAccuracy = line;
   }

   FLOG(2, "The last accuracy line : " << lastAccuracy << endl);
   if (lastAccuracy == "")
      err << "Internal Error - Can't parse Aha's program output\n"
	 "See file " << fileName << fatal_error;

   int pos = lastAccuracy.index(" ", -1);
   Real acc = lastAccuracy.substring(pos, lastAccuracy.length() - pos).
                  real_value();
   FLOG(2, " The accuracy is " << acc << "%" << endl);   
   acc /= 100;
   return acc;
}


/***************************************************************************
  Description : run ibl and return accuracy given train and test sets.
  Comments    :
***************************************************************************/
static Real runIBL(const LogOptions& logOptions,
		   const MString& pgmName,
		   const MString& pgmFlags,
		   const MString& ibClass,
		   unsigned int seedNum,
		   Bool storeAll,
		   const InstanceList& trainList,
		   const InstanceList& testList)
{
   MString fileStem = get_temp_file_name();
   MString datafile = fileStem + ".data";
   MString testfile = fileStem + ".test";
   MString namesfile = fileStem + ".names";
   MString dscrfile = fileStem + ".dscr";
   MString outfile = fileStem + ".out";
   MString outfile2 = fileStem + ".out2";   

   MLCOStream outDatafile(datafile);
   MLCOStream outTestfile(testfile);
   MLCOStream outNamesfile(namesfile);
   MLCOStream outDescripfile(dscrfile);
   
   // now create Aha files.
   display_list_rep(trainList, trainList.get_schema(),
		    outDatafile, noConversion,
		    noNormalization, " ", " ", "", NULL);
   // Pass the trainList schema in case we ever want
   //   to normalize values
   display_list_rep(testList, trainList.get_schema(),
                    outTestfile, noConversion,
		    noNormalization, " ", " ", "", NULL);
   display_names_aha(trainList.get_schema(), outNamesfile);
   display_description_aha(trainList.get_schema(), outDescripfile);

   FLOG(3, "The seed is : " << seedNum << endl);

   outDatafile.close();
   outTestfile.close();
   outNamesfile.close();
   outDescripfile.close();
   MString newFlags = pgmFlags;
   if (storeAll)
      newFlags += " -storeall ";
   
   // now execute IBL.
   MString command = pgmName + " " + dscrfile + "  " + namesfile + " " +
      datafile + " " +  testfile + " " + outfile + " " + 
      MString((long)seedNum, 0) + " " + ibClass + " " + 
      newFlags + " > " + outfile2;
      
   // run it.
   if (system(command) != 0) 
      err <<  "AhaIBInducer::train_and_test: fail to run Aha's program as "
	   << command << fatal_error;

   Real acc = parse_output(logOptions, outfile2);
   // clean up.
   remove_file(datafile);
   remove_file(testfile);
   remove_file(namesfile);
   remove_file(dscrfile);
   remove_file(outfile);
   remove_file(outfile2);   

   return acc;
}



/*****************************************************************************
  Description : Constructor/Destructor.
                Intialize optoins.
  Comments    :
*****************************************************************************/
AhaIBInducer::AhaIBInducer(const MString& description, 
			   const MString& theFlags,
			   const MString& thePgmName)
     : BaseInducer(description),
       pgmFlags(theFlags),
       pgmName(thePgmName)
{
   set_pgm_name(defaultPgmName + " ");
   set_flags(defaultPgmFlags + " ");
   set_seed(defaultRandomSeed);
   set_ib_class(defaultIBClass);
}

AhaIBInducer::~AhaIBInducer()
{
}
   


/***************************************************************************
  Description : Train and test by running Aha's IBL program.
  Comments    :
***************************************************************************/
Real AhaIBInducer::train_and_test(InstanceBag* trainingSet,
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

   MString ibClass;
   IBClass ibClassVal = get_ib_class();
   if (ibClassVal == ib1)
      ibClass = "-ib1";
   else if (ibClassVal == ib2)
      ibClass = "-ib2";
   else if (ibClassVal == ib3)
      ibClass = "-ib3";
   else if (ibClassVal == ib4)
      ibClass = "-ib4";
   else
      err << "AhaIBInducer::train_and_test: unknown ib class : " <<
	 ibClassVal << fatal_error;
   
   return runIBL(get_log_options(), get_pgm_name(), get_flags(), ibClass,
		 get_seed(), get_store_all(), trainList, testList);
}


/*****************************************************************************
  Description : Set user options.
  Comments    :
*****************************************************************************/
void AhaIBInducer::set_user_options(const MString& preFix)
{
   set_pgm_name(get_option_string(preFix + "IBL_PGM_NAME",
				  defaultPgmName,
				  PGM_NAME_HELP,
				  TRUE));
   set_flags(get_option_string(preFix + "IBL_FLAGS",
			       defaultPgmFlags,
			       PGM_FLAGS_HELP,
			       TRUE));
   set_seed(get_option_int(preFix + "IBL_SEED",
			   get_seed(),
			   IBL_SEED_HELP,
			   TRUE));
   set_ib_class(get_option_enum(preFix + "IB_CLASS",
				ibClassEnum,
				defaultIBClass,
				IB_CLASS_HELP,
				TRUE));
   set_store_all(get_option_bool(preFix + "STORE_ALL",
				 defaultStoreAll,
				 STORE_ALL_HELP,
				 TRUE));
}
