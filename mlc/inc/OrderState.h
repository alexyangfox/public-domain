// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _OrderState_h
#define _OrderState_h 1

#include <basics.h>
#include <DblLinkList.h>
#include <LogOptions.h>
#include <AccEstState.h>
#include <StateSpace.h>

#define ORDER_INFO 13

class OrderInfo : public AccEstInfo {
   // should not use these functions, but cannot leave pure-virtual
   virtual int lower_bound(int) { ASSERT(FALSE); return 0; }
   virtual int upper_bound(int) { ASSERT(FALSE); return 0; }
   virtual void display_values(const Array<int>& values,
			       MLCOStream& out = Mcout) const 
      { out << values; }
   virtual Bool use_compound() { return FALSE; }
   virtual int class_id() const { return ORDER_INFO; }
};   

class OrderState : public AccEstState {
   
public:
   OrderState(Array<int>*& initStateInfo, const AccEstInfo& gI);

   virtual DblLinkList<State<Array<int>, AccEstInfo> *>* gen_succ(AccEstInfo*,
	      StateSpace< State<Array<int>, AccEstInfo> > *,
					 Bool computeReal = TRUE);
   virtual void display_info(MLCOStream& out) const;
   
   virtual OrderState *create_state(Array<int>*& initInfo) = 0;
};


#endif


