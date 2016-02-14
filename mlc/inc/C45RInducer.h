// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _C45RInducer_h
#define _C45RInducer_h 1

#include <Inducer.h>
#include <SchemaRC.h>
#include <MLCStream.h>
#include <DecisionTree.h>

// These are useful as global functions.  In fact, they provide
//   more functionality than the inducer above.
// The first parameter should be set to the command line that
//   will execute C4.5, and pick the correct line out (through awk).
Bool runC45R(const MString& c45RPgm, 
 	     const InstanceBag& trainingSet, const InstanceBag& testBag,
	     int& numErr, int& numRules, int& numConjuntions);


// This version is intended to be used if you're running C45 a lot.
// Profiling showed that file creation (creat) and deletion (unlink) were
//   taking up to 40% of the time in some cases, so this version takes open
//   streams, writes to them, and truncates.
Bool runC45R(const MString& c45Pgm1, const MString& c45Pgm2,
	    const InstanceList& trainingList, const InstanceList& testList,
	    int& numErr, int& numRules, int& numConjuntions, MString&
	     tmpFileStem, MLCOStream& namesStream, MLCOStream& dataStream,
	    MLCOStream& testStream);

class C45RInducer : public BaseInducer {
   MString pgmName1;
   MString pgmFlags1;
   MString pgmName2;
   MString pgmFlags2;

   
   MString fileStem;
   MLCOStream* namesStream;
   MLCOStream* dataStream;
   MLCOStream* testStream;
   void init_files();
   void end_files();
public:
   static MString defaultPgmName1;
   static MString defaultPgmName2;
   static MString defaultPgmFlags1;
   static MString defaultPgmFlags2;
   static MString defaultPgmSuffix;
   C45RInducer(const MString& description, 
	      const MString& theFlags1 = defaultPgmFlags1,
	      const MString& theFlags2 = defaultPgmFlags2,
	      const MString& aPgmName1 = defaultPgmName1,
	      const MString& aPgmName2 = defaultPgmName2);
   virtual ~C45RInducer();
   virtual Real train_and_test(InstanceBag* trainingSet,
			       const InstanceBag& testList);
};


#endif








