// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _DiscSearchInducer_h
#define _DiscSearchInducer_h 1

#include <SearchInducer.h>
#include <DiscState.h>

class DiscSearchInducer : public SearchInducer {
public:
   enum Direction { forward, middle, backward };
   Bool middleRestricted; /* up one down one only */
protected:
   Direction direction;

public:
   DiscSearchInducer(const MString& description, BaseInducer* ind = NULL);
   virtual ~DiscSearchInducer();

   virtual void set_user_options(const MString& prefix);
   void display(MLCOStream& stream = Mcout) const;

   virtual AccEstInfo *create_global_info() const {
      return new DiscInfo(); }
   virtual Array<int> *create_initial_info(InstanceBag*);
   virtual AccEstState *create_initial_state(Array<int> *&initialInfo,
					     const AccEstInfo& gI) const 
      { return new DiscState(initialInfo, gI); }

   virtual Categorizer *state_to_categorizer(
      const State<Array<int>, AccEstInfo>& state) const;
         
};

DECLARE_DISPLAY(DiscSearchInducer);

#endif

