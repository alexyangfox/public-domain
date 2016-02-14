// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _OrderFSSState_h
#define _OrderFSSState_h 1

#include <basics.h>
#include <OrderState.h>


class OrderFSSState : public OrderState {
   
public:
   OrderFSSState(Array<int>*& initStateInfo, const AccEstInfo& gI)
      : OrderState(initStateInfo, gI) { }

   virtual void pre_eval(AccEstInfo *gInfo);

   virtual OrderState *create_state(Array<int>*& initInfo) {
      return new OrderFSSState(initInfo, globalInfo); }
};


#endif


