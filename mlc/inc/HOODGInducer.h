// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _HOODGInducer_h
#define _HOODGInducer_h 1

#include <OODGInducer.h>
#include <DestArray.h>
#include <HOODGCIH.h>

class HOODGInducer : public OODGInducer {
   CoverInstanceHeuristic *coverInstanceHeuristic;
protected:
   Bool is_forced_new_bag(ProjInfoPtrList& pipl,
                          InstanceProjection& ip,
                          const NominalAttrInfo& deletedAttrInfo) const;
   void cover_instance(ProjInfoPtrList& pipl,
                       InstanceProjection& ip,
                       const NominalAttrInfo& deletedAttrInfo) const;
   Bool cover_contradicting_instances(ProjBag& projBag, 
                                      int createCount, ProjInfoPtrList& pipl, 
                                      ProjListPix& posPix) const;
public:
   HOODGInducer(const MString& dscr, CGraph* aCgraph = NULL);
   virtual ~HOODGInducer();
   virtual void init_train();
   virtual void end_train();
   void set_cover_instance_heuristic(CoverInstanceHeuristic &cih);
   CoverInstanceHeuristic& get_cover_instance_heuristic() const;
   virtual ProjInfoPtrList* find_cover(int attrNum,
                                       const BagPtrArray& bpa) const;
   virtual num_irrelevant(const ProjInfoPtrList& pipl);
   virtual int best_split(const BagPtrArray& bpa);
   virtual void prune_nodes(ProjInfoPtrList& pipl);
};
#endif
