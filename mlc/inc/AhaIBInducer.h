// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _AhaIBInducer_h
#define _AhaIBInducer_h 1

#include <BaseInducer.h>
#include <Inducer.h>
#include <MLCStream.h>
#include <LogOptions.h>

#define AHA_IB_INDUCER 23

enum IBClass {ib1, ib2, ib3, ib4};

class AhaIBInducer : public BaseInducer {
private:   
   MString pgmName;   
   MString pgmFlags;
   IBClass ibClass;
   Bool storeAll;
   unsigned int randomSeed;   

public:
   static MString defaultPgmName;
   static MString defaultPgmFlags;
   static IBClass defaultIBClass;
   static Bool defaultStoreAll;
   static unsigned int defaultRandomSeed;   
   
   AhaIBInducer(const MString& description, 
		const MString& theFlags = defaultPgmFlags,
		const MString& thePgmName = defaultPgmName);

   virtual ~AhaIBInducer();
   virtual int class_id() const { return AHA_IB_INDUCER; }
   virtual Real train_and_test(InstanceBag* trainingSet,
			       const InstanceBag& testList);
   virtual void set_flags(const MString& flagsVal) { pgmFlags = flagsVal; }
   virtual MString get_flags() const { return pgmFlags; }
   virtual void set_pgm_name(const MString& name) { pgmName = name; }
   virtual MString get_pgm_name() const { return pgmName; }
   virtual void set_seed(unsigned int seed) { randomSeed  = seed; }
   virtual unsigned int get_seed() const { return randomSeed; }
   virtual void set_ib_class(IBClass val) { ibClass = val; }
   virtual IBClass get_ib_class() const { return ibClass; }
   virtual void set_store_all(Bool val) { storeAll = val; }
   virtual Bool get_store_all() const { return storeAll; }   
   virtual void set_user_options(const MString& prefix);
};
#endif





