// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : External Inducer interfacing C4.5
  Assumptions  : shell must find the program in the path.
  Comments     : Interface files tended to be deleted by nightly cleanups
                   since they are usually written to /tmp.  Because many
		   machines are inefficient on other directories (e.g., NSF
		   mounts), longer runs were much more efficient when
		   interface files were written to /tmp.   If an interface
		   file is removed, the whole 25 hour run terminates, so we
		   thought it is important enough to attempt to retry the
		   interface.  This is why the MLCOstreams are pointers, and
		   why we have init_files, end_files, and retries on runC45.
		   This isn't fool-proof (see comment in code where it
		   sometimes failed), but it recovers with very high
		   probability).
  Complexity   :
  Enhancements : We're conservative about cleanup, i.e., we only clean
                   if we're sure there's junk left by C4.5.  If there
		   are errors and we're not sure, there's no cleanup,
		   which might leave some temporary files around.
  History      : Ronny Kohavi                                       10/8/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <C45Inducer.h>
#include <GetOption.h>
#include <InstList.h>
#include <mlcIO.h>  // for get_temp_file_name
#include <CatTestResult.h>
#include <math.h>
#include <BoolArray.h>
#include <DecisionTree.h>
#include <AttrCat.h>
#include <ThresholdCat.h>


RCSID("MLC++, $RCSfile: C45Inducer.c,v $ $Revision: 1.26 $")

MString C45Inducer::defaultPgmName("c4.5 ");
// The %s will be replaced by the file name
MString C45Inducer::defaultPgmFlags("-u -f %s ");
MString C45Inducer::defaultPgmSuffix(" | awk -f $MLCDIR/c45test.awk");

static int totalNodesNum = 0;
static int callCount = 0;
static int totalPruneAttr = 0;
static int totalUnpruneAttr = 0;
static int countAttr = 0;

class C45terminate {
public:
   C45terminate() {};
   ~C45terminate() { if (callCount > 0) 
                     GLOBLOG(1, "Average nodes for all C4.5 trees:" << Real(totalNodesNum) / callCount
			     << "." << endl);
                     if (countAttr > 0)
		     GLOBLOG(1, "Average attributes: " << Real(totalPruneAttr)/countAttr
                             << " for pruned, " << Real(totalUnpruneAttr)/countAttr
                             << " for unpruned." << endl);}
};

C45terminate c45terminate;

/***************************************************************************
  Description : Given a line from the output of C4.5, this function will
		  parse it to determine the size and accuracy of both the
		  unpruned and pruned trees.
		The comment is printed if there's an error, to help
		  the user execute the command outside MLC++.
		The file returns a Boolean which indicates failure to
		  parse the file.  Due to many common cases of
		  deletion of the temporary files by people, we
		  suggest that the caller reattempt to generate the
		  parse file.
  Comments    : File static 
***************************************************************************/
static Bool parseC45(const MString& parseFile, const MString& comment,
		     int& numPruneErrs, Real& expPruneAcc,
		     int& numNoPruneErrs, Real& expNoPruneAcc,
		     Real& estimateAcc, int& noPruneSize, int& pruneSize)
{
   callCount ++;
 
   // read in unpruned tree data.  Check if the file exists first.  This isn't
   // foolproof, since we may think it does, but it will be removed, between
   // the two statements, but that has very low probability.
   if (!file_exists(parseFile)) {
      Mcerr << "C45Interface::parseC45: cannot find interface file" << endl;
      return FALSE;
   }
   
   MLCIStream c45InfoLine(parseFile);
   char c;
   
   if (c45InfoLine.peek() == EOF) {
      Mcerr << "C45Interface::parseC45: Empty C4.5 file " << parseFile << '\n'
          << comment << endl;
      return FALSE;
   }
   c45InfoLine >> noPruneSize;
   c45InfoLine >> numNoPruneErrs;
   while (c45InfoLine.get() != '(')
      ;  // skip
   c45InfoLine >> expNoPruneAcc;
   expNoPruneAcc = 1 - expNoPruneAcc/100.;
   c45InfoLine >> c;
   if (c != '%') {
      Mcerr << "C45Interface::parseC45: parse mismatch.  Do not see percent sign"
	     " after before-prunning error on C4.5 line\n"
          << comment << endl;
      return FALSE;
   } 

   // read in pruned tree data
   while (c45InfoLine.get() != ')')
      ;  // skip
   c45InfoLine >> pruneSize;
   totalNodesNum += pruneSize;

   c45InfoLine >> numPruneErrs;
   while (c45InfoLine.get() != '(')
      ;  // skip
   c45InfoLine >> expPruneAcc;
   expPruneAcc = 1 - expPruneAcc/100.;
   if (c != '%') {
      Mcerr << "C45Interface::parseC45: parse mismatch.  Do not see percent sign"
	     " after pruned error on C4.5 line\n"
          << comment << endl;
      return FALSE;
   }
   // read in estimated accuracy
   while (c45InfoLine.get() != '(')
      ;  // skip
   c45InfoLine >> estimateAcc;
   estimateAcc = estimateAcc / 100;
   if (c != '%') {
      err << "C45Interface::parseC45: parse mismatch.  Do not see percent sign"
	     " after estimated error on C4.5 line\n" 
          << comment << endl;
      return FALSE;
   }
   return TRUE;
}

/***************************************************************************
  Description : Run C4.5, parse the output line.
                The first argument should be set to the command that 
		  will execute C4.5, and pick the correct line out.
                The output is suppose to be one line containing
		    the desired information.
		c45test.awk is an awk script to get the appropriate line
		    out if you run with -u flag.
               This version is intended to be used if you're running C45 a lot.
                 Profiling showed that file creation (creat) and deletion
                 (unlink) were taking up to 40% of the time in some cases,
                 so this version takes open streams, writes to them, 
		 and truncates.
	       The next function is meant for single runs and have a quicker
  	         interface.
		The file returns a Boolean which indicates failure to
		  parse the file.  Due to many common cases of
		  deletion of the temporary files by people, we
		  suggest that the caller reattempt to generate the
		  parse file.
  Comments    :
***************************************************************************/

// @@ should be moved to CGraph,RootCatGraph
// maxAttr is the upper bound on number of attributes
int num_attr(const CatGraph& catGraph, int maxAttr)
{
   BoolArray attrs(0, maxAttr, FALSE);
   const CGraph& cgraph = catGraph.get_graph();

   NodePtr nodePtr;
   forall_nodes(nodePtr, cgraph) {
      const Categorizer& cat = catGraph.get_categorizer(nodePtr);
      if (cat.class_id() == CLASS_CONST_CATEGORIZER)
         ASSERT(cgraph.outdeg(nodePtr) == 0); // skip this, it's a leaf
      else if (cat.class_id() == CLASS_ATTR_CATEGORIZER) {
	 const AttrCategorizer& ac = (AttrCategorizer&)cat; // safe cast
	 int attrNum = ac.attr_num();
	 GLOBLOG(7, "Seeing attribute " << attrNum << endl);
	 ASSERT(attrNum < maxAttr);
	 attrs[attrNum] = TRUE;
      } else if (cat.class_id() == CLASS_THRESHOLD_CATEGORIZER) {
	 const ThresholdCategorizer& tc = (ThresholdCategorizer&)cat; // safe cast
	 int attrNum = tc.attr_num();
	 GLOBLOG(7, "Seeing attribute " << attrNum << endl);
	 ASSERT(attrNum < maxAttr);
	 attrs[attrNum] = TRUE;
      } else
	 err << "C45Inducer::num_attr: Unrecognized node type " << cat.class_id()
	     << fatal_error;
   }
   GLOBLOG(6, "There were " << attrs.num_true() << " attributes" << endl);
   return attrs.num_true();   
}


void get_c45_stats(const MString& fileStem, int maxAttr, const SchemaRC& schema)
{
   static bool get_c45_stats = get_option_bool("C45_STATS", FALSE,
         "Display statistics about number of attributes used", TRUE);

   if (!get_c45_stats)
      return;
   
   MLCIStream prunedTreeStream(fileStem + ".tree");
   DecisionTree *prunedTree = read_c45_tree(prunedTreeStream, schema);
   int numPruneAttr = num_attr(*prunedTree, maxAttr);
   delete prunedTree;
   
   MLCIStream unprunedTreeStream(fileStem + ".unpruned");
   DecisionTree *unprunedTree = read_c45_tree(unprunedTreeStream, schema);
   int numUnpruneAttr = num_attr(*unprunedTree, maxAttr);
   delete unprunedTree;
      
   totalPruneAttr += numPruneAttr;
   totalUnpruneAttr += numUnpruneAttr;
   countAttr++;
}

Bool runC45(const MString& c45Pgm,
	    const InstanceBag& trainingSet, const InstanceBag& testList,
	    Real& pruneAcc, Real& noPruneAcc, Real& estimateAcc,
	    int& noPruneSize, int& pruneSize, const MString& tmpFileStem,
            MLCOStream& namesStream, MLCOStream& dataStream,
	    MLCOStream& testStream)
{
   MString c45PgmArg = insert_arg(c45Pgm, tmpFileStem);
   // @@ change to FLOG
   GLOBLOG(6, "Running C4.5 as " << c45PgmArg << endl);


   namesStream.get_stream().seekp(0);
   dataStream.get_stream().seekp(0);
   testStream.get_stream().seekp(0);
   truncate_file(namesStream);
   truncate_file(dataStream);
   truncate_file(testStream);

   trainingSet.display_names(namesStream, TRUE, "C45Interface generated");
   dataStream << trainingSet;
   testStream << testList;
   int numTestSet = testList.num_instances();

   namesStream << flush; 
   dataStream << flush;   
   testStream << flush;

   // run the program
   MString execLine(c45PgmArg + " > " + tmpFileStem + ".out");
   if (system(execLine)) {
      Mcerr << "C4.5 program returned bad status.  Line executed was\n  "
            << execLine << endl;
      return FALSE;
   }
   int numPruneErrors, numNoPruneErrors;
   Real expPruneAcc, expNoPruneAcc;
   if (!parseC45(tmpFileStem + ".out", "Command used: " + c45PgmArg,
		 numPruneErrors, expPruneAcc, numNoPruneErrors,
		 expNoPruneAcc, estimateAcc, noPruneSize, pruneSize))
      return FALSE;

   // now calculate pruned acc and noPruned acc using test set size.

   pruneAcc = 1 - (Real)numPruneErrors / (Real)numTestSet;
   if (fabs(expPruneAcc - pruneAcc) > 0.001)
      err << "runC45: C45 calculated pruned acc is different from what "
	 "we computed by : " << fabs(expPruneAcc - pruneAcc) << fatal_error;
   noPruneAcc = 1 - (Real)numNoPruneErrors / (Real)numTestSet;
   if (fabs(expNoPruneAcc - noPruneAcc) > 0.001)
      err << "runC45: C45 calculated no pruned acc is different from what "
	 "we computed by : " << fabs(expNoPruneAcc - noPruneAcc) <<
	 fatal_error;   

   GLOBLOG(6, "C4.5: prunedAcc: " << pruneAcc << " noprunAcc: "
	   << noPruneAcc << " estimated acc: " << estimateAcc << endl);


   get_c45_stats(tmpFileStem, trainingSet.num_attr(), trainingSet.get_schema());
   return TRUE;
}

   
Bool runC45(const MString& c45Pgm,
	    const InstanceBag& trainingSet, const InstanceBag& testList,
	    Real& pruneAcc, Real& noPruneAcc, Real& estimateAcc,
	    int& noPruneSize, int& pruneSize)
{
   MString tmpFileStem(get_temp_file_name());
   
   MLCOStream namesStream(tmpFileStem + ".names");
   MLCOStream dataStream(tmpFileStem + ".data");
   MLCOStream testStream(tmpFileStem + ".test");

   if (!runC45(c45Pgm, trainingSet, testList, pruneAcc, noPruneAcc, estimateAcc,
	       noPruneSize, pruneSize, tmpFileStem,
	       namesStream, dataStream, testStream))
      return FALSE;
   
   namesStream.close();
   dataStream.close();
   testStream.close();
   
   remove_file(tmpFileStem + ".names");
   remove_file(tmpFileStem + ".data");
   remove_file(tmpFileStem + ".test");
   remove_file(tmpFileStem + ".out");
   remove_file(tmpFileStem + ".tree");   
   remove_file(tmpFileStem + ".unpruned");
   return TRUE;
}
   

/***************************************************************************
  Description : Constructor.  Open the files and leave them open
                Destructor removes them.
  Comments    :
***************************************************************************/
   
void C45Inducer::init_files()
{
   fileStem = get_temp_file_name();
   namesStream = new MLCOStream(fileStem + ".names");
   dataStream = new MLCOStream(fileStem + ".data");
   testStream = new MLCOStream(fileStem + ".test");
}


void C45Inducer::end_files()
{
   delete namesStream;
   delete dataStream;
   delete testStream;
   namesStream = dataStream = testStream = NULL;
}


C45Inducer::C45Inducer(const MString& description, 
		       const MString& theFlags, Bool usePruned,
		       const MString& aPgmName, const MString& aPgmSuffix)
     : BaseInducer(description),
       pgmFlags(theFlags),
       usePrunedTree(usePruned),
       cleanupNeeded(FALSE),
       pgmName(aPgmName),
       pgmSuffix(aPgmSuffix)
{
   init_files();
}

C45Inducer::C45Inducer(const C45Inducer& src, CtorDummy ctorDummy)
     : BaseInducer(src, ctorDummy),
       pgmFlags(src.pgmFlags),
       usePrunedTree(src.usePrunedTree),
       cleanupNeeded(FALSE),
       pgmName(src.pgmName),
       pgmSuffix(src.pgmSuffix)
{
   init_files();
}


C45Inducer::~C45Inducer()
{
   MString namesName = namesStream->description();
   MString dataName = dataStream->description();
   MString testName = testStream->description();
   end_files();
   remove_file(namesName);
   remove_file(dataName);
   remove_file(testName); 
   if (cleanupNeeded) {
      remove_file(fileStem + ".out");
      remove_file(fileStem + ".tree");   
      remove_file(fileStem + ".unpruned");
   }
}
   


/***************************************************************************
  Description : Train and test using the open streams.
                The version getting a testFile name may be wasteful as we are
		  copying the file to the filestem file.
  Comments    :
***************************************************************************/


Real C45Inducer::train_and_test(InstanceBag* trainingSet,
 			        const InstanceBag& testBag)
{
   Real pruneAcc, noPruneAcc, estimateAcc;
   int  noPruneSize, pruneSize;
   static int maxTries = get_option_int("C45_RETRIES", 0, 
         "Number of times to retry C4.5 interface.  Important for "
	 " huge experiments where night runs clean /tmp", TRUE);
   

   Bool ok;
   int numRuns = 0;
   do {
      ok = runC45(pgmName + pgmFlags + pgmSuffix,
      *trainingSet, testBag, pruneAcc, noPruneAcc, estimateAcc,
      noPruneSize, pruneSize, fileStem,
      *namesStream, *dataStream, *testStream);

      if (!ok && maxTries > 0) {
	 Mcerr << "C45Inducer: resetting files.  Perhaps someone removed "
	    "interface files" << endl;
	 end_files();
	 init_files();
      }
   } while (numRuns++ < maxTries && !ok);
   if (!ok)
      err << "C45Inducer::train_and_test: calls to C4.5 failed"
	  << " Check parameters and file contents" << fatal_error;
   if (numRuns > 1)
      Mcerr << "C45Inducer: Recovered.  Everything OK" << endl;

   cleanupNeeded = TRUE;
   Real acc = usePrunedTree ? pruneAcc : noPruneAcc;
   Real confLow, confHigh;
   int numInst = testBag.num_instances();
   CatTestResult::confidence(confLow, confHigh, acc, numInst);
   LOG(2, "C45Inducer::train_and_test: " << "Accuracy: " << MString(acc*100,2)
	  << "% +- " << MString(CatTestResult::theoretical_std_dev(acc, 
	     numInst)*100,2) << '%' << " [" << MString(confLow*100, 2)
       << "% - " << MString(confHigh*100, 2) << "%]" << endl);

   return acc;
}
