// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _C45APInducer_h
#define _C45APInducer_h 1

#include <basics.h>
#include <SearchInducer.h>
#include <C45APState.h>
#include <C45Inducer.h>

class C45APInducer : public SearchInducer {
public:
   C45APInducer(const MString& description) :
      SearchInducer(description, new C45Inducer("automatic " + description))
   { globalInfo = create_global_info(); }

   virtual void set_user_options(const MString& prefix);
   
   virtual AccEstInfo *create_global_info() const { return new C45APInfo; }
   virtual Array<int> *create_initial_info(InstanceBag*) {
      return C45APInfo::init_flags(); }      
   virtual AccEstState *create_initial_state(Array<int> *&initialInfo,
					     const AccEstInfo& gI) const {
      return new C45APState(initialInfo, gI); }
};

#endif
