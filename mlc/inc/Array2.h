// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef Array2_h
#define Array2_h

#include <stddef.h>
#include <basics.h>

// INST forces instantiation of the function to avoid duplicate template
// instantiations 
#ifdef FASTINLINE
#  define INST
#else
#  define INST virtual
#endif

template <class Element>
class Array2 {
   NO_COPY_CTOR(Array2);
protected:  // Protected in case we want to derive a DynamicArray2
            // from Array2.
   int numRows, numCols, startRow, startCol; 
   Element *elements;

   INST void alloc_array(int startRow, int startCol, 
			 int numRows, int numCols); 
   INST void free() { delete [] elements; }
   INST void copy(const Array2& source); // used by copy constructor
public:
   Array2(const Array2& source, CtorDummy);
   Array2(int numRows, int numCols);
   Array2(int numRows, int numCols, const Element& initialValue);
   Array2(int startRow, int startCol, int numRows,
	  int numCols);
   Array2(int startRow, int startCol, int numRows,
	  int numCols, const Element& initialValue);
   virtual ~Array2() { free(); }
   Array2& operator= (const Array2& source);
   Bool operator==(const Array2& array2) const;
   Bool operator!=(const Array2& array2) const {
       return !operator==(array2);}
   INST void init_values(const Element& initalValue);
   INST int num_rows () const { return numRows;} 
   INST int num_cols ()  const { return numCols; }
   INST int start_row () const { return startRow; }
   INST int start_col () const { return startCol; }
   INST int high_row() const { return startRow + numRows - 1; }
   INST int high_col() const { return startCol + numCols - 1; }
   INST int size () const { return numRows*numCols; }
   INST Element& index(int i);
   INST const Element& index(int i) const;
   INST Element& operator()(int row,int col);
   INST const Element& operator()(int row, int col) const;
   void display(MLCOStream& stream = Mcout) const;
};

template <class Element>
class PtrArray2: public Array2<Element> {
   NO_COPY_CTOR2(PtrArray2,Array2<Element>(1, 1));
public:
   PtrArray2(int numRows, int numCols, Bool init=TRUE);
   PtrArray2(int startRow, int startCol, int numRows,
	     int numCols, Bool init=TRUE);
   ~PtrArray2();          
   // Disabled by default
   INST Array2<Element>& operator= (const Array2<Element>& source);
};

template <class Element>
DECLARE_DISPLAY(Array2<Element>);

template <class Element>
DECLARE_DISPLAY(PtrArray2<Element>);

#endif

