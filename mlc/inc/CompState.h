// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _CompState_h
#define _CompState_h 1

#include <basics.h>
#include <DblLinkList.h>
#include <LogOptions.h>
#include <AccEstState.h>
#include <StateSpace.h>
#include <math.h>

/***************************************************************************
  Description : class CompElem holds a single element in the array of
                  features used for building the compound node in gen_succ.
  Comments    : Should be a private class.
***************************************************************************/

class CompElem {
public:
   int num;
   int value;
   Real eval;

   int operator<(const CompElem& other) const {
      // tie breakers are just an attempt to achieve consistency
      //   across machines because without it there was a difference
      //   on suns and SGI (probably the sort).
      return (eval < other.eval || 
             (eval == other.eval && num < other.num) ||
             (eval == other.eval && num == other.num && value < other.value));}
   int operator==(const CompElem& other) const {
      return (eval == other.eval && num == other.num &&
	      value == other.value); }
   void display(MLCOStream& out = Mcout) const {
      out << num << " (" << value << "): " << eval; }
};

DECLARE_DISPLAY(CompElem);

class CompState : public AccEstState {
   
public:
   CompState(Array<int>*& initStateInfo, const AccEstInfo& gI);
   virtual DblLinkList<State<Array<int>, AccEstInfo> *>* gen_succ(AccEstInfo*,
	      StateSpace< State<Array<int>, AccEstInfo> > *,
					 Bool computeReal = TRUE);
   virtual CompState *create_state(Array<int>*& initInfo) = 0;
};


#endif


