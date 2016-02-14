// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _C45Inducer_h
#define _C45Inducer_h 1

#include <Inducer.h>
#include <SchemaRC.h>
#include <MLCStream.h>
#include <DecisionTree.h>

// These are useful as global functions.  In fact, they provide
//   more functionality than the inducer above.
// The first parameter should be set to the command line that
//   will execute C4.5, and pick the correct line out (through awk).
Bool runC45(const MString& c45Pgm, 
	    const InstanceBag& trainingSet, const InstanceBag& testBag,
	    Real& pruneAcc, Real& noPruneAcc, Real& estimateAcc,
	    int& noPruneSize, int& pruneSize);


// This version is intended to be used if you're running C45 a lot.
// Profiling showed that file creation (creat) and deletion (unlink) were
//   taking up to 40% of the time in some cases, so this version takes open
//   streams, writes to them, and truncates.
Bool runC45(const MString& c45Pgm,
	    const InstanceList& trainingList, const InstanceList& testList,
	    Real& pruneAcc, Real& noPruneAcc, Real& estimateAcc,
	    int& noPruneSize, int& pruneSize, const MString& tmpFileStem,
            MLCOStream& namesStream, MLCOStream& dataStream,
	    MLCOStream& testStream);

// This function reads in a c45Tree datafile and builds a decision tree.
// Does just attribute categorizers and constant cat (aborts otherwise).
// See C45Tree.c before using this function.
DecisionTree* read_c45_tree(MLCIStream& file,
			     const SchemaRC& schema,
			     DecisionTree* dt);

DecisionTree* read_c45_tree(MLCIStream& file,
			     const SchemaRC& schema);


class C45Inducer : public BaseInducer {
   Bool usePrunedTree;
   Bool cleanupNeeded; // If we run C4.5, we need a cleanup of some files it
		       // generates.
   MString pgmName;
   MString pgmFlags;
   MString pgmSuffix;
   
   MString fileStem;
   MLCOStream* namesStream;
   MLCOStream* dataStream;
   MLCOStream* testStream;
   void init_files();
   void end_files();
public:
   static MString defaultPgmName;
   static MString defaultPgmFlags;
   static MString defaultPgmSuffix;
   C45Inducer(const MString& description, 
	      const MString& theFlags = defaultPgmFlags,
	      Bool usePruned = TRUE,
	      const MString& aPgmName = defaultPgmName,
	      const MString& aPgmSuffix  = defaultPgmSuffix);
   C45Inducer(const C45Inducer&, CtorDummy ctorDummy);
   virtual ~C45Inducer();
   virtual int class_id() const { return C45_INDUCER; }
   virtual Real train_and_test(InstanceBag* trainingSet,
			       const InstanceBag& testList);
   virtual void set_pgm_flags(const MString &flags) { pgmFlags = flags; }
   virtual const MString& file_stem() { return fileStem;}
   const MString& get_flags() {return pgmFlags;}
};


#endif








