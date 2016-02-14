// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _OneRInducer_h
#define _OneRInducer_h 1

#include <basics.h>
#include <OneR.h>
#include <BaseInducer.h>
#include <TableInducer.h>

class OneRInducer : public BaseInducer{
   int minInstPerLabel;
public:
   OneRInducer(const MString& description, int small);
   Real train_and_test(InstanceBag* trainingSet, const InstanceBag& testBag);
};


#endif

