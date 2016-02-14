// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef Array_h
#define Array_h

#include <stddef.h>
#include <basics.h>
#include <MLCStream.h>
#include <sortCompare.h>

// INST forces instantiation of the function to avoid duplicate template
// instantiations.  This is only to save compilation time at the expense of
// runtime performance 
#ifdef FASTINLINE
#  define INST
#else
#  define INST virtual
#endif

template <class Element>
class Array {
   NO_COPY_CTOR(Array);
protected:   // in case we want to derive a DynamicArray from Array.
   int base, arraySize; 
   Element *elements;   // the name array conflicts with LEDA
   INST void alloc_array(int base, int size); 
   INST void copy(const Array& source); // used by copy constructor
public:
   Array(const Array& source, CtorDummy);
   Array(int base, int size);
   Array(int size);
   Array(int base, int size, const Element& initialValue); 
   INST ~Array() { delete[] elements; }  
   INST Array& operator=(const Array& source);
   INST Element *get_elements() { return elements; } 
   INST void init_values(const Element& initalValue);
   INST int high () const { return base + arraySize - 1; } 
   INST int low ()  const { return base; }
   INST int size () const { return arraySize; }
   // read, write, display need MLCstream routines for reading/writing
   //  the object in which may not always exist, so they are not INST
   void read_bin(MLCIStream& str);
   void write_bin(MLCOStream& str);
   void display(MLCOStream& stream = Mcout) const;
   Array<Element>& operator +=(const Array<Element>& item);
   Array<Element>& operator -=(const Array<Element>& item);

   // CFront 3 chokes if these are moved outside the class
   // and defined as inline (it generates static function undefined).
   Element& index(int i) {
#     ifndef FAST
      DBG(if (i < 0 || i >= arraySize)
	  err<<"Array<>::index: reference " << i <<
	  " is out of bounds (0.." << arraySize - 1 << ')'
	  << fatal_error); 
#     endif
      return elements[i];
   }

   const Element& index(int i) const {
#     ifndef FAST
      DBG(if (i < 0 || i >= arraySize)
	  err<<"Array<>::index() const:  reference " << i <<
	  " is out of bounds (0.." << arraySize - 1 << ')'
	  << fatal_error); 
#     endif
  
      return elements[i];
   }

   Element& operator[](int i) {
#     ifndef FAST
      DBG(if (i < base || i > base + arraySize - 1)
	  err<<"Array<>::operator[]: reference " << i <<
	  " is out of bounds (" << base << ".." << high() << ')'
	  << fatal_error); 
#     endif
      return elements[i - base];
   }

   inline const Element& operator[](int i) const {
#     ifndef FAST
      DBG(if (i < base || i > base + arraySize - 1)
	  err<<"Array<>::operator[] const: reference " << i <<
	  " is out of bounds (" << base << ".." << high() << ')'
	  << fatal_error); 
#     endif
  
      return elements[i - base];
   }

   // sort, min/max need operator<, so they are not INST
   void sort();
   const Element& max(int& idx) const;  // idx is zero based index to array
   const Element& max() const;
   const Element& min(int& idx) const;  // idx is zero based index to array
   const Element& min() const;
   void  min_max(const Element& min, const Element& max) const;

   int find(const Element& key, int startPos=0) const;
};

/***************************************************************************
    Description :  The next two functions return a reference and a const
                     reference, respectively, to the correct element
		     referenced from a ZERO BASE.
		   The two functions after return the element with respect
		     to the true base.
    Comments    :  Out of bounds reference is a fatal error.
***************************************************************************/

template <class Element>
class PtrArray: public Array<Element> {
   NO_COPY_CTOR(PtrArray);
public:
   // Operator= aborts by default.  Use Array::operator= if the class
   // has a well defined operator= for its element (rarely because it's
   // usually class* that's used in the template)
   Array<Element>& operator= (const Array<Element>& source);
   PtrArray(int base, int size, Bool init=TRUE);
   PtrArray(int size); // initializes to NULL
   ~PtrArray();          
   void display(MLCOStream& stream = Mcout) const;
   void sort();
};


// Template functions
template <class Element> 
Bool operator==(const Array<Element>& a1, const Array<Element>& a2);

template <class Element> 
Bool operator!=(const Array<Element>& a1, const Array<Element>& a2);

template <class Element> 
Bool operator==(const PtrArray<Element>& a1, const PtrArray<Element>& a2);

template <class Element> 
Bool operator!=(const PtrArray<Element>& a1, const PtrArray<Element>& a2);

template <class Element>
DECLARE_DISPLAY(Array<Element>);

template <class Element>
DECLARE_DISPLAY(PtrArray<Element>);

#endif





