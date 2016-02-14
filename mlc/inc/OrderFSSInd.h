// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _OrderFSSInd_h
#define _OrderFSSInd_h 1

#include <SearchInducer.h>
#include <OrderFSSState.h>

class OrderFSSInducer : public SearchInducer {
public:
   enum Direction { forward, backward };
private:
   Inducer *innerInducer;
   Direction direction;
   AttrOrder ao;
public:
   enum InnerType { listHOODG, COODG, tableCas, listODG };
   
   static Inducer* search_inducer();
   OrderFSSInducer(const MString& description, Inducer* ind = NULL);

   virtual void set_user_options(const MString& prefix);
   
   virtual AccEstInfo *create_global_info() const {
      return new OrderInfo; }
   virtual Array<int> *create_initial_info(InstanceBag* trainingSet);
   virtual AccEstState *create_initial_state(Array<int> *&initialInfo,
					     const AccEstInfo& gI) const
      { return new OrderFSSState(initialInfo, gI); }
   virtual Categorizer *state_to_categorizer(
            const State<Array<int>, AccEstInfo>& state) const;
         
};


#endif
