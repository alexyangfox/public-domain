// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _FSSInducer_h
#define _FSSInducer_h 1

#include <SearchInducer.h>
#include <FSSState.h>
#include <ProjectInd.h>

class FSSInducer : public SearchInducer {
public:
   enum Direction { forward, backward };

protected:
   Direction direction;

public:   
   FSSInducer(const MString& description, BaseInducer* ind = NULL);

   virtual void set_user_options(const MString& prefix);
   virtual void display(MLCOStream& stream = Mcout) const;
   virtual AccEstInfo *create_global_info() const {
      return new FSSInfo(); }
   virtual Array<int> *create_initial_info(InstanceBag*);
   virtual AccEstState *create_initial_state(Array<int> *&initialInfo,
					     const AccEstInfo& gI) const
      { return new FSSState(initialInfo, gI); }
   virtual Categorizer *state_to_categorizer(
            const State<Array<int>, AccEstInfo>& state) const;
         
};

DECLARE_DISPLAY(FSSInducer);

#endif
