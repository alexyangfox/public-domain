// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _BagMinArray_h
#define _BagMinArray_h 1
#include <BagSet.h>
#include <MinArray.h>
#include <BagAndDistance.h>
#include <IBInducer.h>

class BagMinArray : protected MinArray<BagAndDistance> {
public:
   void OK(int /* level = 0 */) const;
   BagMinArray(int k) : MinArray<BagAndDistance>(k){}
   void insert(const InstanceRC& inst, Real distance); 
   int num_elem() const { return MinArray<BagAndDistance>::num_elem(); }
   const BagAndDistance& operator[](int i) const {
      return MinArray<BagAndDistance>::operator[](i);
   }
   BagAndDistance& operator[](int i) {
      return MinArray<BagAndDistance>::operator[](i);
   }
   
   const BagAndDistance& max() const { return MinArray<BagAndDistance>::
				       max(); }
   const BagAndDistance& max(int& idx) const
          { return MinArray<BagAndDistance>::max(idx); }
   Category major_category(const IBInducer::NeighborVote vote,
				 const IBInducer::NnkValue nnk) const;
   void display(MLCOStream& stream = Mcout, Bool normalizeReal = FALSE) const; 
};

DECLARE_DISPLAY(BagMinArray);
#endif

