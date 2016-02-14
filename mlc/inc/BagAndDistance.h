// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef BagAndDistance_h
#define BagAndDistance_h 1
#include <BagSet.h>

class BagAndDistance {
private:
   NO_COPY_CTOR(BagAndDistance);
   InstanceBag* bag;
   Real distance;

public:
   BagAndDistance() { bag = NULL; distance = REAL_MAX; }
   ~BagAndDistance() { delete bag; }
   BagAndDistance& operator=(const BagAndDistance& elem);
   BagAndDistance(const InstanceRC& instance, Real dist);
   Real get_distance() const;
   void set_distance(Real dist);
   void insert(const InstanceRC& instance, Real dist);
   const InstanceBag& get_bag() const;
   int operator<(const BagAndDistance& element) const;
   int operator>(const BagAndDistance& element) const;
   int operator==(const BagAndDistance& element) const;
   void display(MLCOStream& stream = Mcout, Bool normalizeReal = FALSE) const;
};

DECLARE_DISPLAY(BagAndDistance);
#endif
