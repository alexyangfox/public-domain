// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Array template with error checking.  Can be initialized with
                   a lower bound & size, or the size only, in which
  		   case zero is the assumed lower bound.  Optional final
		   initialization argument for lower & upper bound is an
		   initial value.
		 Access to elements is either by operator[], or by
		   index which ranges form 0 to size-1.
		 Derived Class PtrArray is an array of pointers, which,
		   when its destructor is called, calls delete on
		   each of the pointers. The default initialization is
		   to NULL, but via the 3-argument constructor, the user
		   can turn off this O(n) operation.
		 PtrArray should be used when you do NOT need to perform
		   copies and operator= which are not defined (to avoid
		   problems with shallow/deep copies using pointers).
		   Note that since we get the template with the pointer, we
		   cannot even create (new) the class. 
  Assumptions  : If no initialization is given, operator= must work
                   assuming that the left side was initialized using
		   the default constructor.  It is an error to
		   use an Array of a structure that contains an MString
		   without proper initialiation of the structure, i.e.,
		   a default constructor that initializes the MString.
		   The reason is that the new elements contain garbage
		   and when operator= is invoke the garbage is "released."
  Comments     : Index is not used in some routines because profiling shows
                   that they took huge amounts of time.  We access elements
		   directly.
		 As a result of the destructor for PtrArray, all of the
		   pointers should be pointing to either 1) a piece of
		   memory that can be freed, or 2) NULL.  Furthermore,
		   to help prevent errors, the user is not allowed to
		   initialize the array with anything except for NULL.
                 The sorting of Arrays turns out to be complex.  We do not
                   want to force classes to have operator<, and the obvious
                   solution which is to create a static function to pass to
                   qsort fails, because static functions are always
                   instantiated.   The solution used here it to create a
                   templated function sort_compare, and take its address in
                   the sort member function.  A cast must be used to cast
                   it to a function taking two voids.  See description in
                   sort().
		 The fact that PtrArray gets an argument with the pointer
		   is a deficiency, since we can't do new.  The reason for
		   this is typedefs.  For example, in LEDA, a "node" is a
		   typedef for a pointer to something that's suppose to be
		   hidden.  It is not clear whether the design should be
		   replaced, or if we should allow a PtrArray that supports
		   copying and operator=.  Both of these could easily be done
		   using Array and doing the appropriate copy operation of
		   pointers in the encapsulating class.
  Complexity   : Initializing an array takes O(num-elements).
                 sort() is done using the system's qsort, which should
                   take O(n^2) in the worst case, but O(n*logn) on
                   average.
  Enhancements : Add reference counting.  This is especially critical
                   in NaiveBayesian classifier which passes arrays
		   from the inducer to the categorizer, and thus
		   could be made fast and incremental if we had
		   reference counting.
                 In many cases, both min() and max() are needed.
                 In such cases we could save some comparisons by
                   computing them together.  Not only would this
                   iterate once through the array, but we could do
                   this trick of comparing pairs of elements in the
                   array.  Then the higher is compared to max and the
                   lower to min.  This saves .5n comparisons.
		   Added read_bin and write_bin methods to save Array
		   in binary format, this is why MLCStream is
		   included.
		   
  History      : James Dougherty                                  12/25/94
                   Made array more powerful with operators +=/-=
                 Ronny Kohavi and James Dougherty                  5/15/94
                   Added sort, probably the most complex cast in MLC++ 
                 James Dougherty                                 05/12/94
		   Added read_bin, write_bin methods, sort methods.
                 Dave Manley                                       9/9/93
                   Initial revision
                 Ronny Kohavi                                      1/9/93
                   operator==, operator!=, <<, min, max.

***************************************************************************/
#include <basics.h>
#include <MLCStream.h>

// The following line is removed because id gets declared twice.
// RCSID("MLC++, $RCSfile: Array.c,v $ $Revision: 1.50 $")

/***************************************************************************
    Description :  creates space for a newly constructed array
    Comments    :  
***************************************************************************/
template <class Element>   
void Array<Element>::alloc_array(int lowVal, int sizeVal) 
{
   if (sizeVal < 0)
      err << "Array<>::Illegal bounds requested, base: " << lowVal <<
	 " high: " << lowVal + sizeVal << fatal_error;
   base = lowVal;
   arraySize = sizeVal;
   elements = new Element[::max(1,arraySize)];
}


/***************************************************************************
    Description :  creates space for new array and assigns its values
    Comments    :  O(num-elements) time, assumes no need to free array
***************************************************************************/
template <class Element>   
void Array<Element>::copy(const Array<Element>& source) 
{  
   alloc_array(source.base, source.arraySize);
   for (int i = 0; i < arraySize; i++)
      elements[i] = source.elements[i];
}


/***************************************************************************
    Description :  Copy constructor which takes an extra argument.
    Comments    :  We don't use the standard issue copy constructor because
                     we want to know exactly when we call the copy constructor.
***************************************************************************/
template <class Element> 
Array<Element>::Array(const Array<Element>& source,
		      CtorDummy /*dummyArg*/)
{
   copy(source);
}


/***************************************************************************
    Description :  Constructor which takes a base and size.
    Comments    :  Note that because we provide this constructor, we
                     can't make size and initial value constructor because
		     it will conflict with this constructor when
		     the template is instantiated with an integer.
***************************************************************************/
template <class Element> 
Array<Element>::Array(int base, int size)
{
   alloc_array(base, size);
}


/***************************************************************************
    Description :  Constructor which takes a size.
    Comments    :  Indexed from zero.
***************************************************************************/
template <class Element> 
Array<Element>::Array(int size)
{
   alloc_array(0, size);
}


/***************************************************************************
    Description :  Constructor which takes a low value, size and initial
                   value.  Initializes all elements to the initial value.
    Comments    :  Takes O(num-elements) time.
***************************************************************************/
template <class Element> 
Array<Element>::Array(int base, int size,
		      const Element& initialValue) 
{
   alloc_array(base,size);
   init_values(initialValue);
}


/***************************************************************************
    Description :  Copies values from source.
    Comments    :  Runs in O(num-elements) time.
                   Arrays must be the same size.
***************************************************************************/
template <class Element> 
Array<Element>& Array<Element>::operator=(const Array<Element>& elem) 
{  
   DBG(if (elem.size() != size())
       err<<"Array<>::operator=: Cannot assign array sized: " <<
       elem.size()<< " to an array sized: " << size() << fatal_error);
   for (int i = 0; i < arraySize; i++)
      elements[i] = elem.elements[i];
   return *this;
}


/***************************************************************************
    Description :  Initializes all elements of an array to the initialValue
    Comments    :  Runs in O(num-elements) time
***************************************************************************/
template <class Element> 
void Array<Element>::init_values(const Element& initialValue) 
{
  // walk through the array and initialize all of the values
  for (int i = 0; i < arraySize; i++)
     elements[i] = initialValue;
}

/***************************************************************************
  Description : Reads in a float array stored as a sequence of byte values
  Comments    : Array<float> passed by reference, ignore warning message.
               
***************************************************************************/
template<class Element>
void Array<Element>::read_bin(MLCIStream& stream)
{
   for (int i = 0; i < size(); i++)
      stream.read_bin(index(i));
}

/***************************************************************************
  Description : Writes out an Array<float> as a sequence of byte values
  Comments    : Array<float> value passed by reference, ignore warning message
**************************************************************************/
template<class Element>
void Array<Element>::write_bin(MLCOStream& stream)
{
   for (int i = 0; i < size(); i++)
      stream.write_bin(index(i));
}

/***************************************************************************
  Description : Sorts the items in the array using C qsort() function.
  Comments    : Overridden by PtrArray, but NOT virtual.  The reason is
                  that virtual functions are always instantiated, and we
                  do not want to force operator< to be defined.
                We take the address of sort_compare which is a templated
                  function, and then cast it to one that takes two voids.
                  It is important to get the pointer to the function with
                  the actual type, THEN cast, because if we get a pointer
                  to a function taking two voids, it will not use the
                  class's operator<.
                Any changes here must be made in PtrArray::sort().
**************************************************************************/

template<class Element>
void Array<Element>::sort()
{
   // Declare sortFunc as a pointer to one of the overloaded
   // sort_compare functions (template functions) which have our
   // desired type.
   int (*sortFunc)(const Element *a, const Element *b) = &sort_compare;

   // Now cast the function into one that takes two voids.
   int (*voidSortFunc)(const void *a, const void *b) =
          (int (*)(const void*, const void*))sortFunc;
   qsort(elements, size(), sizeof(*elements), voidSortFunc);
   DBG(for (int i = 0; i < size()-1; i++)
          if (! (index(i) < index(i+1) || index(i) == index(i+1)))
	     err << "Array<Element>::sort: sort error at index " << i <<
                    ".\n  Most probably cause is that operator< and operator="
                    " are inconsistent" << fatal_error);
}


/***************************************************************************
  Description : Min/max operations.  min() and max() return a reference
                   to the element in the array.
  Comments    : Array must not be empty.
                Tie breaker during equality prefer earlier elements in
                  the array.
***************************************************************************/
template <class Element> 
const Element& Array<Element>::max(int& idx) const
{
   if (arraySize == 0)
      err << "Array<Element>::max() - empty array" << fatal_error;

   const Element* max = &index(0);
   idx = 0;
   for (int i = 1; i < arraySize; i++)
      if (index(i) > *max) {
         max = &index(i);
         idx = i;
      }
   return *max;
}

template <class Element> 
const Element& Array<Element>::max() const
{
   int dummy;
   return max(dummy);
}


template <class Element> 
const Element& Array<Element>::min(int& idx) const
{
   if (arraySize == 0)
      err << "Array<Element>::min() - empty array" << fatal_error;

   const Element* min = &index(0);
   idx = 0;
   for (int i = 1; i < arraySize; i++)
      if (index(i) < *min) {
         min = &index(i);
         idx = i;
      }

   return *min;
}

template <class Element> 
const Element& Array<Element>::min() const
{
   int dummy;
   return min(dummy);
}

/***************************************************************************
  Description : Output array to an MLCStream
  Comments    :
***************************************************************************/
template<class Element>
void Array<Element>::display(MLCOStream& stream) const
{
   for (int i = 0; i < size() - 1; i++)
      stream <<  index(i) << ", ";

   if (size() > 0)
      stream << index(i);  // no trailing comma.
}


/***************************************************************************
  Description : Element-wise addition/subtraction.  Modifies the array so that
                  each value is incremented/decremented by the amount
		  contained in the array passed as an actual.
		  If B is an array, and A is an array of the same type and
		  size, A+=B is the operation defined
		  as A = (a[0]+-b[0],a[1]+-b[1], ..., a[n]+-b[n]) where n is
		  the size() of the array.
  Comments    :
***************************************************************************/
template <class Element>
Array<Element>& Array<Element>::operator+=(const Array<Element>& array)
{
   if (size() != array.size())
      err << "Array<Element>::operator +=: sizes of array do not match"
	  << " attempting to add an array of size " << array.size() + 1
	  << " when array has only " << size() << " elements."
	  << fatal_error;
   for(int i = 0; i < arraySize; i++)
      elements[i] += array.elements[i];
   return *this;
}

template <class Element>
Array<Element>& Array<Element>::operator-=(const Array<Element>& array)
{
   if (size() != array.size())
      err << "Array<Element>::operator -=: sizes of array do not match"
	  << " attempting to add an array of size " << array.size() + 1
	  << " when array has only " << size() << " elements."
	  << fatal_error;
   for(int i = 0; i < arraySize; i++)
      elements[i] -= array.elements[i];
   return *this;
}




/***************************************************************************
    Description :  Initializes all the pointers in the specified array by
                     default and if the user passes in true for init.
		     Either way it sets up an array starting at base of
		     the specified size.
    Comments    :  Takes O(num-elements) time if the user wishes to
                     initialize the array.
***************************************************************************/
template <class Element> 
PtrArray<Element>::PtrArray(int base, int size, Bool init)
                  :Array<Element>(base, size)
{
   if (init == TRUE)
      for (int i = 0; i < size; i++)
	 elements[i] = NULL;
}

template <class Element> 
PtrArray<Element>::PtrArray(int size) : Array<Element>(size)
{
   for (int i = 0; i < size; i++)
      elements[i] = NULL;
}



/***************************************************************************
    Description :  Calls delete for all of the Pointers in the array.
    Comments    :  Takes O(num-elements) time.
***************************************************************************/
template <class Element> 
PtrArray<Element>::~PtrArray()
{
   for (int i = 0; i < arraySize; i++)
      delete (elements[i]);
}


/***************************************************************************
    Description :  Assigning to a PtrArray causes a fatal error.
    Comments    :  The reasoning for this: Assignment will cause two arrays
                     to point to the same element.  When the
		     destructor is called, elements are freed twice.
***************************************************************************/
template <class Element> 
Array<Element>& PtrArray<Element>::operator=(const Array<Element>&
						/*elem*/) 
{
   err<<"PtrArray<>::operator=: Cannot assign to a PtrArray" << fatal_error;
   return (*this);
}


/***************************************************************************
  Description : Output a pointer array to an MLCStream by outputing to
                  that which each element points.
                PtrArray is assumed to be a pointer to something, i.e.,
                  operator* must be defined.
  Comments    : For NULL values, the string "NULL" is printed.
***************************************************************************/

template<class Element>
void PtrArray<Element>::display(MLCOStream& stream) const
{
   for (int i = 0; i < size() - 1; i++)
     if (index(i))
        stream << *index(i) << ", ";
     else 
        stream << "NULL, ";
   
   if (size() > 0)  // avoid trailing comma
     if (index(i))
        stream << *index(i);
     else 
        stream <<  "NULL";
}

/***************************************************************************
    Description :  Compares the two arrays element-by-element
                   Returns FALSE for arrays of different sizes.
    Comments    :  Runs in O(num-elements) time.
***************************************************************************/
template <class Element> 
Bool operator==(const Array<Element>& a1, const Array<Element>& a2)
{  
   if(a1.size() != a2.size())
      return FALSE;

   for (int i = 0; i < a1.size(); i++)
      if (a1.index(i) != a2.index(i))
         return FALSE;

   return TRUE;
}

template <class Element> 
Bool operator!=(const Array<Element>& a1, const Array<Element>& a2)
{  
   DBG(if (a1.size() != a2.size())
       err<<"operator!=(Array,Array): Arrays of different size: " <<
       a1.size() << " vs. " << a2.size() << fatal_error);

   return !(a1 == a2);
}


template <class Element>
DEF_DISPLAY(Array<Element>)

template <class Element>
DEF_DISPLAY(PtrArray<Element>)

/***************************************************************************
  Description : Sorts the items in the array using C qsort() function.
  Comments    : See Array::sort()
 ***************************************************************************/
template <class Element>
void PtrArray<Element>::sort()
{
   int (*sortFunc)(const Element **a, const Element **b) = &sort_ptr_compare;
   // Now cast the function into one that takes two voids.
   int (*voidSortFunc)(const void *a, const void *b) =
          (int (*)(const void*, const void*))sortFunc;
   qsort(elements, size(), sizeof(**elements), voidSortFunc);
}



/***************************************************************************
    Description :  Compares the two ptr arrays element-by-element
    Comments    :  Runs in O(num-elements) times operator==() for Element.
                   Ptr Arrays must be the same size.
***************************************************************************/
template <class Element> 
Bool operator==(const PtrArray<Element>& a1, const PtrArray<Element>& a2)
{  
   DBG(if (a1.size() != a2.size())
       err<<"operator==(PtrArray, PtrArray): Arrays of different size: " <<
       a1.size() << " vs. " << a2.size() << fatal_error);

   for (int i = 0; i < a1.size(); i++)
      if (*a1.index(i) != *a2.index(i))
         return FALSE;

   return TRUE;
}



template <class Element> 
Bool operator!=(const PtrArray<Element>& a1, const PtrArray<Element>& a2)
{  
   DBG(if (a1.size() != a2.size())
       err<<"operator!=(PtrArray, PtrArray): Arrays of different size: " <<
       a1.size() << " vs. " << a2.size() << fatal_error);

   return !(a1 == a2);
}
