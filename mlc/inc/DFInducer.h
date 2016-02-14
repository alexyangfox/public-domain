// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _DiscFilterInducer_h
#define _DiscFilterInducer_h 1
#include <basics.h>
#include <Array.h>
#include <DiscCat.h>
#include <Inducer.h>
#include <CtrInducer.h>
#include <GetOption.h>
#include <BaseInducer.h>
#include <CtrInstList.h>
#include <DiscDispatch.h>

class DiscFilterInducer : public CtrInducer{
   DiscCat* categorizer;
   Array<int>* discVect;
   BaseInducer* inducer;
   DiscDispatch* dispatcher;
   
public:
   DiscFilterInducer(const MString& descr);
   ~DiscFilterInducer();
   void train();
   Bool was_trained(Bool fatalOnFalse = TRUE) const;
   const Categorizer& get_categorizer() const;
   void set_disc(const Array<int>& sourceDiscVect);
   int class_id() const { return DF_INDUCER;}
   DiscDispatch& get_dispatcher() { return *dispatcher;}
   Real train_and_test(InstanceBag* trainingSet,
		       const InstanceBag& testBag);

   void display_struct(MLCOStream& stream = Mcout,
		       const DisplayPref& dp = defaultDisplayPref) const;

   Bool can_cast_to_inducer() const;
   void set_user_options(const MString& prefix);
   void set_user_options_no_inducer(const MString& prefix);
   void set_inducer(BaseInducer*& inducer);
   BaseInducer* get_inducer();
   DiscDispatch* set_dispatch(DiscDispatch*& dispatcher);
   DiscDispatch* get_dispatch();
};

#endif










