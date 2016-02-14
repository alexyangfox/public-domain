// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "TableInducer.c".

#ifndef TableInducer_h
#define TableInducer_h


#include <basics.h>
#include <IncrInducer.h>
#include <AugCategory.h>
#include <BagSet.h>

class TableCategorizer;

class TableInducer : public IncrInducer {
   NO_COPY_CTOR(TableInducer);

   Bool majorityOnUnknown;
   TableCategorizer* categorizer;

public:
   TableInducer(const MString& dscr, Bool majority = TRUE);
   virtual ~TableInducer();

   virtual void train();
   virtual Bool was_trained(Bool fatalOnFalse = TRUE) const;
   virtual const Categorizer& get_categorizer() const;
   virtual InstanceRC del_instance(Pix& pix);
   virtual Pix add_instance(const InstanceRC& inst);

   // Options.
   Bool get_majority_on_unknown() const { return majorityOnUnknown; }
   void set_majority_on_unknown(Bool majority)
                                           {majorityOnUnknown = majority; }
   virtual Bool can_cast_to_incr_inducer() const;
   virtual IncrInducer& cast_to_incr_inducer();
};
#endif









