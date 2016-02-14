/******************************************************************************
  Description  : HashTable is an ABC which implements a general hash table data
                   structure for distinct keys, with key type and data
		   type templated. Collision is handled by chaining.
  Assumptions  : Templated KeyType is assumed to provide operator==()
                   which must compare deep equality between keys.
  		 Templated DataType is assumed to provide operator=()
		   which is needed for DblLinkList, but since DataType
		   will usually be a pointer, it will exist by
		   default.
		 Templated DataType is assumed to provide
		   contains_key(const KeyType&) which is needed to
		   compare between key and data.
		 Templated DataType is assumed to provide get_key(const
		   DataType&) which returns KeyType.
  Comments     : The hash function is undefined.
  Complexity   : del() and find() takes O(LOAD_FACTOR + h), where
                   LOAD_FACTOR is a load factor of the hash table
		   and h is the time hash() function takes. For
		   details, refer to 'Introduction to Algorithms',
		   Cormen, Leiserson, and Rivest, p224-p225, MIT press.
  Enhancements : For space and speed efficiency, dynamic feature is
		   desirable. With dynamic feature, hash table may 
		   shrink or grow depending on data loads and key
		   distributions.
  History      : Yeogirl Yun                                      10/20/94
                   Added a find(const DataType&, int) method and made
		   insert call hash() only one time using this method.
                 Yeogirl Yun
                   Added a copy constructor and display().         9/06/94
                 Yeogirl Yun
                   Added ok_members().                             9/02/94
                 Yeogirl Yun
                   Initial revision(.c .h)               	   4/26/94
******************************************************************************/

#include <basics.h>
#include <DblLinkList.h>
#include <HashTable.h>
#include <MLCStream.h>



/******************************************************************************
  Description  : Constructor with estimated table size argument.
                 Actual size loaded is estimatedNumElement/LOAD_FACTOR
  Comments     : 
******************************************************************************/
template <class KeyType, class DataType>
HashTable<KeyType, DataType>::HashTable(int estimatedNumElement) : 
     table((int)(estimatedNumElement/LOAD_FACTOR))
{
   numItems = 0;

   DBG( maxNumItemsInHT = 0;
        maxNumElementsInAllBuckets = 0;
        numInserts = 0;
        numDeletes = 0;
        numFinds =0;
        numNonEmptyBuckets = 0;
        maxNonEmptyBuckets = 0;
        maxWorstRate = 0.0 );
}

/******************************************************************************
  Description  : Destructor.  May display hash table stats on destruction
                   if the enviornment variable DISPLAY_HASH_STATS is set
		   to yes.
  Comments     : 
******************************************************************************/
template <class KeyType, class DataType>
HashTable<KeyType, DataType>::~HashTable()
{
   if (get_env_default("DISPLAY_HASH_STATS", "no") == "yes")
   DBG(stats("dtor")); // function in DBG_DECLARE 
}

/*****************************************************************************
   Description : It merges all the data elmements of the source' hash table by
                    inserting them.
		 The caller must ensure that this will not cause duplicate
		   data to be inserted, or insert will abort.
   Comments    : It may have a different structure from the source but it will
                    have the exactly same data elments with the source.
*****************************************************************************/
template <class KeyType, class DataType>
void HashTable<KeyType, DataType>::merge(const HashTable<KeyType,DataType>&
					source)   
{
   for (int i=0; i < source.size(); i++) 
      for (DLLPix<DataType> pix(source.table[i], 1); pix; ++pix)
	 insert((source.table[i])(pix));
}




/*****************************************************************************
   Description : Calls OK() function members for all the data items in
                    the hash table.
   Comments    : This takes O(n) where n is the size of the hash table.
                 For DataType which has no OK() function member should
                    not use this method, otherwise it would cause a
		    compile time error.
*****************************************************************************/
template <class KeyType, class DataType>
void HashTable<KeyType, DataType>::ok_members() const
{
   for (int i = 0; i < size(); i++) {
      const DblLinkList< DataType > &list = table[i];
      if (list.length() > 0) 
	 for( DLLPix< DataType > pix(list,1); pix; pix.next() ) 
	    list(pix).OK();
   }
}




/******************************************************************************
  Description  : Inserts a data item into hash table.
                 An attempt to insert duplicate data is regarded as a
	           fatal error.
  Comments     :
******************************************************************************/
template <class KeyType, class DataType>
void HashTable<KeyType, DataType>::insert(const DataType& dataValue)
{
   KeyType keyValue = dataValue.get_key();
   int hashBin = hash(keyValue);
   
   // check if there is a duplicate data.
   if( find(dataValue, hashBin) ) {
      err << "HashTable<KeyType, DataType>::insert(const KeyType&,"
          << "const DataType&) : attempt to insert duplicate data "
          << keyValue << fatal_error;
   }
   
   // just prepend data, DLL will do everything.
   table[hashBin].prepend(dataValue);
   numItems++;
   
   DBG(numInserts++;
       if (table[hashBin].length() == 1)
          numNonEmptyBuckets++;
       maxNonEmptyBuckets = max(maxNonEmptyBuckets, numNonEmptyBuckets) ;
       maxNumItemsInHT = max(maxNumItemsInHT, numItems);
       maxNumElementsInAllBuckets =
          max(maxNumElementsInAllBuckets,table[hashBin].length());
       maxWorstRate =
          max( maxWorstRate, (Real)numItems/numNonEmptyBuckets);
     );
}



/******************************************************************************
  Description   : Removes a data item from hash table. Returns True
                    iff found.
  Comments      : 
******************************************************************************/
template <class KeyType, class DataType>
Bool HashTable<KeyType, DataType>::del(const KeyType& keyValue)
{
   DblLinkList<DataType> &listItems = table[hash(keyValue)];
   for(DLLPix< DataType > pix(listItems, 1); pix ; pix.next() ) {
      if( listItems(pix).contains_key(keyValue) ) {
	 listItems.del(pix); // delete the data item.
	 numItems--;
	 DBG( numDeletes++;
	      if( listItems.length() == 0 ) numNonEmptyBuckets--
	    );
	 return TRUE;
      }
   }
   
   return FALSE;
}




/******************************************************************************
  Description  : Find the data item corresponding to keyValue.
                 Returns a valid pointer to the data item, if found.
                   Otherwise returns NULL.
  Comments     : Note that there are two versions of find(). This is
                   for non-const object.
******************************************************************************/
template <class KeyType, class DataType>
DataType* HashTable<KeyType, DataType>::find(const KeyType& keyValue) 
{
  DblLinkList<DataType> &listItems = table[hash(keyValue)];
  for(  DLLPix< DataType > pix(listItems,1); pix; pix.next() ) {
     if( listItems(pix).contains_key(keyValue) ) {
	DBG( numFinds++ );
	return &(listItems(pix));
     }
  }
  return NULL;
}




/******************************************************************************
  Description  : Find the data item corresponding to keyValue.
                 Returns a valid pointer(const) to the data item, if found.
                   Otherwise returns NULL.
  Comments     : Note that there are two versions of find(). This is
                   for const object.
******************************************************************************/
template <class KeyType, class DataType>
const DataType* HashTable<KeyType, DataType>::find(const KeyType&
						     keyValue) const

{
  const DblLinkList<DataType> &listItems = table[hash(keyValue)];
  for(  DLLPix< DataType > pix(listItems,1); pix; pix.next() ) {
     if( listItems(pix).contains_key(keyValue) ) {
	// for statistics cast const into non-const
	DBG(((HashTable<KeyType, DataType>*)this)->numFinds++ );
	return &(listItems(pix));
     }
  }
  return NULL;
}




/******************************************************************************
  Description  : Find the data item in the given hash bin.
                 Returns TRUE iff the given data item is found in the
		   hash bin.
  Comments     : 
******************************************************************************/
template <class KeyType, class DataType>
Bool HashTable<KeyType, DataType>::find(const DataType& data, int hashBin)
   const  
{
   const DblLinkList<DataType> &listItems = table[hashBin];
   const KeyType& keyValue = data.get_key();
   for(  DLLPix< DataType > pix(listItems,1); pix; pix.next() ) {
      if( listItems(pix).get_key() == keyValue  ) {
	 DBG(((HashTable<KeyType, DataType>*)this)->numFinds++ );
	 return TRUE;
      }
   }
   return FALSE;
}




/*****************************************************************************
   Description : Displays the contents of hash table. The sequence of data
                    elements to be displayed is the order of buckets. Lower
		    numbered bucket will precede the higher one. The format
		    will be the sequence number followed by the bucket number
		    and the contents of the bucket. If the bucket has no data
		    elements is will be skipped.
   Comments    : At each bucket, it calls the DblLinkList::display(), since
                    bucket is a DblLinkList.
*****************************************************************************/
template <class KeyType, class DataType>
void HashTable<KeyType, DataType>::display(MLCOStream& stream) const
{
   int count = 0;
   for (int i = 0; i < size(); i++)
      if (table[i].length() > 0) {
	 stream << "\nThe "<< setw(4) << count++ <<"th (bucket number : "
	        << setw(5) << i << ")" << endl;
	 table[i].display(stream);
      }
}



/******************************************************************************
  Description  : Gives various statistics for the hash table.
  Comments     : Note that this is not regular member function in the
                   sense that it may not be compiled in FAST mode.
******************************************************************************/
// note that can't use DBG_DECLARE, because of the two template
// arguments

#ifndef FAST
template <class KeyType, class DataType>
void HashTable<KeyType, DataType>::stats(const MString& description)
{
   Mcout <<"\n\n||******* HashTable<>stats : Statistics for"
	        "( "<<description<<" ) *******||"
                "\n* Hash table size : " << size() <<
                "\n* Number of insertions : " << numInserts <<
                "\n* Number of deletions : " << numDeletes <<
                "\n* Number of finds : " << numFinds << 
                "\n* Maximum number of non-empty  buckets ever in"
	        " the hash table : " << maxNonEmptyBuckets << 
                "\n* Maximum number of elements ever in the hash"
                " table  : " << maxNumItemsInHT <<
                "\n* Maximum number of bucket length ever in the"
	        " hash table : " << maxNumElementsInAllBuckets << 
                "\n* Worst Average length of buckets( out of"
	        " non-empty buckets ) : "<< maxWorstRate <<
                "\n----------------------------------------------"
                "--------------------------\n\n\n" << endl
         ;
}  
#endif



