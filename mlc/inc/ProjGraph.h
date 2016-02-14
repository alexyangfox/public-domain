// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _ProjGraph_h
#define _ProjGraph_h 1

#include <basics.h>
#include <DblLinkList.h>
#include <LEDA/graph.h>
#include <MLCStream.h>
#include <DisplayPref.h>
#include <LogOptions.h>
#include <ProjLevel.h>

// LEDA functions
void Print(const ProjSet*& nodeInfo, ostream& stream);
void Read (const ProjSet*& nodeInfo, istream& stream);

typedef node NodePtr;
typedef edge EdgePtr;

class ProjGraph : public GRAPH<ProjSet*, int> {
   LOG_OPTIONS;
   
   void process_DotPoscript_preferences(MLCOStream& stream,
					const DisplayPref& pref) const;
   DynamicArray<NodePtr> nodeArray;
   
protected:
   void free();
   virtual void process_XStream_display(const DisplayPref& dp) const;
   virtual void process_DotPostscript_display(MLCOStream& stream,
					      const DisplayPref& dp) const;
   virtual void process_DotGraph_display(MLCOStream& stream,
					 const DisplayPref& dp) const;
   virtual void convertToDotFormat(MLCOStream& stream,
				   const DisplayPref& pref)const;

   void init_coloring(int maxColors);
   void assign_color(NodePtr n, int color);
   NodePtr get_node_to_color();
   
   
public:
   ProjGraph();
   virtual ~ProjGraph();
   virtual MString graph_options(NodePtr v) const;

   // Graph Manipulation
   NodePtr create_node(ProjSet*& proj);
   void connect(NodePtr from, NodePtr to, int edge);
   void add_conflict_edges();

   // Accessor Functions.
   ProjSet& get_proj_set(NodePtr node) const;

   // Graph Coloring
   Bool color(int maxColors);

   // Display Functions
   virtual void display(MLCOStream& stream = Mcout,
                        const DisplayPref& dp = defaultDisplayPref) const;

   // Function to retrieve colored sets
   Array<ProjSet *> *get_colored_sets(int max);
};

DECLARE_DISPLAY(ProjGraph);   


#endif











