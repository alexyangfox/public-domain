// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _Bagging_h
#define _Bagging_h 1

#include <Categorizer.h>
#include <Array.h>


class BaggingCat : public Categorizer {
   LOG_OPTIONS;
   // we need a pointer here to get an ownership from outside.
   PtrArray<Categorizer *> * catSet;
   Array<Real> *weightSet;
   Real avgWeight;
   Bool useAboveAvgWeight;
   Array<int>  numCorrect;  // statistics: how many did you get correct
   int numCategorize;       // number of categorizations done
   NO_COPY_CTOR(BaggingCat);
   
public:

   static Bool defaultUseAboveAvgWeight;
   virtual void OK(int level = 0)const;

   BaggingCat( const MString& dscr,
	       PtrArray<Categorizer *>*& cats,
	       Array<Real>*& weights);
   
   BaggingCat( const BaggingCat& source,
	       const CtorDummy /* dummyArg */);

   virtual ~BaggingCat();

   virtual const PtrArray<Categorizer *> & get_categorizer_set() const
       {return *catSet;}
   virtual const Array<Real> & get_weight_set() const
       {return *weightSet;}   

   virtual AugCategory categorize(const InstanceRC& inst) const;

   virtual void display_struct(MLCOStream& stream,
			       const DisplayPref& dp) const;

   virtual void display(MLCOStream& stream = Mcout,
			const DisplayPref& dp = defaultDisplayPref) const;

   virtual Categorizer* copy() const;

   
   virtual int class_id() const { return CLASS_BAGGING_CATEGORIZER; }

   virtual Bool operator==(const Categorizer &cat) const;
   virtual Bool operator==(const BaggingCat &cat) const;

   virtual void set_use_above_avg_weight(Bool val) {
      useAboveAvgWeight = val; }
   virtual Bool get_use_above_avg_weight() const { return useAboveAvgWeight; }
   
};

DECLARE_DISPLAY(BaggingCat);

#endif




