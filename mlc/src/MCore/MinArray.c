// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/******************************************************************************
  Description  : MinArray<> is derived from Array<> and contains the
                    kth smallest data items at most.
  Assumptions  : 
  Comments     : 
  Complexity   : Insert() takes O(k).
  Enhancements : 
  History      : Yeogirl Yun, Ronny Kohabi                          5/03/94
                   Initial Revision(.c .h)
******************************************************************************/

#include <basics.h>
#include <MinArray.h>
#include <MLCStream.h>

/******************************************************************************
  Description  : Constructor with kValue initializer and initial
                    vlaues for array elements and maxValue.
		 Argument initialValue defaults to 0.
  Comments     :
******************************************************************************/
template <class Element>
MinArray<Element>::MinArray(int k)
: Array<Element>(0,k)
{
   if( k <= 0 )
      err << "MinArray<>::MinArray : too small a k value " << k << " -"
             " k is supposed to be greater than 0" << fatal_error;
   numMinElements = 0;
}


/******************************************************************************
  Description  : Inserts a data item into MinArray. The data item
                    is inserted iff its value is less than the
            	    maximum value in the array. And the data item with
		    maximum values is discarded.
  Comments     : the size of the array is supposed to be greater than 1.
******************************************************************************/
template <class Element>
void MinArray<Element>::insert(const Element& element)
{
   if (numMinElements < size()) {
      if (numMinElements == 0) { // first element
         maxValue = element;
         maxIndex = 0;
      } else if (maxValue < element) {
         maxValue = element;
         maxIndex = numMinElements;
      }
      index(numMinElements++) = element; // store in array
   }
   else if (element < maxValue) { // insert the element
      index(maxIndex) = element;
      maxValue = max(maxIndex);      
   }
}

