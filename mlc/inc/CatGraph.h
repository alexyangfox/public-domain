// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "Graph.c".

#ifndef _CatGraph_h
#define _CatGraph_h 1

#include <MLCStream.h>
#include <DisplayPref.h>
#include <CGraph.h>
#include <Attribute.h>

class AugCategory;

extern const int defaultLevel;

class CatGraph {
   NO_COPY_CTOR(CatGraph);
// must be visible to subclasses for them to uses LEDA methods
protected:
   CGraph& cGraph;
   Bool graphAlloc;

   virtual void process_XStream_display(const DisplayPref& dp) const;
   virtual void process_DotPostscript_display(MLCOStream& stream,
					      const DisplayPref& dp) const;
   virtual void process_DotGraph_display(MLCOStream& stream,
					 const DisplayPref& dp) const;
   virtual void convertToDotFormat(MLCOStream& stream,
				   const DisplayPref& pref)const;
public:
   virtual void OK(int level = 1) const;
   CatGraph();
   CatGraph(NodeInfo*& prototype);
   CatGraph(CGraph&); // using caller's CGraph (no copy is made)
   CatGraph(const CatGraph& source, CtorDummy);
   virtual ~CatGraph();
   CGraph& get_graph() const;
   NodePtr create_node(int level = defaultLevel);
   // gets ownership of Categorizer
   NodePtr create_node(Categorizer*&, int level = defaultLevel);
  // gets ownership of Categorizer
   virtual void set_categorizer(const NodePtr, Categorizer*&);
   const Categorizer& get_categorizer(const NodePtr) const;
   int level(const NodePtr) const;
   // gets ownership of AugCategory
   virtual void connect(const NodePtr from, const NodePtr to, AugCategory*&);
   int num_children(const NodePtr parent) const;
   virtual NodePtr get_child(const NodePtr parent,
				   const AugCategory&) const;
   virtual void display(MLCOStream& stream = Mcout,
			const DisplayPref& dp = defaultDisplayPref) const;
   Bool check_reachable(const NodePtr source, 
			Bool fatalOnFalse=FALSE)const;
   Bool check_tree(const NodePtr root, Bool fatalOnFalse = FALSE)const;
   Bool check_node_in_graph(const NodePtr nodePtr, 
			    Bool fatalOnFalse = FALSE) const;
   virtual int num_nodes() const;
   virtual int num_leaves() const;
   virtual const AugCategory* get_edge(const NodePtr nodePtr,
				       Category edgeCategory) const;
};

DECLARE_DISPLAY(CatGraph);

#endif
