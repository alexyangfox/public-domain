// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef MaxArray_h
#define MaxArray_h

#include <basics.h>
#include <Array.h>

template <class Element>
class MaxArray : private Array<Element> {
   int numMaxElements;
   Element minValue;
   int     minIndex;
public:
   MaxArray(int k);
   void insert(const Element& element);
   INST const Element& operator[](int i) const {
      return Array<Element>::operator[](i);
   }
   Element& operator[](int i) {
      return Array<Element>::operator[](i);
   }
   int num_elem() { return numMaxElements; }
};
#endif


