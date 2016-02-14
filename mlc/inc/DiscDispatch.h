// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _DiscDispatch_h
#define _DiscDispatch_h 1

#include <basics.h>
#include <Array.h>
#include <RealDiscretizor.h>
#include <BinningDisc.h>
#include <EntropyDisc.h>
#include <OneR.h>
#include <C45Disc.h>
#include <T2Disc.h>
#include <GetOption.h>

class DiscDispatch{
public:
   enum DiscretizationType { oneR, binning, entropy, c45Disc, t2Disc};
   // AlgoHeuristic means algorithm specific.
   enum NumBins { AlgoHeuristic, FixedValue, MDL};
private:
   LOG_OPTIONS;
   NO_COPY_CTOR(DiscDispatch);
   Array<int>* discVect;
   SchemaRC* schema;
   PtrArray<RealDiscretizor*>* disc;
   DiscretizationType discType;
   NumBins binHeuristic;
   void set_disc_vector(const InstanceBag& bag);
   int minSplit;                  //used for entropy
   int initVal;                   //initial discretization value
   int minInstPerLabel;           //used for 1R
   void allocate(const InstanceBag& bag);
   void free();
public:
   DiscDispatch();
   ~DiscDispatch() { free(); }
   virtual void display(MLCOStream& stream = Mcout) const;
   void set_user_options(const MString& prefix);
   void set_disc_vect(Array<int>*& dv); //gets ownership
   const Array<int>* disc_vect();
   PtrArray<RealDiscretizor*>* discretizors();
   void OK(int level = 0) const;
   void set_bin_heuristic(NumBins h);
   NumBins get_bin_heuristic() const {return binHeuristic;}
   int get_initial_val() const { return initVal;}
   int get_min_split() const { return minSplit;}
   int get_min_inst() const { return minInstPerLabel;}
   DiscretizationType get_disc_type() const { return discType;}
   void set_initial_val(int newVal);
   void set_min_split(int split){ minSplit = split;}
   void set_min_inst(int min) {minInstPerLabel = min;}
   void set_disc_type(DiscretizationType discType);
   InstanceBag* discretize_bag(const InstanceBag& bag); 
   void create_discretizors(const InstanceBag& bag); //must call first
   PtrArray<RealDiscretizor*>*  disc_copy(); 

};

DECLARE_DISPLAY(DiscDispatch);

#endif

