// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _AccEstState_h
#define _AccEstState_h 1

#include <DynamicArray.h>
#include <AccEstDispatch.h>
#include <AccData.h>
#include <State.h>
#include <CtrInstList.h>
#include <BaseInducer.h>


#define ACC_EST_INFO 10

class AccEstInfo : public StateGlobalInfo {
   LOG_OPTIONS;
   
public:
   AccEstInfo();

   // functions required by CompState (see CompState.c for details)
   virtual int lower_bound(int index) = 0;
   virtual int upper_bound(int index) = 0;
   virtual void display_values(const Array<int>& values, MLCOStream& out =
			       Mcout) const = 0;
   virtual Bool use_compound() { return useCompound; }

   // provide a set_user_options to quickly set up this struct
   virtual void set_user_options(const MString& prefix);

   virtual int class_id() const { return ACC_EST_INFO; }

   // public data members for use by AccEstState::eval().
   BaseInducer *inducer;
   InstanceList *trainList;
   InstanceList *testList;
   AccEstDispatch accEst;
   int seed;
   Bool useCompound;
   Real complexityPenalty;
};
   

class AccEstState : public State<Array<int>, AccEstInfo> {
   AccData accData;
   int numEvaluations;
   
public:
   AccEstState(Array<int>*& initialInfo, const AccEstInfo& gI);
   virtual ~AccEstState() { }

   virtual void set_final_state();

   virtual Real eval(AccEstInfo *, Bool computeReal=TRUE,
		     Bool computeEstimated=TRUE);

   virtual void display_info(MLCOStream& stream = Mcout) const;
   virtual void display_stats(MLCOStream& stream = Mcout) const;
   virtual void display_for_graph(MLCOStream& stream = Mcout) const;   
   virtual Real get_accuracy() const { return accData.mean(); }
   virtual Real get_accuracy_std_dev() const
     { return accData.std_dev_of_mean(); }
   virtual Real get_real_accuracy() const
     { return accData.real_accuracy(); }
   virtual Real get_theoretical_std_dev() const
     { return accData.theo_std_dev(); }

   virtual void pre_eval(AccEstInfo *) { };
   virtual void construct_lists(AccEstInfo *, InstanceList *& /* trainList */, 
				InstanceList *& /* testList */) {}
   virtual void destruct_lists(AccEstInfo *, InstanceList * /* trainList */,
			       InstanceList * /* testList */) { };
};


#endif





