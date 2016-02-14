// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/*****************************************************************************
  Description  : Implements universal dynamic array derived from
                   DynamicArray<>. This array differs from usual
		   dynamic array in that it inserts random data in the
		   extened area when it grows, which is very useful for
		   UniversalHashTable<>.
  Assumptions  :
  Comments     : 
  Enhancements : 
  History      : Yeogirl Yun                                   4/28/94
                   Initial revision(.c .h)
*****************************************************************************/

#include <basics.h>
#include <RandCharArray.h>

/*****************************************************************************
  Description  : Extends the current array and inserts random data in
                   the extended area.
  Comments     : This is a protected member function.
*****************************************************************************/
void RandCharArray::extend_array(int indexRequested)
{
   // increase sizeAllocated member until it gets
   // larger than sizeRequested.
   while( increase_size() <= indexRequested );

   unsigned char* tempBuf = new unsigned char[sizeAllocated];
   int i;
   for( i = 0; i < arraySize; i++)
      tempBuf[i] = elements[i];
   // initialize up to all the allocated size by random characters.
   for( ; i < sizeAllocated; i++ )
      tempBuf[i] = (unsigned char)randomVal.integer(UCHAR_MAX + 1);
   
   // set the current array size
   arraySize = indexRequested + 1;
   delete [] elements;
   elements = tempBuf;
}



/*****************************************************************************
  Description  : Constructor with size.
  Comments     : Notice that an initial value argument is not necessary.
*****************************************************************************/
RandCharArray::RandCharArray(int size)
: DynamicArray<unsigned char>(size), randomVal(0)
{
   for(int i=0; i < size; i++) 
      index(i) = (unsigned char)randomVal.integer(UCHAR_MAX+1);
}




/*****************************************************************************
  Description  : Constructor with size and random seed.
  Comments     :
*****************************************************************************/
RandCharArray::RandCharArray(int size, unsigned int seed)
: DynamicArray<unsigned char>(size), randomVal(seed)
{
   for(int i=0; i < size; i++) 
      index(i) = (unsigned char)randomVal.integer(UCHAR_MAX+1);
}



/***************************************************************************
  Description : Display array.
  Comments    : We display the characters as integers because otherwise
                  they're mostly non-ASCII
***************************************************************************/

void RandCharArray::display(MLCOStream& stream) const
{
  int i;
  for (i = 0; i < size() - 1; i++)
    stream << setw(3) << (int)index(i) << ", ";
  
  if (size() > 0)
    stream << setw(3) << index(i);  // no trailing comma.
}   


DEF_DISPLAY(RandCharArray)





