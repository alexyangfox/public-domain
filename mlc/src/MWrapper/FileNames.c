// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description : A class for handling file names, mostly for utilities.  You
                  can ask this class for the names file, data file, and test
                  file, and it will do its best to derive those names in a 
                  smart manner, offering reasonable options to users.
                The DATAFILE option is the basis for all the heuristics.
                  The stem (up to the period) is called the root, and all 
                  other names are derived from the root by appending the
                  correct suffixes. 
                If the DATAFILE contains a period and the suffix ".all"
                   we assume that this contains all the data.
		   We still allow the user to explicitly select a test
		   file, but do not suggest one.
		   
  Assumptions : 
  Comments     :
  Complexity   :
  Enhancements :
  History      : Ronny Kohavi                                       12/16/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <FileNames.h>
#include <GetOption.h> 
#include <mlcIO.h>

RCSID("MLC++, $RCSfile: FileNames.c,v $ $Revision: 1.16 $")

static MString dataFileHelp = "Datafile containing instances.  A '.data' is "
  "appended if no suffix is given.  A '.all' suffix implies that "
  "there is no test set available";

static MString namesFileHelp = "Namesfile describing how to parse the "
  " datafile.  A '.names' is appended if no suffix is given.";

static MString testFileHelp = "Testfile containing instances.  A '.test' is "
  "appended if no suffix is given.";

static MString dumpStemHelp = "Name of a file stem for dumping instances.";


/***************************************************************************
  Description : Constructor
  Comments    :
***************************************************************************/

FileNames::FileNames()
{
   dataFile = get_option_string("DATAFILE", "", dataFileHelp);
   suggestTestFile = TRUE;
   testFilePrompted = FALSE;
   if (dataFile.contains(".")) {
      rootName = MString(dataFile, dataFile.index("."));
      if (MString((const char *)dataFile + rootName.length()) == ".all")
	 suggestTestFile = FALSE;
   }
   else {
      rootName = dataFile;
      dataFile += ".data";
   }

   dumpStem = "";

   (void)names_file(); // prompt for it now because it's related.
}

/***************************************************************************
  Description : Suggest namesfile, testfile
  Comments    :
***************************************************************************/

MString FileNames::data_file() const
{
   return dataFile;
}

MString FileNames::names_file()
{
   if (namesFile == "") {
      namesFile = get_option_string("NAMESFILE", rootName + ".names",
				    namesFileHelp, TRUE);   
      if (!namesFile.contains("."))
	 namesFile += ".names";
   }
   return namesFile;
}

MString FileNames::test_file(Bool abortOnEmpty)
{
   if (!testFilePrompted) {
      // NOTE: use " " instead of "" for the suggested test file name
      // to not suggest a test file.  Otherwise we'll be asking for a
      // nuisance option with no default value, which is illegal.
      MString suggestedTestName;
      if(suggestTestFile) {
	 suggestedTestName = MString(rootName + ".test");
	 if (!file_exists(suggestedTestName))
	    suggestedTestName = " ";
      }
      else
	 suggestedTestName = " ";
      
      testFile = get_option_string("TESTFILE", suggestedTestName,
				    testFileHelp, TRUE);   
      if(testFile == " ")
	 testFile = "";
      if (testFile != "" && !testFile.contains("."))
	 testFile += ".test";

      if(testFile != "" && !suggestTestFile)
	 Mcerr << "Warning: using DATAFILE with .all suffix.\n"
               "   If the TESTFILE instances overlap, the accuracy estimate"
               " is invalid!" << endl;
      
      testFilePrompted = TRUE;
   }
   
   if (testFile == "" && abortOnEmpty)
      err << "FileNames::test_file: No TESTFILE specified" << fatal_error;
   return testFile;
}

MString FileNames::dump_stem()
{
   if(dumpStem == "")
      dumpStem = get_option_string("DUMPSTEM","", dumpStemHelp);
   
   return dumpStem;
}  

