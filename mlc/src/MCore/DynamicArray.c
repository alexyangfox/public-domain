// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/******************************************************************************
  Description  : Implements dynamic array derived from Array<>.
                 DynamicArray may grow when using operator=(),
		   operator[], and index() for non-const object.
		   DynamicArray may shrink to the size of the source
		   array when using operator=() for non const object.
		 DynamicArray may not grow or shrink for const object.
		   An attempt to try to read beyond the current
		   allocated range  by using operator[] or index() is
		   regarded as a fatal error.
		   The reason for this is that by allowing these operators to
		   extend the array, we are allowing a const object to be
		   changed, which contradicts conceptual constness (not to
		   mention bitwise constness).  For example, the size of the
		   array could change even if we call a function taking a
		   const array.
		 size() returns the largest index, ever been
		   accessed either for read or for write, plus one.
		 Users are advised to try to extend array as large as
		   possible at his/her first try, which will reduce
		   additional unnecessary operations otherwise incurred.
  Assumptions  :
  Comments     : 
  Enhancements : 
  History      : Yeogirl Yun                                   5/03/94
                   Initial revision(.c .h)
******************************************************************************/

#include <basics.h>
#include <DynamicArray.h>

const int DEFAULT_INCR_VAL = -1;

/******************************************************************************
  Description  : Increase sizeAllocated  according to incrementSize
                   data member. 
  Comments     : This is a protected member function.
******************************************************************************/
template <class Element>
int DynamicArray<Element>::increase_size()
{
   if( incrementSize == -1 ) 
      sizeAllocated = ::max( 1, sizeAllocated*2 );
   else
      sizeAllocated += incrementSize;
   return sizeAllocated;
}


/******************************************************************************
  Description  : Extends the current array to be larger than
                    indexRequested which is the index requested by
		    index() or operator[] methods.
  Comments     : This is a proctected member function.
******************************************************************************/
template <class Element>
void DynamicArray<Element>::extend_array(int indexRequested)
{
   // increase sizeAllocated member until it gets
   // larger than indexRequested.
   while (increase_size() <= indexRequested)
      ; /* NULL */

   Element* tempBuf = new Element[::max(1,sizeAllocated)];
   for (int i = 0; i < arraySize; i++)
      tempBuf[i] = elements[i];
   // initialize up to indexRequested if wasInitialized is on
   if( wasInitialized )
      for( ; i <= indexRequested; i++ )
	 tempBuf[i] = initialValue;
   
   // set the current array size
   arraySize = indexRequested + 1;
   delete [] elements;
   elements = tempBuf;
}

/******************************************************************************
  Description  : Copy constructor which takes a dummy argument.
  Comments     : 
******************************************************************************/
template <class Element>
DynamicArray<Element>::DynamicArray(const DynamicArray<Element>& source,
				    CtorDummy dummyArg )
: Array<Element>(source, dummyArg)
{
   incrementSize = DEFAULT_INCR_VAL;
   wasInitialized = source.wasInitialized;
   sizeAllocated = source.size(); // NOT sizeAllocated because we are only
				  // copying the real size
   if(wasInitialized)
      initialValue = source.initialValue;
}




/******************************************************************************
  Description  : Constructor which tkaes a base, a size, and an
                   initial value arguments.
  Comments     : 
******************************************************************************/
template <class Element>
DynamicArray<Element>::DynamicArray(int base, int size,
				    const Element& initialVal)
: Array<Element>(base,size,initialVal)
{
   incrementSize = DEFAULT_INCR_VAL;
   wasInitialized = TRUE;
   initialValue = initialVal;
   sizeAllocated = size;
}

/******************************************************************************
  Description  : Constructor which takes a size and an initial value arguments.
               : Base is assumed to be zero.
  Comments     :
******************************************************************************/
template <class Element>
DynamicArray<Element>::DynamicArray(int size, const Element& initialVal)
: Array<Element>(0,size,initialVal)
{
   initialValue = initialVal;
   wasInitialized = TRUE;
   incrementSize = DEFAULT_INCR_VAL;
   sizeAllocated = size;
}


/******************************************************************************
  Description  : Constructor which takes a size argument.
                 Base is assumed to be zero.
  Comments     :
******************************************************************************/
template <class Element>
DynamicArray<Element>::DynamicArray(int size)
: Array<Element>(size)
{
   incrementSize = DEFAULT_INCR_VAL;
   sizeAllocated = size;
   wasInitialized = FALSE;
   
}

/***************************************************************************
  Description : Set the increment size for the dynamic array to grow.
                -1 means double every time, which has good properties if you
  		  amortize costs.  For example, a series of sequential
		  inserts will have O(1) time complexity per insert, because
		  each extension means n/2 new elements have been inserted,
		  and charging 2 extra operations for each, means we have
		  deposited money for n operations (to move everything when we
		  extend the array).
  Comments    :
***************************************************************************/

template <class Element>
void DynamicArray<Element>::set_increment_size(int incrSize)
{
   if( incrSize <= 0 && incrSize != -1 )
      err << "DynamicArray<>::set_increment_size : illegal"
	     " increment size argument : " << incrSize << fatal_error;
   incrementSize = incrSize;
}


/******************************************************************************
  Description  : Implements assignment operation with dynamic feature.
                 If the size of source array is different from that of
                   current array, the size of the current array is
		   adapted to that size. After this operation, the
		   sizes of two arrays are the same.
  Comments     :                 
******************************************************************************/

template <class Element>
DynamicArray<Element>& DynamicArray<Element>::operator=
                                         (const DynamicArray<Element>& source)
{
   if( this != &source ) {
      delete [] elements;
      alloc_array(source.low(), source.size());
      sizeAllocated = source.size();
      for(int i = 0; i < source.size(); i++)
	 elements[i] = source.index(i);
   }
   return *this;
}


/******************************************************************************
  Description  : Returns a reference to the element indexed from the
                   base as if it is 0. When index is bigger than the
		   current upper bound, the current array is extended
		   according to incrementSize data member.
  Comments     :
******************************************************************************/

template <class Element>
Element& DynamicArray<Element>::index(int i)
{
   if( i < arraySize ); // empty statement
   else if( i < sizeAllocated ) { 
      // need to update arraySize and do initialization
      if(wasInitialized)
	 for( int j = arraySize; j <= i; j++ )
	    elements[j] = initialValue;
      arraySize = i + 1;
   }
   else
      extend_array(i);      

   return Array<Element>::index(i);
}



/******************************************************************************
  Description  : Returns a reference to the element index from the base.
                   When index is bigger than the current upper bound,
		   the current array is extended according to
		   incrementSize data member.
  Comments     :
******************************************************************************/


template <class Element>
Element& DynamicArray<Element>::operator[](int i)
{
   if( i - base < arraySize ); // empty statement 
   else if( i - base < sizeAllocated ) { 
      // need to update arraySize and do initialization
      if(wasInitialized)
	 for( int j = arraySize - base; j <= i - base; j++ )
	    elements[j] = initialValue;
      arraySize = i - base + 1;
   }
   else
      extend_array(i - base);
   
   return Array<Element>::operator[](i);
}


/******************************************************************************
  Description  : Appends elements from the specified array onto the end
                   of this dynamic array.
  Comments     :
******************************************************************************/
template <class Element>
void DynamicArray<Element>::append(const Array<Element>& other)
{
   for(int otherElem = 0; otherElem < other.size(); otherElem++)
      index(size()) = other.index(otherElem);
}


/*****************************************************************************
  Description : Truncates the size of the DynamicArray as if it has grown
                  to the truncated size.
  Comments    : 
*****************************************************************************/
template <class Element>
void DynamicArray<Element>::truncate(int sizeVal)
{
   if (sizeVal < 0 || sizeVal > arraySize)
      err << "DynamicArray<>::truncate: Illegal truncated size : " << sizeVal
	 << ". The current size is " << arraySize << fatal_error;
   
   arraySize = sizeVal;
}




