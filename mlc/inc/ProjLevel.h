// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _ProjLevel_h
#define _ProjLevel_h 1

#include <basics.h>
#include <DynamicArray.h>
#include <InstanceRC.h>
#include <InstList.h>
#include <UnivHashTable.h>
#include <RDGCat.h>
#include <CatGraph.h>
#include <Projection.h>
#include <ProjSet.h>

class ProjLevel {
   NO_COPY_CTOR(ProjLevel);

   // static number for dumping coloring info
   static int colorIndex;

   // keep track of an array of ProjSets as well
   DynamicArray<ProjSet *> projSets;

   // These are index selectors for the indices we're projecting on,
   // the indices representing the projected instance, and the
   // indices which we're not using at this level of the projection.
   FeatureSet projIndices;
   FeatureSet instIndices;
   FeatureSet notInstIndices;

   // Level information used for consistency checks
   SchemaRC levelSchema;
   const ProjLevel *levelSource;
   const ProjLevel *levelDest;

   // display level: controls how much .dot output is created
   int displayLevel;
   
   
public:
   void OK() const;
   
   // constructor #1: build the toplevel ProjLevel from a single bag
   ProjLevel(const InstanceBag& bag, int displayLev = 0);

   // constructor #2: build the toplevel ProjLevel from a single bag
   // while setting the projected portion at will.
   ProjLevel(const InstanceBag& bag, const FeatureSet& projSet,
	     int displayLev = 0);

   // constructor #3: build a projlevel by projecting another level down
   // by the attributes in selector.  Allow specification of a lower
   // level to constrain the projection (i.e. interpolation)
   ProjLevel(const ProjLevel& level, const FeatureSet& projectBy,
	     const ProjLevel *lower = NULL);

   ~ProjLevel();
   
   // accessors for feature sets
   const FeatureSet& inst_features() const { return instIndices; }
   const FeatureSet& dest_features() const { return projIndices; }
   const FeatureSet& not_inst_features() const {
      return notInstIndices; }

   ProjGraph *create_conflict_graph();
   void rebuild_sets(ProjGraph *graph, int num_colors);
   int color_level(int numColors);
   void special_display(MLCOStream& out) const;
   
   void update_source(void);
   Bool feature_set_match(const FeatureSet *instPtr,
			  const FeatureSet *projPtr) const {
      return (instPtr == &instIndices && projPtr == &projIndices);
   }
   SchemaRC get_schema() { return levelSchema; }

   // calls to fill in sections of the CatGraph
   void insert_in_graph(CatGraph& graph);
   void connect_to_level(CatGraph& graph, ProjLevel& other,
			 const SchemaRC& schema);
   void connect_to_leaves(CatGraph& graph, const SchemaRC& schema);

   // call to redirect this ProjLevel's projection to the next level.
   // call BEFORE coloring for bottom-up approaches
   void direct_to_level(const ProjLevel& other);
   
   Bool is_bottom_level() const { return notInstIndices.num_features() == 0; }
   Bool is_top_level() const { return instIndices.num_features() == 0; }
   NodePtr get_node(int which) const { return projSets[which]->get_node(); }
   NodePtr get_toplevel_node() const;
   int get_width() const { return projSets.size(); }
   
   void display(MLCOStream& stream = Mcout) const;
   
};

DECLARE_DISPLAY(ProjLevel);

// prune a graph
void prune_graph(CatGraph& catG);

#endif






