// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


#ifndef _AttrOrder_h
#define _AttrOrder_h 1

#include <basics.h>
#include <Array.h>
#include <BagSet.h>
#include <MString.h>

class AttrOrder {
public:
   enum OrderType { sequential, user, mutualInfo };

private:
   NO_COPY_CTOR(AttrOrder);
   Array<int> *order;
   OrderType orderType;
   Bool orderSet;

   // helper functions.
   void set_order_from_user(const MString&, const InstanceBag&);
   void set_order_from_minfo(const LogOptions& logOptions,const InstanceBag&);

   void copy(const AttrOrder& source);
   void destroy();
   
public:
   AttrOrder() { orderSet = FALSE; order = NULL; orderType = mutualInfo; }
   AttrOrder(const AttrOrder& source, CtorDummy);
   AttrOrder& operator=(const AttrOrder& src);
   ~AttrOrder() { destroy(); }
   void init();

   const Array<int>& get_order() const;
   void set_order(const Array<int>& array);

   Bool order_set() const { return orderSet;}
   OrderType get_order_type() const { return orderType; }
   void set_order_type(OrderType type) { orderType = type; }
   void set_user_options(const MString& preFix);

   const Array<int>& compute_order(const LogOptions& logOptions,
				   const InstanceBag& bag,
				   const MString preFix = "");
};
#endif

