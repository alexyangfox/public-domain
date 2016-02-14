// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _IncrInducer_h
#define _IncrInducer_h 1

#include <CtrInducer.h>

class IncrInducer : public CtrInducer { // ABC
public:   
   IncrInducer(const MString& description) : CtrInducer(description) {}
   virtual ~IncrInducer() {}
   virtual Pix add_instance(const InstanceRC& instance) = 0;
   virtual InstanceRC del_instance(Pix& pix) = 0;
};
#endif


