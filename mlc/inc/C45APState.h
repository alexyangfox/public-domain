// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _C45APState_h
#define _C45APState_h 1

#include <DynamicArray.h>
#include <CompState.h>
#include <CtrInstList.h>
#include <BaseInducer.h>


class PgmFlagsStrings : public PtrArray<Array<MString>*> {
public:
   void OK() const;
   
   PgmFlagsStrings(int numFlags);
   virtual ~PgmFlagsStrings();
   void set_flag(int num, Array<MString>* flagVals);
};

#define C45AP_INFO 12

class C45APInfo : public AccEstInfo {
   PgmFlagsStrings flagStrings;
   BoolArray varyArray;   // indicates which flags may be varied
   Array<int> initFlags;  // initial values
   
public:
   static Array<int> *init_flags();

   C45APInfo();
   virtual int lower_bound(int num) {
      return should_vary(num) ? flagStrings.index(num)->low() :
	 initFlags.index(num); }
   virtual int upper_bound(int num) {
      return should_vary(num) ? flagStrings.index(num)->high() :
	 initFlags.index(num); }
   virtual void display_values(const Array<int>& values, MLCOStream& out =
			       Mcout) const;
   MString get_c45_flags(const Array<int>& values) const;
   void set_vary(int n, Bool val) { varyArray.index(n) = val; }
   Bool should_vary(int n) const { return varyArray.index(n); }
   virtual int class_id() const { return C45AP_INFO; }
};


class C45APState : public CompState {
public:
   C45APState(Array<int>*& info, const AccEstInfo& gI) : CompState(info, gI) {}
   virtual ~C45APState() { }

   virtual CompState *create_state(Array<int>*& info) {
      return new C45APState(info, globalInfo); }
   
   // functions for deriving from AccEstState
   virtual void pre_eval(AccEstInfo *);
   
   // display functions
   virtual void display_info(MLCOStream& stream = Mcout) const;
};

DECLARE_DISPLAY(C45APState);

#endif





