// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _SearchInducer_h
#define _SearchInducer_h 1

#include <Inducer.h>
#include <SSSearch.h>
#include <MRandom.h>
#include <AccEstState.h>
#include <ProjectInd.h>

class SearchInducer : public CtrInducer {
   RAND_OPTIONS;
   MString dotFileName;     // name of dot file for graph output
   
protected:
   int evalLimit;
   StateSpaceSearch<Array<int>, AccEstInfo> *ssSearch;
    // we don't own this but it may be NULL
   State<Array<int>, AccEstInfo>* finalState;
public:
   enum SearchMethod { bestFirst, hillClimbing, simulatedAnnealing };

protected:
   SearchMethod searchMethod;
   Categorizer *categorizer;
   BaseInducer *baseInducer;
   
public:   
   AccEstInfo *globalInfo; // global search information.
                           // must be set in derived class constructor
   
   // Get's ownership of given inducer.  Defaults to environment
   //   variable <prefix>INDUCER if NULL
   SearchInducer(const MString& description, BaseInducer* ind = NULL);
   virtual ~SearchInducer();
   virtual Bool has_global_info(Bool fatalOnFalse = TRUE) const;
   virtual void set_user_options(const MString& prefix);
   
   virtual const State<Array<int>, AccEstInfo> &
     search(InstanceBag* trainingSet);
   
   virtual void train();
   virtual Bool was_trained(Bool fatalOnFalse = TRUE) const;
   virtual const Categorizer& get_categorizer() const;
   
   Real train_and_test_files(const MString& fileStem,
			     const MString& namesExtension = defaultNamesExt,
			     const MString& dataExtension = defaultDataExt,
			     const MString& testExtension = defaultTestExt);

   void display(MLCOStream& stream = Mcout) const;
   virtual Real train_and_test(InstanceBag* trainingSet,
			       const InstanceBag& testList);
   virtual Bool can_cast_to_inducer() const;

   // new virtuals
   virtual AccEstInfo *create_global_info() const = 0;
   virtual Array<int> *create_initial_info(InstanceBag* trainingSet) = 0;
   virtual AccEstState *create_initial_state(Array<int> *&initialInfo,
					     const AccEstInfo& gI) const =0;
   virtual Categorizer *state_to_categorizer(
            const State<Array<int>, AccEstInfo>& state) const;
   const State<Array<int>, AccEstInfo>& get_final_state();

      
   
   
};

DECLARE_DISPLAY(SearchInducer);

#endif
