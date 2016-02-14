// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "NaiveBayesInd.c".

#ifndef NaiveBayesInd_h
#define NaiveBayesInd_h


#include <basics.h>
#include <CtrInducer.h>
#include <AugCategory.h>
#include <Instance.h>
#include <BagSet.h>
#include <NaiveBayesCat.h>

class NaiveBayesCat;  // includes definition of struct NBNorm


class NaiveBayesInd : public CtrInducer {
   NO_COPY_CTOR(NaiveBayesInd);
   NaiveBayesCat * categorizer;
   int cAttrCount;
   Array2<NBNorm> * normDens;			// 1 row per attribute
   void clear();
   void cleanup();
   
public:
   // Value to use for Variance when actual variance = 0:
   static Real epsilon;
   // Value to use for Vaiance when actual variance is undefined becase there
   // is only one occurance.
   static Real defaultVariance;

   NaiveBayesInd(const MString& description);
   NaiveBayesInd(const NaiveBayesInd& source, CtorDummy);
   virtual ~NaiveBayesInd();

   virtual Bool was_trained(Bool fatalOnFalse = TRUE) const;
   virtual const Categorizer& get_categorizer() const;

   virtual void train();

   virtual Bool can_cast_to_incr_inducer() const {return FALSE;}
   virtual void display(MLCOStream& stream = Mcout,
			    const DisplayPref& dp = defaultDisplayPref) const;
   virtual Inducer* copy() const; 
};

DECLARE_DISPLAY(NaiveBayesInd);
#endif









