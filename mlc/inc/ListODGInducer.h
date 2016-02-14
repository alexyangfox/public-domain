// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "ListODGInducer.c".

#ifndef _ListODGInducer_h
#define _ListODGInducer_h 1

#include <DTCategorizer.h>
#include <CtrBag.h>
#include <CtrInducer.h>
#include <ODGInducer.h>
#include <Array.h>
#include <DynamicArray.h>
#include <SplitInfo.h>


class ListODGInducer : public ODGInducer {
   AttrOrder ao; // for use in OrderFSS
   int currentSplitInfo;
   ODGInducer* caller;
public: 
   ListODGInducer(const MString& dscr, ODGInducer* cal,
		  CGraph* aCgraph = NULL) :
      ODGInducer(dscr, aCgraph) { caller = cal; }
   ~ListODGInducer();
   virtual Categorizer *best_split(Array<UsedAttributes>& usedAttributes,
				   DynamicArray<SplitInfo>& splitInfoArray,
                                   DynamicArray<const CtrInstanceBag*> &,
				   MStringArray*& catNames,
				   Real& minCondEntropy,
				   Real previousEntropyy);
   virtual void train();
   // setting the split info array can handle continuous attributes
   void set_split_info_array(const Array<SplitInfo>& spArray);
   NodePtr induce_oblivious_decision_graph(CGraph& aCgraph);
   // The attribute order is just for nominals
   virtual AttrOrder& get_attr_order_info() { return ao; }
   // convenience function
   void set_order(const Array<int>& order) { ao.set_order(order);}
   virtual void set_user_options(const MString& preFix);
};

#endif
