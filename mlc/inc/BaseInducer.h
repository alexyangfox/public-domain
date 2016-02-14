// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _BaseInducer_h
#define _BaseInducer_h 1

#define CONST_INDUCER       1
#define TDDT_INDUCER        2
#define TABLE_INDUCER       3
#define C45_INDUCER         4
#define FSS_INDUCER         5
#define OODG_INDUCER        6
#define NAIVE_BAYES_INDUCER 7
#define NULL_INDUCER        8
#define IB_INDUCER          9
#define ONER_INDUCER       10
#define DISC_TAB_INDUCER   11
#define DISC_NB_INDUCER    12
#define DF_INDUCER         13
#define LIST_HOODG_INDUCER 14
#define WINNOW_INDUCER     15

#include <LogOptions.h>
#include <DisplayPref.h>
#include <AugCategory.h>

class InstanceList;
class InstanceBag;
class CtrInstanceBag;
class CtrInstanceList;
class Categorizer;
class Inducer;
class IncrInducer;

extern const MString defaultDataExt;
extern const MString defaultNamesExt;
extern const MString defaultTestExt;

class BaseInducer { // ABC
   LOG_OPTIONS;
   const MString dscr;
   NO_COPY_CTOR(BaseInducer);
   
protected:
   InstanceBag *TS;
   // Gets ownership
   virtual InstanceBag* assign_bag(InstanceBag*& newTS);
   
public:
   virtual void OK(int level = 0) const {(void)level;}
   BaseInducer(const MString& description) : dscr(description) {TS=NULL;}
   BaseInducer(const BaseInducer& source, CtorDummy ctorDummy);
   virtual ~BaseInducer();
   // Returns the class id
   //@@ Temporary, change this to pure virtual
   virtual int class_id() const { return -1;}
   const MString& description() const {return dscr;};
   // read_data reads the training set.  The file name suffices for the data
  // file and names file default to ".data" and ".names"
  // respectively.  If the files have a different prefix, give an
  // empty file name, and give the full file name as suffix.
   virtual void read_data(const MString& file,
			  const MString& namesExtension = defaultNamesExt,
			  const MString& dataExtension = defaultDataExt);
   // See .c file for explanation of why these are non-virtual.
   InstanceBag* assign_data(InstanceBag*& newTS);
   InstanceBag* assign_data(InstanceList*& newTS);
   InstanceBag* assign_data(CtrInstanceBag*& newTS);
   InstanceBag* assign_data(CtrInstanceList*& newTS);
   // release_data gives you back the bag.
   virtual InstanceBag* release_data();
   virtual Bool has_data(Bool fatalOnFalse = TRUE) const;
   const InstanceBag& instance_bag() const;
  // These are non-const becuase we temporarily need to store the trainingSet
   // We need temporary ownership of trainingSet, but return it.
   virtual Real train_and_test(InstanceBag* trainingSet,
			       const InstanceBag& testBag) = 0;

   // Convenience function that reads the files in.
   // This isn't called train_and_test (the obvious choice) because derived
   //    classes will hide this function if they only define the InstanceBag
   //    version (scope is different).
   virtual Real train_and_test_files(const MString& fileStem,
			     const MString& namesExtension = defaultNamesExt,
			     const MString& dataExtension = defaultDataExt,
			     const MString& testExtension = defaultTestExt);
   virtual Bool can_cast_to_inducer() const;
   virtual Inducer& cast_to_inducer();
   virtual Bool can_cast_to_incr_inducer() const;
   virtual IncrInducer& cast_to_incr_inducer();
};

#endif
