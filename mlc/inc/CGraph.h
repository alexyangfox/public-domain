// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _CGraph_h
#define _CGraph_h 1

#include <error.h>

// WARNING: Clear must be defined BEFORE #include <LEDA/graph.h>
// Let LEDA use our clear procedure to avoid OC warning when
// deleting the GRAPH.  Note the functions have precedence over 
// templates (Stroustrup page 597).
class AugCategory;
class ostream;
class Categorizer;
class NodeInfo;
inline void Clear(NodeInfo*& x) { x = NULL; } // already deallocated.
inline void Clear(AugCategory*& x) { x = NULL; } // already deallocated.


// used to print edge labels in LEDA's print routine
void Print(const AugCategory* ac, ostream& stream);
void Print(const NodeInfo* nodeInfo, ostream& stream);


#include <LEDA/graph.h>
#include <DblLinkList.h>

extern const int defaultLevel;

typedef node NodePtr;  // LEDA's node is really a pointer to a node
typedef edge EdgePtr;  // LEDA's edge is really a pointer to an edge

class NodeInfo {
   Categorizer* cat;
   const int lev;
   DblLinkList<NodePtr> parents;
   NO_COPY_CTOR(NodeInfo);

public:
   void OK(int okLevel = 0) const;
   // Sets categorizer to badCategorizer
   NodeInfo(int levl);
   NodeInfo(Categorizer*& categorizer, int levl);
   NodeInfo(Categorizer*& categorizer, int levl,
	    const DblLinkList<NodePtr>& par);
   NodeInfo(const NodeInfo& info, CtorDummy);
   ~NodeInfo();
   int level() const {return lev;}
   // Gets ownership of categorizer
   void set_categorizer(Categorizer*& categorizer);
   const Categorizer& get_categorizer() const {return *cat;}
   // non-const version of get_categorizer()
   Categorizer& get_categorizer() { return *cat; }
   // Add and delete par to the parents list.
   void add_parent(NodePtr par);
   Bool del_parent(NodePtr par);
   // Returns the parents list.
   const DblLinkList<NodePtr>& get_parents() const { return parents; }

   virtual NodeInfo *create_my_type(int level) const
       { return new NodeInfo(level); }
   virtual NodeInfo *clone() const { return new NodeInfo(*this, ctorDummy); }
   virtual int class_id() const { return -1; }
};


template<class V, class E> class GRAPH;
typedef GRAPH<NodeInfo*, AugCategory*> CGRAPH;


class CGraph : public CGRAPH {
   NO_COPY_CTOR(CGraph);
   NodeInfo *prototype;   // used to build correct NodeInfo types when
                          // building graph, by using NodeInfo's
                          // create_my_type member function.
   
public:
   virtual void OK(int level=0);
   CGraph();
   CGraph(NodeInfo *& proto);
   CGraph(const CGraph& source, CtorDummy);
   virtual ~CGraph();
   virtual int num_leaves() const;
   // Update and access function for parent nodes.
   virtual void add_parent(NodePtr n, NodePtr par);
   virtual void del_parent(NodePtr n, NodePtr par);
   virtual const DblLinkList<NodePtr>& get_parents(NodePtr n) const;
   EdgePtr new_edge(NodePtr v, NodePtr w, AugCategory* x);
   void del_edge(EdgePtr e);
   virtual NodeInfo *get_prototype() const { return prototype; }

   // "index" is a friend function usable only within this class.
   // node_index does the same thing but is a member function.
   int node_index(NodePtr whichNode) const { return index(whichNode); }
   
private:
   virtual Bool is_child(NodePtr c, NodePtr p);
   virtual Bool is_parent(NodePtr p, NodePtr c);

};

#endif
