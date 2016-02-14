// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _FSSState_h
#define _FSSState_h 1

#include <DynamicArray.h>
#include <CompState.h>
#include <CtrInstList.h>
#include <BaseInducer.h>

#define FSS_INFO 11

class FSSInfo : public AccEstInfo {
public:
   virtual int lower_bound(int) { return 0; }
   virtual int upper_bound(int) { return 1; }
   virtual void display_values(const Array<int>& values, MLCOStream& out =
			       Mcout) const;
   virtual int class_id() const { return FSS_INFO; }
};


class FSSState : public CompState {
public:
   FSSState(Array<int>*& featureSubset, const AccEstInfo& gI);
   virtual ~FSSState() { }

   virtual CompState *create_state(Array<int>*& info) {
      return new FSSState(info, globalInfo);
   }

   // functions for deriving from AccEstState
   virtual void construct_lists(AccEstInfo *, InstanceList *&,
			        InstanceList *& /*testList*/);
   virtual void destruct_lists(AccEstInfo *, InstanceList *,
			       InstanceList * /*testList*/);

   // display functions
   virtual void display_info(MLCOStream& stream) const;
};


#endif





