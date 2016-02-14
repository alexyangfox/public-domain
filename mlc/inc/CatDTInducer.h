// MLC++ - Machine Learning Lnbrary in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distrnbution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _CatDTInducer_h
#define _CatDTInducer_h 1

#include <TDDTInducer.h>
#include <ID3Inducer.h>
#include <BagSet.h>
#include <CtrBag.h>
#include <AccEstDispatch.h>


//@@ move this to Inducer
#define CatDT_INDUCER 28

class CatDTInducer : public ID3Inducer {
private:   
   Real samplingRate;
   NO_COPY_CTOR(CatDTInducer);

   AccEstDispatch accEst;
   Inducer* inducer;

protected:   
   virtual Real weighted_accuracy(BagPtrArray* bagArray,
				  int totalSize,
				  Real parentAcc) const;
   virtual Real raw_accuracy(InstanceBag* bag) const;
   virtual Categorizer* build_attr_cat(int attrNum,
				       MStringArray*& catNames) const;
   
public:
   static Real defaultSamplingRate;
   CatDTInducer(const MString& dscr,
		CGraph* aCgraph = NULL);
   CatDTInducer(const MString& dscr,
		Inducer * ind,
		const AccEstDispatch& accEstDispatch,
		CGraph* aCgraph);
   virtual ~CatDTInducer() { delete inducer; }
   virtual int class_id() const { return CatDT_INDUCER; }
   virtual void train(); 
   virtual Categorizer* best_split(MStringArray*& catNames) const;
   virtual TDDTInducer* create_subinducer(const MString& dscr,
                                          CGraph& aCgraph) const;
   // create_leaf_cat creates a leaf categorizer when splitting is done.
   virtual Categorizer* create_leaf_categorizer(int numInstances,
						Category preferredMajority) const;
   virtual Real get_sampling_rate() const { return samplingRate; }
   virtual void set_sampling_rate(Real minVal);
   virtual void set_cat_inducer(Inducer *& inducer);
   virtual const Inducer* get_cat_inducer() const { return inducer; }
   virtual void set_user_options(const MString& preFix);

   // this is not virtual. Each class has its one copy_options() method
   // and call its base class's copy_options() method.
   void copy_options(const CatDTInducer& inducer); 
};

#endif
