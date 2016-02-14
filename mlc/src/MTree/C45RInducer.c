// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : External Inducer interfacing C4.5rules
  Assumptions  : shell must find the program in the path.
  Comments     : See C45Inducer.
  Complexity   :
  Enhancements :
  History      : Ronny Kohavi                                       4/5/95
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <C45RInducer.h>
#include <InstList.h>
#include <mlcIO.h>  // for get_temp_file_name
#include <CatTestResult.h>


RCSID("MLC++, $RCSfile: C45RInducer.c,v $ $Revision: 1.3 $")

MString C45RInducer::defaultPgmName1("c4.5 ");
MString C45RInducer::defaultPgmName2("c4.5rules ");
// The %s will be replaced by the file name
MString C45RInducer::defaultPgmFlags1("-f %s ");
MString C45RInducer::defaultPgmFlags2("-u -f %s; echo ");

/***************************************************************************
  Description : Given the C4.5 output file, this function will 
		  parse it to determine the number of rules and number of
		  errors.
		The comment is printed if there's an error, to help
		  the user execute the command outside MLC++.
		The file returns a Boolean which indicates failure to
		  parse the file.  Due to many common cases of
		  deletion of the temporary files by people, we
		  suggest that the caller reattempt to generate the
		  parse file.
  Comments    : File static 
***************************************************************************/

static Bool parseC45R(const MString& parseFile, const MString& comment,
		     int& numErrors, int& numRules, int& numTotalConjuntions)
{
   // read in unpruned tree data.  Check if the file exists first.  This isn't
   // foolproof, since we may think it does, but it will be removed, between
   // the two statements, but that has very low probability.
   if (!file_exists(parseFile)) {
      Mcerr << "C45Interface::parseC45: cannot find interface file" << endl;
      return FALSE;
   }
   
   MLCIStream c45InfoLine(parseFile);
   
   if (c45InfoLine.peek() == EOF) {
      Mcerr << "C45Interface::parseC45: Empty C4.5 file " << parseFile << '\n'
          << comment << endl;
      return FALSE;
   }

   MString line;

   Bool testSection = FALSE;
   numErrors = -1;

   int errorPos = -1;
   int parPos = -1;
   numRules = 0;
   numTotalConjuntions = 0;
   while (c45InfoLine.peek() != EOF) {
      line.get_line(c45InfoLine);
      if(line.contains("Rule ", 0) && line.contains(":", line.length() - 1)) {
	 GLOBLOG(2, "Find a rule at line:" << line <<  endl);
	 numRules++;
	 Bool leaf;
	 int numConjuntions = 0;
	 do {
	    line.get_line(c45InfoLine);
	    leaf = line.contains("->");
	    if (!leaf)
	       numConjuntions++;
	 } while (!leaf);
	 numTotalConjuntions += numConjuntions;
	 GLOBLOG(2, "This rule has " << numConjuntions << " conjuntions" <<
		 endl);
      }
      if (!testSection) {
	 if ( line.contains("test data (") ) 
	    testSection = TRUE;
      } else
	 if (line.contains("Tested", 0) ) {
	    for (int pos = 0;
		 (pos <= line.length() -1) && (numErrors == -1) ;
		 pos++) {
	       if (line.contains("errors", pos))
		  errorPos = pos;
	       if (line.contains("(", pos))
		  parPos = pos;
	    }
	    GLOBLOG(1, "Parsing ..." <<  line << endl);
	    MString numString = line.substring(errorPos + 7, parPos -
						errorPos - 7 - 1);
	    GLOBLOG(1, "number string: " << numString << endl);
	    numErrors = int(numString.long_value());
	 }
   }
   GLOBLOG(1, "Number conjuntions: " << numTotalConjuntions << endl);
   return TRUE;
}

/***************************************************************************
  Description : Run C4.5rules, parse the output.
                See C45Inducer
  Comments    :
***************************************************************************/

Bool runC45R(const MString& c45Pgm1, const MString& c45Pgm2,
	     const InstanceBag& trainingSet, const InstanceBag& testList,
	     int& numErr, int& numRules, int& numConjuntions,
	     const MString& tmpFileStem,
	     MLCOStream& namesStream, MLCOStream& dataStream,
	     MLCOStream& testStream)
{
   MString c45PgmArg = insert_arg(c45Pgm1, tmpFileStem) + "; " +
                       insert_arg(c45Pgm2, tmpFileStem);
   GLOBLOG(3, "Running C4.5rules as " << c45PgmArg << endl);

   namesStream.get_stream().seekp(0);
   dataStream.get_stream().seekp(0);
   testStream.get_stream().seekp(0);
   truncate_file(namesStream);
   truncate_file(dataStream);
   truncate_file(testStream);

   trainingSet.display_names(namesStream, TRUE, "C45Interface generated");
   dataStream << trainingSet;
   testStream << testList;

   namesStream << flush; 
   dataStream << flush;   
   testStream << flush;

   // run the program
   MString execLine("(" + c45PgmArg + ") > " + tmpFileStem + ".out");
   if (system(execLine)) {
      Mcerr << "C4.5rules program returned bad status.  Line executed was\n  "
            << execLine << endl;
      return FALSE;
   }
   if (!parseC45R(tmpFileStem + ".out", "Command used: " + c45PgmArg,
		  numErr, numRules, numConjuntions))
      return FALSE;

   remove_file(tmpFileStem + ".out");
   remove_file(tmpFileStem + ".tree");   
   remove_file(tmpFileStem + ".unpruned");

   GLOBLOG(3, "C4.5: num errors: " << numErr << " Rules: " << numRules <<endl);
   return TRUE;
}

   
Bool runC45R(const MString& c45Pgm1, const MString& c45Pgm2,
	    const InstanceBag& trainingSet, const InstanceBag& testList,
            int& numErr, int& numRules, int& numConjuntions)

{
   MString tmpFileStem(get_temp_file_name());
   
   MLCOStream namesStream(tmpFileStem + ".names");
   MLCOStream dataStream(tmpFileStem + ".data");
   MLCOStream testStream(tmpFileStem + ".test");

   if (!runC45R(c45Pgm1, c45Pgm2, trainingSet, testList, 
		numErr, numRules, numConjuntions, tmpFileStem,
	       namesStream, dataStream, testStream))
      return FALSE;
   
   namesStream.close();
   dataStream.close();
   testStream.close();
   
   remove_file(tmpFileStem + ".names");
   remove_file(tmpFileStem + ".data");
   remove_file(tmpFileStem + ".test");
   return TRUE;
}
   

/***************************************************************************
  Description : Constructor.  Open the files and leave them open
                Destructor removes them.
  Comments    :
***************************************************************************/
   
void C45RInducer::init_files()
{
   fileStem = get_temp_file_name();
   namesStream = new MLCOStream(fileStem + ".names");
   dataStream = new MLCOStream(fileStem + ".data");
   testStream = new MLCOStream(fileStem + ".test");
}


void C45RInducer::end_files()
{
   delete namesStream;
   delete dataStream;
   delete testStream;
   namesStream = dataStream = testStream = NULL;
}


C45RInducer::C45RInducer(const MString& description, 
		       const MString& theFlags1, const MString& theFlags2,
		       const MString& aPgmName1, const MString& aPgmName2)
     : BaseInducer(description),
       pgmFlags1(theFlags1),
       pgmFlags2(theFlags2),
       pgmName1(aPgmName1),
       pgmName2(aPgmName2)
{
   init_files();
}

C45RInducer::~C45RInducer()
{
   MString namesName = namesStream->description();
   MString dataName = dataStream->description();
   MString testName = testStream->description();
   end_files();
   remove_file(namesName);
   remove_file(dataName);
   remove_file(testName); 
}
   


/***************************************************************************
  Description : Train and test using the open streams.
                The version getting a testFile name may be wasteful as we are
		  copying the file to the filestem file.
  Comments    :
***************************************************************************/


Real C45RInducer::train_and_test(InstanceBag* trainingSet,
 			        const InstanceBag& testBag)
{
   int  numErr;
   int  numRules;
   int  numConjuntions;
   
   int saveGlobLogLevel = globalLogLevel;
   globalLogLevel = max(0, get_log_level() - 1);
   Bool ok;
   int numRuns = 0;
   do {
      ok = runC45R(pgmName1 + pgmFlags1, pgmName2 + pgmFlags2,
      *trainingSet, testBag, numErr, numRules, numConjuntions, fileStem,
      *namesStream, *dataStream, *testStream);

      if (!ok) {
	 Mcerr << "C45RInducer: resetting files.  Perhaps someone removed "
	    "interface files" << endl;
	 end_files();
	 init_files();
      }
   } while (numRuns++ < 3 && !ok);
   if (!ok)
      err << "C45RInducer::train_and_test: calls to C4.5rules failed"
	  << " Check parameters and file contents" << fatal_error;
   if (numRuns > 1)
      Mcerr << "C45RInducer: Recovered.  Everything OK" << endl;
   globalLogLevel = saveGlobLogLevel;

   static int totalNumRules = 0;
   static int totalNumConjuntions = 0;
   static int totalNumRuns = 0;
   totalNumRules += numRules;
   totalNumConjuntions += numConjuntions;
   totalNumRuns++; 
   LOG(0, "Runs: " << totalNumRuns << " AvgRules: " << Real(totalNumRules) /
       totalNumRuns << " AvgConjuntions: " << Real(totalNumConjuntions) /
       totalNumRuns << endl);
   int numInst = testBag.num_instances();
   Real acc = 1 - Real(numErr) / numInst;
   Real confLow, confHigh;
   CatTestResult::confidence(confLow, confHigh, acc, numInst);
   LOG(2, "C45RInducer::train_and_test" << "Accuracy: " << MString(acc*100,4)
	  << "% +- " << MString(CatTestResult::theoretical_std_dev(acc, 
	     numInst)*100,4) << '%' << " [" << MString(confLow*100, 4)
       << "% - " << MString(confHigh*100, 4) << "%]" << endl);

   return acc;
}
