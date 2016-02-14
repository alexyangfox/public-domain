// MLC++ - Machine Learning in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file. For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _UniversalHashTable_h
#define _UniversalHashTable_h 1

#include <HashTable.h>
#include <RandCharArray.h>

// This class implements hash table based on Universal hash function.

template <class KeyType, class DataType>
class UniversalHashTable :  public HashTable<KeyType, DataType> {

   // can't use NO_COPY_CTOR2() because compiler get confused with
   // two template arguments of HashTable.
   UniversalHashTable(const UniversalHashTable& );
   
protected :
   RandCharArray randomData;   
   virtual int hash(const KeyType& keyValue) const;

public :
   UniversalHashTable(int estimatedNumElement);
   UniversalHashTable(int estimatedNumElement, unsigned int seed);
   UniversalHashTable(const UniversalHashTable& source, CtorDummy);
   virtual ~UniversalHashTable() {}
};
#endif

    


