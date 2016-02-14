// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _LIGenFunct_h
#define _LIGenFunct_h 1

#include <MRandom.h>
#include <Schema.h>
#include <Instance.h>

class AttrGenFunctor {  // ABC

protected:
   MRandom& randGen;  // used by the derived classes

public:
   AttrGenFunctor(MRandom& aRandGen): randGen(aRandGen) {}
   virtual ~AttrGenFunctor() {}   // nothing to deallocate
   virtual AttrValue_ operator() (const AttrInfo& ai) = 0;
};

class UniformAttrGenFunctor : public AttrGenFunctor {
public:
   UniformAttrGenFunctor(MRandom& aRandGen) : AttrGenFunctor(aRandGen) {}
   virtual AttrValue_ operator() (const AttrInfo& ai);
};

// SkewedAttr is only valid for nominal attributes.
// It generates the value 0 p percent of the time.
class SkewedAttrGenFunctor : public AttrGenFunctor {
   Real probZero;
public:
   SkewedAttrGenFunctor(MRandom& aRandGen, Real p) : 
      AttrGenFunctor(aRandGen), probZero(p) {}
   virtual AttrValue_ operator() (const AttrInfo& ai);
};


class IndependentLabInstGenerator;  // used by LabInstGenFunctor

class LabInstGenFunctor {  // ABC

public:
   virtual ~LabInstGenFunctor() {}  // nothing to deallocate
   virtual InstanceRC operator() () = 0;
};

class IndependentLabInstGenFunctor : public LabInstGenFunctor {
protected:
   // can be used by the derived classes
   const SchemaRC labInstSchema;
   const IndependentLabInstGenerator& labInstGen;

public:
   IndependentLabInstGenFunctor(const SchemaRC& lii,
				const IndependentLabInstGenerator& lig);
   virtual InstanceRC operator() ();
};


class  LabelGenFunctor {  // ABC
   
public:
   LabelGenFunctor() {}
   virtual ~LabelGenFunctor() {}  // nothing to deallocate
   virtual void operator() (const InstanceRC& inst, AttrValue_& label,
			    AttrValue_& noisyLabel) = 0;
};

#endif
