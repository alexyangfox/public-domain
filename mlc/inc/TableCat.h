// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the "TableCat.c".

#ifndef _TableCategorizer_h
#define _TableCategorizer_h 1

#include <Categorizer.h>
#include <BagSet.h>
#include <InstanceHash.h>

class TableCategorizer : public Categorizer {

   NO_COPY_CTOR(TableCategorizer);
   Category defaultCat;
protected:
   InstanceHashTable hashTable;
   
public:
   TableCategorizer(const InstanceBag& bag, Category defaultCategory,
		    const MString& dscr);
   TableCategorizer(const TableCategorizer& source, CtorDummy ctorDummy);
   virtual ~TableCategorizer() { }
   virtual AugCategory categorize(const InstanceRC&) const;
   virtual void display_struct(MLCOStream& stream = Mcout,
                          const DisplayPref& dp = defaultDisplayPref) const; 
   virtual void add_instance(const InstanceRC& instance);
   virtual void del_instance(const InstanceRC& instance);
   virtual Bool find_instance(const InstanceRC& instance) const;
   Category get_default_category() const { return defaultCat; }
   void set_default_category(Category cat) { defaultCat = cat; }
   int table_size() const {return hashTable.num_instances();}
   virtual Categorizer* copy() const;

   // Returns the class id
   virtual int class_id() const { return CLASS_TABLE_CATEGORIZER; }

   virtual Bool operator==(const Categorizer &cat) const;
};
#endif
