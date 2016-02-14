// MLC++ - Machine Learning in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/*****************************************************************************
  Description  : This is a class for an Universal hash table. It derives from
                    HashTable<KeyType, DataType> base class and 
		    defines Universal hash function. Universal hashing
		    function is chosen at run time from a collection
		    of such functions and the expected number of
		    collisions involving a particular key is less than
		    1. For details, refer to 'Introduction to
		    Algorithms', Cormen et al, p230-p231, MIT press.
  Assumptions  : hash() function assumes KeyType can be cast to char*
                    (without loss of semantics, i.e. that key does not
		    contain pointers within it). In other cases, new
		    hash() function may be defined  properly in any
		    derived classes from this class.
		 The KeyType should be a class which has a length()
		    member function returning the length of key in bytes.
  Comments     : The size of hash table, if it is less than 2^8+1, is
                    extended to 2^8+1. This is a necessary condition for
		    universal hashing functions to work properly.
  Complexity   : hash() takes O(m), where m is the length of keyValue.
  Enhancements :
  History      : Yeogirl Yun                                      4/17/94
                   initial revision
*****************************************************************************/ 

#include <basics.h>
#include <UnivHashTable.h>


/******************************************************************************
  Description  : Copy constructor which generates a fatal error when
                   called.
  Comments     :
******************************************************************************/
template <class KeyType, class DataType>
UniversalHashTable<KeyType, DataType>::UniversalHashTable
                           (const UniversalHashTable<KeyType, DataType>& )
: HashTable<KeyType, DataType>(1), randomData(0)
{
   err << "UniversalHashTable<>::UniversalHashTable : copy"
          " constructor is not allowed. Use copy constructor with"
          " dummy arguments" << fatal_error;
}



/******************************************************************************
  Description  : Copy constructor with dummy argument.
                 It initializes randomData and pass the source argument over
      		   to base class' copy constructor.
		 It should not be called by any derived class when it defined
		   its own hash function because within this copy constructor
		   UniversalHashTable<>::hash() would be used.
  Comments     :
******************************************************************************/
template <class KeyType, class DataType>
UniversalHashTable<KeyType, DataType>::UniversalHashTable
   (const UniversalHashTable<KeyType, DataType>& source, CtorDummy)
   : HashTable<KeyType, DataType>(max(source.num_items(),
				      (int)(source.size()*LOAD_FACTOR))),
     randomData(0)
{
   merge(source);
}




/******************************************************************************
  Description  : Constructor with estimated number arguments.
  Comments     : The size samller than (UCHAR_MAX + 1) is extened to
                   the size of UCHAR_MAX + 1 without any error
		   message. This is to make hash table size greater
		   than 2^8. This will work even in the case
		   LOAD_FACTOR is 1.
******************************************************************************/
template <class KeyType, class DataType>
UniversalHashTable<KeyType, DataType> :: UniversalHashTable(int estimatedNum)
 : randomData(0),
   HashTable<KeyType, DataType>(max(UCHAR_MAX + 1, estimatedNum))
{
}



/******************************************************************************
  Description  : Constructor with estimated number arguments and seed for
                   RandCharArray.
  Comments     : The size samller than (UCHAR_MAX + 1) is extened to
                   the size of UCHAR_MAX + 1 without any error
		   message. This is to make hash table size greater
		   than 2^8. This will work even in the case
		   LOAD_FACTOR is 1.
******************************************************************************/
template <class KeyType, class DataType>
UniversalHashTable<KeyType, DataType> :: UniversalHashTable
   (int estimatedNum, unsigned int seed) : randomData(0, seed),
   HashTable<KeyType, DataType>(max(UCHAR_MAX + 1, estimatedNum))
{
}





/******************************************************************************
  Description  : Universal hash function. It assumes the length of
                   keyValue is greater than 0( >= 1 ).
  Comments     : It requires the size of hash table should be greater
                   than 2^8, or 256.
******************************************************************************/
template <class KeyType, class DataType>
int UniversalHashTable<KeyType, DataType>:: hash(const KeyType& keyValue) const
{

   const unsigned char* ptr = (const unsigned char *)&keyValue;
   long hashValue = 0;

   // note : To iterate from the end helps speed execution time since
   //        it obviates unnecesary iterative extensions of array.
   //        For details, see the DynamicArray.c file.
   for(int i = keyValue.length()-1; i >= 0 ; i-- ) 
      hashValue += ptr[i] * // constness cast away : randomData, being a
			    // dynamic array, does not allow const function.
      ((UniversalHashTable<KeyType, DataType> *)this)->
	 randomData.index(i);

   return hashValue  % size();
}
    





