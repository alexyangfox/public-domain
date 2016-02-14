// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "Inducer.c".

#ifndef _Inducer_h
#define _Inducer_h 1

#include <BaseInducer.h>
#include <AugCategory.h>
#include <AttrOrder.h>
class InstanceRC;

class Inducer : public BaseInducer { // ABC
public:
   Inducer(const MString& description) : BaseInducer(description) {}
   Inducer(const Inducer& source, CtorDummy ctorDummy);
   virtual ~Inducer() {}
   virtual void train() = 0;                                // build structure
   // display learned structure
   virtual Bool was_trained(Bool fatalOnFalse = TRUE) const = 0;
   virtual const Categorizer& get_categorizer() const = 0; 

   virtual Real train_and_test(InstanceBag* trainingSet,
			       const InstanceBag& testBag);

   // The following are convenience functions from the categorizer class.
   virtual AugCategory predict(const InstanceRC& instance) const;
   virtual void display_struct(MLCOStream& stream = Mcout,
			       const DisplayPref& dp = defaultDisplayPref) const;
   virtual Bool can_cast_to_inducer() const;
   virtual Inducer& cast_to_inducer();
   virtual Inducer* copy() const;
   virtual AttrOrder& get_attr_order_info();
};

#endif
