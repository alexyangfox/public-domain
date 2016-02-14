// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/******************************************************************************
  Description  : MaxArray<> is derived from Array<> and contains the
                    kth largest data items at most.
  Assumptions  : 
  Comments     : 
  Complexity   : Both insert()s takes O(n), where n is kValue.
  Enhancements : 
  History      : Yeogirl Yun                                      10/20/94
                   Initial Revision(.c .h)
******************************************************************************/

#include <basics.h>
#include <MaxArray.h>
#include <MLCStream.h>

/******************************************************************************
  Description  : Constructor with kValue initializer and initial
                    vlaues for array elements and minValue.
		 Argument initialValue defaults to 0.
  Comments     :
******************************************************************************/
template <class Element>
MaxArray<Element>::MaxArray(int k)
: Array<Element>(0,k)
{
   if( k <= 0 )
      err << "MaxArray<>::MaxArray : too small a k value " << k << " -"
             " k is supposed to be greater than 0" << fatal_error;
   numMaxElements = 0;
}


/******************************************************************************
  Description  : Inserts a data item into MaxArray. The data item
                    is inserted iff its value is less than the
            	    maximum value in the array and the data item with
		    maximum values is discarded.
  Comments     : the size of the array is supposed to be greater than 1.
******************************************************************************/
template <class Element>
void MaxArray<Element>::insert(const Element& element)
{
   if (numMaxElements < size()) {
      if (numMaxElements == 0) { // first element
         minValue = element;
         minIndex = 0;
      } else if (minValue > element) {
         minValue = element;
         minIndex = numMaxElements;
      }
      index(numMaxElements++) = element; // store in array
   }
   else if (element > minValue) { // insert the element
      index(minIndex) = element;
      minValue = min(minIndex);      
   }
}




