// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _ProjSet_h
#define _ProjSet_h 1

#include <basics.h>
#include <DynamicArray.h>
#include <InstanceRC.h>
#include <InstList.h>
#include <UnivHashTable.h>
#include <RDGCat.h>
#include <CatGraph.h>
#include <Projection.h>


class ProjSet {
   NO_COPY_CTOR(ProjSet);
   DynamicArray<Projection *> projArray;
   int color;   // used for graph coloring
   int maxColors;  // maximum colors allowed
   int colorsUsed; // number of colors in use
   BoolArray *nogoods;  // used for graph coloring

   NodePtr node;   // node pointer used for CatGraph
   int idNum;      // identifying number
   int displayLevel;  // level of .dot output requested
   
public:
   void OK() const;
   void check_consistency(const SchemaRC& schema,
			  const ProjLevel *apparentSource,
			  const ProjLevel *apparentDest) const;
   
   ProjSet(int displayLev = 0);
   ProjSet(Projection*& proj, int displayLev = 0);
   ~ProjSet();

   void add(Projection*& proj);
   void add(ProjSet*& projSet);
   int conflicts_with(const ProjSet& other) const;
   void set_color(int col);
   int get_color() const { return color; }
   int colors_left() const { return maxColors - colorsUsed; }
   void add_nogood(int col);
   int get_first_ok_color() const;
   void reset(int max);
   void update_source(Category newDest);
   void set_node(NodePtr n) { node = n; }
   void set_id(int n) { idNum = n; }
   NodePtr get_node() const { return node; }
   
   Array<ProjSet *> *project_down(const FeatureSet& set,
				  ProjLevel& dest);
      
   Array<ProjSet *> *split_by_attribute(int attrNum);

   void connect_to_level(CatGraph& graph, ProjLevel& level, int attr,
			 const SchemaRC& schema);
   void connect_to_leaves(CatGraph& graph, Array<NodePtr>& leaves, int attr,
			  const SchemaRC& schema);

   InstanceBag *get_instances() const;
   
   void display(MLCOStream& stream) const;
   void display_for_graph(MLCOStream& stream) const;
};

DECLARE_DISPLAY(ProjSet);


#endif






