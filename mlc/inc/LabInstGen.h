// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _LabInstGen_h
#define _LabInstGen_h 1

#include <Array.h>
#include <MRandom.h>
#include <BagSet.h>
#include <LIGenFunct.h>


class LabelledInstanceGenerator {

   Real noiseRate;  // rate is a real number in [0,1]
   
   AttrValue_ generate_label(const InstanceRC&);
   
protected:
   LabInstGenFunctor& labInstGenFunctor;
   LabelGenFunctor&   labelGenFunctor;
   const SchemaRC schema;  // used by IndependentLabInstGenerator
   MRandom randNumGen;     // used by IndependentLabInstGenerator
   
public:
   LabelledInstanceGenerator(const SchemaRC& aSchema,
			     LabInstGenFunctor& instFunctor,
			     LabelGenFunctor& labFunctor);
   virtual ~LabelledInstanceGenerator() {}  // nothing to deallocate
   
   Real get_label_noise() const { return noiseRate; } 
   void set_label_noise(Real rate);
   void set_seed(int seed);
   // used to get the parameter to be passed to AttrGenFunctor.
   MRandom& get_rand_num_gen() { return randNumGen; }
      
   virtual InstanceRC get_lab_inst_unset_label(); 
   virtual InstanceRC get_labelled_inst();
};

class IndependentLabInstGenerator : public LabelledInstanceGenerator {
   PtrArray<AttrGenFunctor *> attrGenFunctors;
public:
   IndependentLabInstGenerator(const SchemaRC& schema,
			       LabelGenFunctor &labFunctor);
   // Note that PtrArray automatically deallocates the space pointed
   //   to by its elements. 
   virtual ~IndependentLabInstGenerator();
   void set_attr_generator(int attrNum, AttrGenFunctor*& attrGenFunctot);
   virtual AttrValue_ gen_attr_value(const AttrInfo&, int attrNum) const;
   // give ownership of new, get ownership of old
};

void data_generator(InstanceBag& lib, int numInstances, 
		    LabelledInstanceGenerator& lig, Bool allowDup = TRUE);
void full_space_generator(InstanceBag *fullSpaceLib);

#endif
