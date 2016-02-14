// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : CGraph is derived from LEDA's CGRAPH<NodeInfo*, AugCategory*>
                 This allows us to add functions.
  Assumptions  :
  Comments     :
  Complexity   : CGraph::num_leaves() takes O(N), where N is the
                     number of nodes in the graph.
  Enhancements :
  History      : Chia-Hsin Li                                       11/21/94
                   Fixed set_categorizer which leaked.
                 Richard Long                                       1/28/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <CGraph.h>
#include <BadCat.h>
#include <AugCategory.h>

RCSID("MLC++, $RCSfile: CGraph.c,v $ $Revision: 1.14 $")

SET_DLLPIX_CLEAR(NodePtr, NULL);

const int defaultLevel = -1;


/***************************************************************************
  Description : Does nothing
  Comments    : Cannot check categorizer for being a bad categorizer
                  because bad categorizer is occasionally valid for
		  a NodeInfo (such as a prototype).
***************************************************************************/
void NodeInfo::OK(int level) const
{
   (void)level;
}


/***************************************************************************
  Description : Constructors and destructor.
  Comments    : 
***************************************************************************/
// Sets categorizer to badCategorizer
// Note that a NodeInfo created with this contructor is not OK()
NodeInfo::NodeInfo(int levl) : lev(levl)
{
   cat = &badCategorizer;
}

// gets ownership of categorizer.
NodeInfo::NodeInfo(Categorizer*& categorizer, int levl) : lev(levl)
{
   cat = NULL; // so that we can do checks in set_categorizer
   set_categorizer(categorizer);
   ASSERT(categorizer == NULL); // set_categorizer() gets ownership
}

// gets ownership of categorizer.
NodeInfo::NodeInfo(Categorizer*& categorizer, int levl,
		   const DblLinkList<NodePtr>& par) :
   lev(levl), parents(par,ctorDummy)
{
   cat = NULL; // so that we can do checks in set_categorizer
   set_categorizer(categorizer);
   ASSERT(categorizer == NULL); // set_categorizer() gets ownership
}

/***************************************************************************
  Description : Copy constructor
  Comments    :
***************************************************************************/
NodeInfo::NodeInfo(const NodeInfo& info, CtorDummy)
   : cat(NULL),
     lev(info.lev),
     parents(info.parents, ctorDummy)
{
   if (BadCategorizer::is_bad_categorizer(*(info.cat)))
      cat = &badCategorizer;
   else
      cat = info.cat->copy();
}


NodeInfo::~NodeInfo()
{
   DBG(OK());
   // Don't delete badCategorizer
   if (!BadCategorizer::is_bad_categorizer(*cat))
      delete cat;
}


/***************************************************************************
  Description : Sets node info to point to given categorizer.
  Comments    : Gets ownership of categorizer.
***************************************************************************/
void NodeInfo::set_categorizer(Categorizer*& categorizer)
{
   if (categorizer == NULL)
      err << "NodeInfo::set_categorizer: Cannot set node to NULL categorizer"
	  << fatal_error;

   if (cat !=NULL)
      if (!BadCategorizer::is_bad_categorizer(*cat))
	 delete cat;

   cat = categorizer;
   categorizer = NULL;
}

/***************************************************************************
  Description : Add and delete a node to the parents list of a node
  Comments    : del_parents() returns FALSE while the elements is not found.
***************************************************************************/
void NodeInfo::add_parent(NodePtr par)
{
   parents.append(par);
}

Bool NodeInfo::del_parent(NodePtr par)
{
   for (DLLPix<NodePtr> pix(parents,1); pix; pix.next()) {
      if (parents(pix) == par) {
	 parents.del(pix, 1);
	 return TRUE;
      }
   }
   return FALSE;
}




/***************************************************************************
  Description : Prints a readable representation of the categorizer/
                  category to the given ostream.
  Comments    : This function is required for LEDA's print() routine
                  which is called by CatGraph::display() (for ASCII output).
		Only prints the node's level if it is not defaultLevel.
***************************************************************************/
void Print(const NodeInfo* nodeInfo, ostream& stream)
{
  nodeInfo->get_categorizer().short_display(stream);
  if (nodeInfo->level() != defaultLevel)
     stream << ", level " << nodeInfo->level();
}


void Print(const AugCategory* ac, ostream& stream)
{
   stream << ac->description();
}


/***************************************************************************
  Description : OK function of CGraph.  Also calls OK() of nodes.
  Comments    :
***************************************************************************/
void CGraph::OK(int level)
{
   NodePtr iterNode;

   forall_nodes(iterNode, *this) {
      inf(iterNode)->OK(level);
      const DblLinkList<NodePtr>& parentsList = get_parents(iterNode);
      //each node pointed to by the parents DblLinkList actually points back
      //to us.
      for (DLLPix<NodePtr> pix(parentsList,1); pix; pix.next()) {
	 NodePtr parentNode = parentsList(pix);
	 if (!is_child(iterNode, parentNode))
	    err << "CGraph::OK: A node in the parent DblLinkList doesn't "
		<< "point back to us" << fatal_error;
      }
      //each node we point to also has us in the DblLinkList
      NodePtr childNode;
      forall_adj_nodes(childNode, iterNode) {
	 if (!is_parent(iterNode, childNode))
	    err << "CGraph::OK: A parent isn't in the the parent list of "
		<< "one of its children" << fatal_error;
      }
   }
}


/***************************************************************************
  Description : Constructor.  We allow passing of an optional NodeInfo
                  to use as a prototype for all NodeInfo's used by this
		  graph.  This feature allows subclasses of NodeInfo to
		  be correctly propagated throughout the graph.
		The graph gets ownership of the prototype.
  Comments    :
***************************************************************************/
CGraph::CGraph()
   : CGRAPH(),
     prototype(new NodeInfo(0))
{
}

CGraph::CGraph(NodeInfo*& proto)
   : CGRAPH(),
     prototype(proto)
{
   proto = NULL;
}

/***************************************************************************
  Description : Copy Constructor.
  Comments    :
***************************************************************************/
CGraph::CGraph(const CGraph& source, CtorDummy /*dummyArg*/)
   : CGRAPH(source),
     prototype(source.prototype->clone())
{
   // Make a deep copy of all of the NodeInfo
   NodePtr nodePtr;
   forall_nodes(nodePtr, *this)
      (*this)[nodePtr] = (*this)[nodePtr]->clone();

   // Update the parent lists.  First remove the existing lists.
   forall_nodes(nodePtr, *this) {
      const DblLinkList<NodePtr> parents(get_parents(nodePtr), ctorDummy);
      for (DLLPix<NodePtr> pix(parents,1); pix; pix.next()) 
	 del_parent(nodePtr, parents(pix));
      ASSERT(get_parents(nodePtr).empty());
   }
   // Create new parent lists pointing to new nodes.
   NodePtr childPtr;
   forall_nodes(nodePtr, *this) {
      forall_adj_nodes(childPtr, nodePtr) 
	 add_parent(childPtr, nodePtr);
   }

   // Make a deep copy of all of the AugCategory
   EdgePtr edgePtr;
   forall_edges(edgePtr, *this)
      (*this)[edgePtr] = new AugCategory((*this)[edgePtr]->num(),
					 (*this)[edgePtr]->description());
   OK(1);
}

/***************************************************************************
  Description : Destructor.
  Comments    :
***************************************************************************/
CGraph::~CGraph()
{
   OK();
   NodePtr nodePtr;
   forall_nodes(nodePtr, *this)
      delete inf(nodePtr);
   EdgePtr edgePtr;
   forall_edges(edgePtr, *this)
      delete inf(edgePtr);
   delete prototype;
}


/***************************************************************************
  Description : Returns the number of leaves in the RootedCatGraph.
  Comments    :
***************************************************************************/
int CGraph::num_leaves() const
{
   int numLeaves = 0;
   NodePtr nodePtr;
   forall_nodes(nodePtr, *this)
      if (outdeg(nodePtr) == 0)
	 numLeaves++;
   
   return numLeaves;
}


/***************************************************************************
  Description : is_child() returns TRUE if c is a child node of p.
                is_parent() returns TRUS if p is a parent node of c.
  Comments    : Private. Helper function of OK().
***************************************************************************/
Bool CGraph::is_child(NodePtr c, NodePtr p)
{
   NodePtr iterNode;
   forall_adj_nodes(iterNode, p) 
      if (c == iterNode) {
	 graph_of(p)->init_adj_iterator(p);
	 return TRUE;
      }
   return FALSE;
}

Bool CGraph::is_parent(NodePtr p, NodePtr c)
{
   const DblLinkList<NodePtr> &parentsList = get_parents(c);
   for (DLLPix<NodePtr> pix(parentsList,1); pix; pix.next()) {
      if (p == parentsList(pix))
	 return TRUE;
   }
   return FALSE;
}

/***************************************************************************
  Description : add_parent() adds an element to the parent list of a node
                del_parent() deletes an element from the parent list of a node
                get_parent() returns the parent DblLinkList of n.
  Comments    :
***************************************************************************/
void CGraph::add_parent(NodePtr n, NodePtr par)
{
   inf(n)->add_parent(par);
}

void CGraph::del_parent(NodePtr n, NodePtr par)
{
   if (!inf(n)->del_parent(par))
      err << "CGraph::del_parent: Node not found in parent list.";
}

const DblLinkList<NodePtr>& CGraph::get_parents(NodePtr n) const 
{
   return inf(n)->get_parents();
}

/***************************************************************************
  Description : Adds and delete an edge to graph.
  Comments    : Override the functions in LEDA
***************************************************************************/
EdgePtr CGraph::new_edge(NodePtr v, NodePtr w, AugCategory* x)
{
   add_parent(w, v);
   return CGRAPH::new_edge(v,w,x);
}

void CGraph::del_edge(EdgePtr e)
{
   NodePtr p = source(e);
   NodePtr c = target(e);
   del_parent(c, p);
   CGRAPH::del_edge(e);
}









