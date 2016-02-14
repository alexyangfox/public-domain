// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : 2d Array template with error checking.  Can be initialized
                   with either with simply the number of rows and columns
		   or the number of rows and columns given a starting
		   row and column.  In either case, there also exists an
		   optional final initialization argument for initializing
		   the array to an initial value.
		 Access to elements is either by operator(), or by
		   index() which ranges form 0 to size-1.
		 Provides operator= for copying an Array2, if the
		   Array2's are the same size.
		 Derived Class PtrArray2 is an Array2 of pointers, which,
		   when its destructor is called, calls delete on
		   each of the pointers.
		 The default initialization is to NULL, but via the 3-argument
		   constructor, the user can turn off this O(numRows*numCols)
		   operation.
  Assumptions  : 
  Comments     : Index is not used in some routines because profiling shows
                   that they took huge amounts of time.  We access elements
		   directly.
		 For PtrArray2, because of the destructor, all of the pointers
		   should be pointing to either 1) a piece of memory that
		   can be freed, or 2) NULL.  Furthermore, to help prevent
		   errors, the user is not allowed to initialize the
		   Array2 with anything except for NULL.  
  Complexity   : Initializing an Array2 takes O(num-elements), as does the
                   destructor and free() routines.
  Enhancements : Display() should show a 2-D display instead of the string
                   it currently displays.
  History      : Dave Manley                                       9/26/93
                   Initial revision
***************************************************************************/
#include <basics.h>

// Template files shouldn't have RCSID
// RCSID("MLC++, $RCSfile: Array2.c,v $ $Revision: 1.21 $")


/***************************************************************************
    Description :  creates space for a newly constructed Array2
    Comments    :  
***************************************************************************/
template <class Element>   
void Array2<Element>::alloc_array(int startR, int startC,
				 int numR,int numC) 
{
   if (numR < 0 || numC < 0)
      err << "Array2<>::Illegal bounds requested, starting row: " << startR <<
	 " starting column: " << startC << fatal_error;
   startRow = startR;
   startCol = startC;
   numRows = numR;
   numCols = numC;
   elements = new Element[numR * numC];
}


/***************************************************************************
    Description :  creates space for new Array2 and assigns its values
    Comments    :  O(num-elements) time, assumes no need to free Array2
***************************************************************************/
template <class Element>   
void Array2<Element>::copy(const Array2<Element>& source) 
{  
   alloc_array(source.startRow, source.startCol, source.numRows,
	       source.numCols);
   for (int i = 0; i < size(); i++)
      elements[i] = source.elements[i];
}


/***************************************************************************
    Description :  Constructor which takes number of Rows and columns.
    Comments    :  Creates Array2 where rows and columns start from 0.
***************************************************************************/
template <class Element> 
Array2<Element>::Array2(int numRows, int numCols)
{
   alloc_array(0,0,numRows,numCols);
}


/***************************************************************************
    Description :  Constructor which takes number of Rows and columns.
                   Initializes elements to given initial value.
    Comments    :  Creates Array2 where rows and columns start from 0.
                   Takes O(num-elements) time.
***************************************************************************/
template <class Element> 
Array2<Element>::Array2(int numRows, int numCols,
			const Element& initialValue)
{
   alloc_array(0,0,numRows,numCols);
   init_values(initialValue);
}

/***************************************************************************
    Description :  Constructor which takes starting Row, starting Column,
                     and number of following rows and columns.
    Comments    :  
***************************************************************************/
template <class Element> 
Array2<Element>::Array2(int startRow, int startCol,
			int numRows, int numCols)
{
   alloc_array(startRow, startCol, numRows, numCols);
}


/***************************************************************************
    Description :  Constructor which takes starting Row, starting Column,
                     and number of following rows and columns.
		   Initializes elements to given initial value.
    Comments    :  Takes O(num-elements) time.
***************************************************************************/
template <class Element> 
Array2<Element>::Array2(int startRow, int startCol,
			int numRows, int numCols,
			const Element& initialValue)
{
   alloc_array(startRow, startCol, numRows, numCols);
   init_values(initialValue);
}


/***************************************************************************
    Description :  Copy constructor which takes an extra argument.
    Comments    :  We don't use the standard issue copy constructor because
                     we want to know exactly when we call the copy constructor.
***************************************************************************/
template <class Element> 
Array2<Element>::Array2(const Array2<Element>& source,
			CtorDummy /*dummyArg*/)
{
   copy(source);
}


/***************************************************************************
    Description :  copies values from elem.
    Comments    :  Runs in O(num-elements) time.
                   Arrays must be the same size.
***************************************************************************/
template <class Element> 
Array2<Element>& Array2<Element>::operator=(const Array2<Element>& elem) 
{
   DBG(if (elem.size() != size())
       err<<"Array2<>::operator=: Cannot assign array sized: " <<
       elem.size()<< " to an array sized: " << size() << fatal_error);
   for (int i = 0; i < size(); i++)
      elements[i] = elem.elements[i];
   return *this;
}

/***************************************************************************
  Description : Compares two arrays.  Must have the same size to be equal.
  Comments    :
***************************************************************************/

template <class Element>
Bool Array2<Element>::operator==(const Array2<Element>& array2) const
{
   if (size() != array2.size())
      return FALSE;

   for (int i = 0; i < size(); i++)
      if (elements[i] != array2.elements[i])
	 return FALSE;
   
   return TRUE;
}
   

/***************************************************************************
    Description :  Initializes all elements of an Array2 to the initialValue
    Comments    :  Runs in O(num-elements) time
***************************************************************************/
template <class Element> 
void Array2<Element>::init_values(const Element& initialValue) 
{
  // walk through the Array2 and initialize all of the values
  int arraySize = size(); // Profiling shows init is critical
  for (int i = 0; i < arraySize; i++)
     elements[i] = initialValue;
}


/***************************************************************************
    Description :  The next two functions return a reference and a const
                     reference, respectively, to the correct element
		     referenced from a zero base. 
                   
    Comments    :  Out of bounds reference is a fatal error.
***************************************************************************/
template <class Element> 
Element& Array2<Element>::index(int i)
{
   DBG(if (i < 0 || i >= size())
       err<<"Array2<>::index: reference " << i <<
       " is out of bounds (" << 0 << ".." << size() - 1 << ')'
       << fatal_error); 
  
   return elements[i];
}

template <class Element> 
const Element& Array2<Element>::index(int i) const
{
   DBG(if (i < 0 || i >= size())
       err<<"Array2<>::index() const:  reference " << i <<
       " is out of bounds (" << 0 << ".." << size() - 1 << ')'
       << fatal_error); 
  
   return elements[i];
}


/***************************************************************************
    Description :  The next two functions return a reference and a const
                     reference, respectively, to the element referenced.
    Comments    :  Out of bounds reference is a fatal error.
***************************************************************************/

template <class Element> 
Element& Array2<Element>::operator()(int row, int col)
{
   DBG(if (row < startRow || row >= numRows + startRow)
       err<<"Array2<>::operator(): row reference " << row <<
       " is out of bounds (" << startRow << ".." << startRow + numRows - 1
       << ')' << fatal_error);

   
   DBG(if (col < startCol || col >= numCols +startCol)
       err<<"Array2<>::operator(): column reference " << col <<
       " is out of bounds (" << startCol << ".." << startCol + numCols - 1
       << ')' << fatal_error); 
  
   return elements[(row-startRow)*numCols + (col-startCol)];
}

template <class Element> 
const Element& Array2<Element>::operator()(int row, int col) const
{
   DBG(if (row < startRow || row >= numRows + startRow)
       err<<"Array2<>::operator() const: row reference " << row <<
       " is out of bounds (" << startRow << ".." << startRow + numRows - 1
       << ')' << fatal_error);

   
   DBG(if (col < startCol || col >= numCols + startCol)
       err<<"Array2<>::operator() const: column reference " << col <<
       " is out of bounds (" << startCol << ".." << startCol + numCols - 1
       << ')' << fatal_error); 
  
   return elements[(row-startRow)*numCols + (col-startCol)];
}


/***************************************************************************
  Description : Output array to an MLCStream
  Comments    :
***************************************************************************/
template<class Element>
void Array2<Element>::display(MLCOStream& stream) const
{
   for (int i = 0; i < size() - 1; i++)
      stream <<  index(i) << ", ";

   if (size() > 0)
      stream << index(i);  // no trailing comma.
}

/***************************************************************************
    Description :  Two constructors for PtrArray2.
                   Initializes all the pointers in the specified Array2 by
                     default or if the user passes in true for init.  (Leaves
		     unitialized if the user specifies false).
    Comments    :  Takes O(num-elements) time if the user wishes to
                     initialize the Array2.
		   Cannot use init_vals because it takes a reference.
***************************************************************************/
template <class Element> 
PtrArray2<Element>::PtrArray2(int numRows, int numCols,
			      Bool init)
                  :Array2<Element>(numRows, numCols)
{
   if (init == TRUE)
      for (int i = 0; i < size(); i++)
	 elements[i] = NULL;
}

template <class Element> 
PtrArray2<Element>::PtrArray2(int startRow, int startCol,
			      int numRows, int numCols,
			      Bool init)
                  :Array2<Element>(startRow, startCol, numRows, numCols)
{
   if (init == TRUE)
      for (int i = 0; i < size(); i++)
	 elements[i] = NULL;
}


/***************************************************************************
    Description :  Calls delete for all of the Pointers in the Array2.
    Comments    :  Takes O(num-elements) time.
***************************************************************************/
template <class Element> 
PtrArray2<Element>::~PtrArray2()
{
   for (int i = 0; i < size(); i++)
      delete elements[i];
}

/***************************************************************************
    Description :  Assigning to a PtrArray2 causes a fatal error.
    Comments    :  The reasoning for this: Assignment will cause two arrays
                     to point to the same element.  When the
		     destructor is called, elements are freed twice.
***************************************************************************/
template <class Element> 
Array2<Element>& PtrArray2<Element>::operator=(const Array2<Element>&
						  /*elem*/) 
{
   err<<"PtrArray2<>::operator=: Cannot assign to a PtrArray2" << fatal_error;
   return (*this);
}

template <class Element>
DEF_DISPLAY(Array2<Element>)


