// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _ListHOODGInd_h
#define _ListHOODGInd_h 1

#include <HOODGInducer.h>
#include <AttrOrder.h>

class ListHOODGInducer : public HOODGInducer {
   AttrOrder ao;
   Array<int> *tmpOrder; // temporary order for projected sets
   int orderPosition;   // position within the order vector.
   
public:
   ListHOODGInducer(const MString& dscr, CGraph* aCgraph = NULL);
   virtual ~ListHOODGInducer() {delete tmpOrder;}

   virtual int class_id() const { return LIST_HOODG_INDUCER; }
   virtual void train(); 
   virtual int best_split(const BagPtrArray& bpa);
   virtual void set_user_options(const MString& preFix);
   virtual AttrOrder& get_attr_order_info() { return ao; }
   // convenience function
   void set_order(const Array<int>& order) { ao.set_order(order);}
};


#endif
