// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "Inducer.c".

#ifndef _CtrInducer_h
#define _CtrInducer_h 1

#include <Inducer.h>

class BagCounters;

class CtrInducer : public Inducer {
   CtrInstanceBag *TSWithCounters;
protected:
   // assign_bag() with a non-counter bag will generate an error.
   virtual InstanceBag* assign_bag(InstanceBag*& newTS); 
public:
   virtual void OK(int level = 0) const;
   CtrInducer(const MString& description);
   CtrInducer(const CtrInducer& source, CtorDummy);
   virtual ~CtrInducer();
   virtual void read_data(const MString& file,
			  const MString& namesExtension = defaultNamesExt,
			  const MString& dataExtension = defaultDataExt);
   virtual InstanceBag* release_data();
   virtual const CtrInstanceBag& TS_with_counters() const;
   virtual const BagCounters& counters() const;
};

#endif

