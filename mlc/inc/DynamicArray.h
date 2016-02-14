// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef DynamicArray_h
#define DynamicArray_h

#include <basics.h>
#include <Array.h>

template <class Element>
class DynamicArray : public Array<Element> {
//   NO_COPY_CTOR(DynamicArray);
protected:   
   Element initialValue;
   int sizeAllocated; // physically allocated size. Note that this
		      // size is different from arraySize which is
                      // the size actually ever been accessed.
   Bool wasInitialized;
   int incrementSize;

   // These are virtual because it makes sense to override them.
   // See RandArray
   virtual int increase_size();
   virtual void extend_array(int sizeRequested);
public:

   DynamicArray(const DynamicArray& source, CtorDummy);
   DynamicArray(int base, int size, const Element& initialVal);   
   DynamicArray(int size, const Element& initialVal);
   DynamicArray(int size);

   virtual ~DynamicArray() {}

   INST void set_increment_size(int incrSize);
   INST int get_increment_size(void) const {return incrementSize;}
   INST DynamicArray& operator=(const DynamicArray& source);
   INST Element& index(int i);
   INST Element& operator[](int i);

   // These two methods do not have dynamic feature, because they
   // are const functions. So just call the base methods.
   // Note that due to ObjectCenter bug, you cannot inline and not
   //   inline based on const alone, so we force these in the .c file
   Array<Element>& operator=(const Array<Element>& source) {
      return Array<Element>::operator=(source);}
   const Element& index(int i) const {
      return Array<Element>::index(i); }
   const Element& operator[](int i) const {
      return Array<Element>::operator[](i); }
   INST void append(const Array<Element>& other);
   INST void truncate(int size);
};



#endif 


