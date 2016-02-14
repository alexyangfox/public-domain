// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "NullInducer.c".

#ifndef _NullInducer_h
#define _NullInducer_h 1

#include <Inducer.h>

class NullInducer : public Inducer {
   Bool abortOnCalls;
   Bool trained;      // was train() called
protected:
   InstanceBag* assign_bag(InstanceBag*& newTS);
public:
  NullInducer(const MString& description, Bool fatalOnCalls = TRUE);
  virtual void read_data(const MString& file,
			 const MString& namesExtension = defaultNamesExt,
			 const MString& dataExtension = defaultDataExt);
  virtual InstanceBag* release_data();
  virtual Bool has_data(Bool fatalOnFalse = TRUE) const;
  const InstanceBag& instance_bag() const;
  virtual void train();
  virtual Bool was_trained(Bool fatalOnFalse = TRUE) const;
  virtual const Categorizer& get_categorizer() const;
  virtual void display_struct(MLCOStream& stream = Mcout,
		       const DisplayPref& dp = defaultDisplayPref) const;
};

#endif
