// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _NaiveBayesCat_h
#define _NaiveBayesCat_h 1

#include <Categorizer.h>
#include <MLCStream.h> 
#include <CtrInstList.h>
#include <StatData.h>
#include <Array2.h>


// structure to hold values needed to express Normal Density function.
// allow default copy constructor
class NBNorm {
   Real mean;
   Real var;
   Bool hasData;

public:
   NBNorm( Real m=0.0, Real v=0.0, Bool hd=FALSE );
   Bool operator==(const NBNorm& nbn2) const;
   Bool operator!=(const NBNorm& nbn2) const;
   friend class NaiveBayesCat;
   friend class NaiveBayesInd;
};


// NaiveBayes Categorizer returns the category with the highest likelihood 
// given the contents of the training bag, assuming independence between
// attributes.
class NaiveBayesCat : public Categorizer {
   NO_COPY_CTOR(NaiveBayesCat);
   BagCounters nominCounts;		// hold data on nominal attributs
   Array2<NBNorm>* continNorm;		// hold data on real attributes
   int numTrainInst;
   int numAttributes;

   void NaiveBayesCat::log_prob(const Array<Real>& prob,
				    const SchemaRC& instSchema) const;
public:
   // Fraction of a single occurence to use in cases when a label
   // has no occurences of a given nominal value in the training set:
   static Real noMatchesFactor;
   
   virtual void OK(int = 0) const;

   NaiveBayesCat(const MString& dscr, int numCategories,
		 const BagCounters& bc, 
		 const Array2<NBNorm> * normDens,
		 int numInst);
   
   NaiveBayesCat(const NaiveBayesCat&, CtorDummy);
   
   virtual ~NaiveBayesCat();
   virtual AugCategory categorize(const InstanceRC&) const;

   virtual void display_struct(MLCOStream& stream = Mcout,
			const DisplayPref& dp = defaultDisplayPref) const;

   // Returns current number of training instances
   virtual int num_train_instances() const { return numTrainInst; }

   // Returns a pointer to a deep copy of this NaiveBayesCat
   virtual Categorizer* copy() const;

   virtual int class_id() const {return CLASS_NB_CATEGORIZER;}

   virtual Bool operator==(const Categorizer&) const;
   virtual Bool operator==(const NaiveBayesCat&) const;
};

#endif



