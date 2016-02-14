// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _BoolArray_h
#define _BoolArray_h 1

#include <basics.h> // for Bool
#include <Array.h>
#include <fstream.h>
#include <MString.h>

class BoolArray : public Array<Bool> {
   NO_COPY_CTOR(BoolArray);
public:
   BoolArray(const BoolArray& source, CtorDummy);
   BoolArray(const Array<int>& source);
   BoolArray(int base, int size);
   BoolArray(int size);
   BoolArray(int base, int size, const Bool& initialValue);
   ~BoolArray() {}

   void display(MLCOStream& stream = Mcout) const;
   MString get_true_indexes() const;
   int num_true() const;
   int num_false() const { return size() - num_true(); }
};

DECLARE_DISPLAY(BoolArray);

//ofstream& operator<<(ofstream& s, const BoolArray& c);

#endif
