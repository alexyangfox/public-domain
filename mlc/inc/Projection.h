// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _Projection_h
#define _Projection_h 1

#include <basics.h>
#include <DynamicArray.h>
#include <InstanceRC.h>
#include <InstList.h>
#include <UnivHashTable.h>
#include <RDGCat.h>
#include <CatGraph.h>
#include <FeatureSet.h>

class ProjLevel;

class ProjKey {
   // this class represents the key in the Projection hash table
   // can't use NO_COPY_CTOR because ProjInst needs it to let
   // DblLinkList instantiate correctly.
   InstanceRC instance;
   const FeatureSet& featureSet;

public:
   ProjKey(const InstanceRC& inst, const FeatureSet& set)
      : instance(inst), featureSet(set) { }

   // UniversalHashTable won't instantiate without having this function
   // as a public.  It should never be called.
   int length() const { ASSERT(FALSE); return 0; }
   
   const InstanceRC& get_instance() const { return instance; }
   
   // operator == needed for use as key type in hash table
   Bool operator==(const ProjKey& other) const;

   // display of key info
   void display(MLCOStream& stream = Mcout) const;
};

DECLARE_DISPLAY(ProjKey);

class ProjGraph;

class ProjInst {
   // a projected instance consists of an instance and a destination.
   // the instance is actually part of the key.
   // can't use NO_COPY_CTOR because DblLinkList won't instantiate without it.
   ProjKey key;
   Category destination;

public:
   ProjInst(const InstanceRC& inst, Category dest, const FeatureSet& set)
      : key(inst, set), destination(dest) { }
   
   Category get_destination() const { return destination; }
   void set_destination(Category newDest) { destination = newDest; }
   void display(const FeatureSet& instSet,
		const FeatureSet& projSet,
		MLCOStream& stream = Mcout) const;
   void display(MLCOStream& stream = Mcout) const;

   // contains_key and get_key needed for use as datatype in hash table
   Bool contains_key(const ProjKey& otherKey) const {
      return (key == otherKey); }
   const ProjKey& get_key() const { return key; }
};

DECLARE_DISPLAY(ProjInst);


// Projection is implemented by keeping a hash table from instances to
// destinations.  We also keep a list of instances for iteration.
class Projection : private UniversalHashTable<ProjKey, ProjInst> {
   NO_COPY_CTOR(Projection);

   // Keep a reference to the level this projection belongs to.
   // Note that this is a mutual reference so it isn't const
   ProjLevel& owner;

   InstanceList iterList;
   InstanceRC mainInstance;
   Projection *source;

   // inherited from hash table as private
   virtual int hash(const ProjKey& instance) const;

public:
   void OK() const;
   void check_consistency(const SchemaRC& schema,
			  const ProjLevel *apparentSource,
			  const ProjLevel *apparentDest) const;

   Projection(const InstanceBag& bag,
	      ProjLevel &itsOwner,
	      const InstanceRC& mainInst,
	      Projection *source = NULL);
   
   void add_instance(const InstanceRC& inst, Category dest);
   Bool instance_belongs(const InstanceRC& inst) const;
   Bool conflicts_with(const Projection& other) const;
   void update_source(Category newDest);

   DynamicArray<Projection *> *project_down(const FeatureSet& selector,
					    ProjLevel& dest) const;
   void fill_destinations(const Categorizer& cat, Array<int>& destArray,
			  Array<int>& counts);
   
   // note: only the features of mainInstance specified by instSet are
   // meaningful.
   const InstanceRC& get_main_instance() const { return mainInstance; }
   SchemaRC get_schema() const { return iterList.get_schema(); }
   
   void display_main_instance(MLCOStream& stream = Mcout) const;
   void display(MLCOStream& stream = Mcout) const;   
};

DECLARE_DISPLAY(Projection);

#endif






