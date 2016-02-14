// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _ContinFilterInducer_h
#define _ContinFilterInducer_h 1
#include <basics.h>
#include <Array.h>
#include <Inducer.h>
#include <CtrInducer.h>
#include <GetOption.h>
#include <BaseInducer.h>
#include <CtrInstList.h>
#include <convDisplay.h>

//@@ move this to base inducer.
#define CF_INDUCER 29

class ContinFilterInducer : public BaseInducer {
   BaseInducer *continInducer;
   Normalization normMethod;
   Conversion convMethod;
   
public:
   ContinFilterInducer(const MString& descr);
   ~ContinFilterInducer();

   static Normalization defaultNormMethod;
   static Conversion defaultConvMethod;
   
   int class_id() const { return CF_INDUCER;}
   Real train_and_test(InstanceBag* trainingSet, const InstanceBag& testBag);
   void set_cf_sub_inducer(BaseInducer*& ind);
   void set_norm_method(Normalization val) { normMethod = val; }
   Normalization get_norm_method() const { return normMethod; }
   void set_conv_method(Conversion val) { convMethod = val; }
   Conversion get_conv_method() const { return convMethod; }
   void set_user_options(const MString& preFix);
};
#endif


