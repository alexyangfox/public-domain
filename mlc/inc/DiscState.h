// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _DiscState_h
#define _DiscState_h 1

#include <DynamicArray.h>
#include <CompState.h>
#include <CtrInstList.h>
#include <BaseInducer.h>
#include <RealDiscretizor.h>
#include <BinningDisc.h>
#include <OneR.h>
#include <EntropyDisc.h>
#include <DFInducer.h>


#define DISC_INFO 14

class DiscInfo : public AccEstInfo {
   Bool doFSS;
   Array<int> *lowerBound;
   Array<int> *upperBound;
   
public:
   DiscInfo() : doFSS(TRUE), lowerBound(NULL), upperBound(NULL) { }
   ~DiscInfo() { delete lowerBound; delete upperBound; }
   
   Bool does_fss() const { return doFSS;}
   void does_fss(Bool decision){ doFSS = decision;}

   virtual int lower_bound(int);
   virtual int upper_bound(int);
   virtual void display_values(const Array<int>& values,
			       MLCOStream& out = Mcout) const;
   virtual int class_id() const { return DISC_INFO; }

   virtual void compute_bounds(const SchemaRC& schema);
   virtual void compute_bounds(const SchemaRC& schema,
			       const Array<int>* initial);
   virtual void check_bounds() const;
};


class DiscState : public CompState {
public:
   DiscState(Array<int> *& featureSubset, const AccEstInfo& gI);
   virtual ~DiscState() { }

   virtual CompState *create_state(Array<int>*& info)
     { return new DiscState(info, globalInfo); }

   // functions for deriving from AccEstState
   virtual void construct_lists(AccEstInfo *, InstanceList *&,
				InstanceList *& testList);
   virtual void destruct_lists(AccEstInfo *, InstanceList *,
			       InstanceList *testList);
   virtual void pre_eval(AccEstInfo *);
   
   // display functions
   virtual void display_info(MLCOStream& stream) const;
};


#endif




