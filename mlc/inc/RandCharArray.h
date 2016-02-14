// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef RandCharArray_h
#define RandCharArray_h

#include <basics.h>
#include <DynamicArray.h>
#include <MRandom.h>

// this is a dynamic array which is solely used for universal hashing.

class RandCharArray : public DynamicArray<unsigned char> {
   NO_COPY_CTOR(RandCharArray);
   MRandom randomVal;
protected:
   virtual void extend_array(int i);
public :
   RandCharArray(int size);
   RandCharArray(int size, unsigned int seed);
   virtual ~RandCharArray() {}
   void display(MLCOStream& stream = Mcout) const;
};

DECLARE_DISPLAY(RandCharArray);

#endif

