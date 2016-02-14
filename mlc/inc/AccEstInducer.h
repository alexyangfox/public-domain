// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _AccEstInducer_h
#define _AccEstInducer_h 1

#include <BaseInducer.h>
#include <AccEstDispatch.h>
#include <MRandom.h>

class AccEstInducer : public BaseInducer {
   RAND_OPTIONS;

protected:
   AccEstDispatch accEst;
   BaseInducer *baseInducer;
   
public:
   AccEstInducer(const MString& prefix,
		 const MString& description, BaseInducer* ind = NULL);
   virtual ~AccEstInducer();
   
   Real train_and_test_files(const MString& fileStem,
			     const MString& namesExtension = defaultNamesExt,
			     const MString& dataExtension = defaultDataExt,
			     const MString& testExtension = defaultTestExt);
   void init(const MString& prefix); // get options from user
   void display(MLCOStream& stream = Mcout) const;
   virtual Real train_and_test(InstanceBag* trainingSet,
			       const InstanceBag& testList);
};

DECLARE_DISPLAY(AccEstInducer);

#endif
