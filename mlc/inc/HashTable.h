// MLC++ - Machine Learning in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file. For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef HashTable_h
#define HashTable_h

#include <basics.h>
#include <DblLinkList.h>
#include <Array.h>


// This is an ABC for all hash table related classes.
// hash() is a pure virtual function which must be defined in any
// derived classes.

// For restrictions of KeyType and DataType see HashTable.c file

template <class KeyType, class DataType>
class HashTable  {
   Array< DblLinkList< DataType > > table;
   int numItems; // stores actual number of data items in the hash table.
                 // note: this is different from the size of the table.

   NO_COPY_CTOR2(HashTable, table(1));

   /* data members for statistics */
   DBG_DECLARE(
	       int maxNumItemsInHT; 
	       int numInserts;
	       int numDeletes;
	       int numFinds;
	       int maxNumElementsInAllBuckets;
	       int numNonEmptyBuckets;
	       int maxNonEmptyBuckets;
	       Real maxWorstRate; 
	      )

protected :
   virtual int hash(const KeyType& key) const = 0;
   static const Real LOAD_FACTOR;
   
public :
   HashTable(int estimatedNum);
   virtual ~HashTable();

   virtual void merge(const HashTable& source);
   void ok_members() const;
   // returns the size of the hash table
   virtual int size()  const { return table.size(); }
   // returns the actual number of data
   virtual int num_items() const { return numItems; }

   virtual void insert(const DataType&);
   virtual DataType* find(const KeyType&);  // returns NULL, if not found.
   virtual const DataType* find(const KeyType&) const;
   virtual Bool find(const DataType&, int hashBin) const;
   virtual Bool del(const KeyType&);
   virtual void display(MLCOStream& stream = Mcout) const;   
   DBG_DECLARE(virtual void stats(const MString& description);)
};
#endif  



