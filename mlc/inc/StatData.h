// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef StatData_h
#define StatData_h 1
#include <basics.h>
#include <DynamicArray.h>

class StatData{
   friend Bool operator==(const StatData& sd1, const StatData& sd2);
   DynamicArray<Real> items;
public:
   // Note that we must initialize the DynamicArray with 0 because if we
   // initialize it with a high number, we'll have zeroes cropping in when
   // the array is sorted.
   StatData(): items(0) {}
   ~StatData(){}
   Real operator [](int index) const;
   void insert(Real item);
   int size() const { return items.size(); }
   Real mean(Real trim = 0) const;
   Real variance(Real trim = 0) const;
   Real std_dev(Real trim = 0) const;
   Real variance_of_mean(Real trim = 0) const;
   Real std_dev_of_mean(Real trim = 0) const;
   // percentil gives the confidence interval with the given probability
   void percentile(Real confIntProb, Real& low, Real& high) const;
   void display(MLCOStream& stream = Mcout) const;
   void display_math_histogram(Real columnWidth, int precision,
			       MLCOStream& stream = Mcout);
   virtual void append(const StatData& other);
   
   virtual void clear();
   virtual StatData& operator=(const StatData& other);
   Real squaredError(Real trueVal) const;
};

Bool operator==(const StatData& sd1, const StatData& sd2);
Bool operator!=(const StatData& sd1, const StatData& sd2);

DECLARE_DISPLAY(StatData);

#endif
