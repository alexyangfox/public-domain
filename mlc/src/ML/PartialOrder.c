// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : This class maintains a partial order among
  Assumptions  :
  Comments     :
  Complexity   : In the complexity measurements, V is the number of
                    items in the partial order, and E is the number of
		    edges in the graph representing the transitive
		    closure of the partial order.
                 OK() takes O(V+E).
		 PartialOrder() constructor takes O(V).
		 get_relation() takes O(V*E).
		 is_minimal() takes O(V*E).
		 delete_min_node() takes O(V*E).
		 Unless calls to set_less() or delete_min_node() are called,
		    subsequent calls to get_relation() or is_minimal() take
		    O(1).
  Enhancements :
  History      : Richard Long                                       2/01/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <PartialOrder.h>
#include <LEDA/graph_alg.h>
#include <DLList.h>

RCSID("MLC++, $RCSfile: PartialOrder.c,v $ $Revision: 1.7 $")

/***************************************************************************
  Description : Finds the transitive closure of the partial order and
                   removes all edges from a node to itself.
  Comments    : Private method.
                This is const because it just transitively extends the
		   relationships established by set_less().
		Takes O(V*E), where V is the number of nodes and E is
		   the number of edges in "ordering".
***************************************************************************/
void PartialOrder::transitive_closure() const
{
   // cast away constness.
   ((PartialOrder*)this)->ordering = TRANSITIVE_CLOSURE(ordering);

   // Delete all edges that have a node pointing to itself.
   // The edges must be stored in an array before deletion because
   // edges cannot be deleted while an iterator is active.  Must cast
   // away constness.
   Array<EdgePtr> edgeArray(nodePtrArray.size());
   int i = 0;
   NodePtr nodePtr;
   forall_nodes(nodePtr, ordering) {
      EdgePtr edgePtr;
      forall_adj_edges(edgePtr, nodePtr)
	 if (ordering.target(edgePtr) == nodePtr)
	    edgeArray[i] = edgePtr;
      i++;
   }
   
   for (i = 0; i < edgeArray.size(); i++)
      ((PartialOrder*)this)->ordering.del_edge(edgeArray[i]);
   
   // Now we have to update the nodePtrArray to point to the nodes in
   // the new graph.  Luckily, LEDA keeps them in the same order.
   i = 0;
   forall_nodes(nodePtr, ordering) {
      ((PartialOrder*)this)->nodePtrArray[i] = nodePtr;
      i++;
   }
   ASSERT(i == nodePtrArray.size());
   OK();  // The time for OK() in negligible compared to TRANSITIVE_CLOSURE
}


/***************************************************************************
  Description : Checks that there are no cycles in the graph.
  Comments    :
***************************************************************************/
void PartialOrder::OK(int /*level*/) const
{
   node_array<int> ord(ordering); // required argument for TOPSORT
   if (!TOPSORT(ordering, ord))
      err << "PartialOrder::OK: The partial order contains a cycle"
	  << fatal_error;
}


/***************************************************************************
  Description : Constructor.
                The partial order consists of the integers in the
		   range [0, numInOrder). 
  Comments    : 
***************************************************************************/
PartialOrder::PartialOrder(int numInOrder)
   : nodePtrArray(numInOrder), changedSinceTC(FALSE)
{
   for (int i = 0; i < nodePtrArray.size(); i++)
      nodePtrArray[i] = ordering.new_node();
}


/***************************************************************************
  Description : Destructor.
  Comments    : Nodes are deleted by the graph destructor.
***************************************************************************/
PartialOrder::~PartialOrder()
{
   DBG(OK());
}


/***************************************************************************
  Description : Sets a < b.
  Comments    : a must not equal b.
***************************************************************************/
void PartialOrder::set_less(int a, int b)
{
   if (a == b)
      err << "PartialOrder::set_less: Cannot make " << a
	  << " less than itself" << endl;
   // Range checking done by Array.
   ordering.new_edge(nodePtrArray[b], nodePtrArray[a]);
   changedSinceTC = TRUE;
}


/***************************************************************************
  Description : Returns TRANSITIVE relationship between the two arguments.
  Comments    : This method is const because it does not change the
                   structure in any way visible to the user.  It does
		   update internal structures, but they are not
		   logical changes to the object.
***************************************************************************/
PartialOrder::Relation PartialOrder::get_relation (int a,
						   int b) const
{
   if (changedSinceTC)
      transitive_closure();
   NodePtr nodePtr;
   forall_adj_nodes(nodePtr, nodePtrArray[b])
      if (nodePtr == nodePtrArray[a])
	 return lessThan;
   // Reverse the ordering so that we can check for greater than
   // Cast away constness, because it is reversed again before exiting.
   ((PartialOrder*)this)->ordering.rev();

   // Cannot return from within the if and reverse the edges, because
   // reversing the edges cannot be done while an iterator is active.
   Relation relation = noRelation;
   forall_adj_nodes(nodePtr, nodePtrArray[b])
      if (nodePtr == nodePtrArray[a]) {
	 relation = greaterThan;
      }
   // Restore original ordering.  Cast away constness.
   ((PartialOrder*)this)->ordering.rev();
   return relation;
}


/***************************************************************************
  Description : Returns TRUE iff no node is less than the given one.
  Comments    :
***************************************************************************/
Bool PartialOrder::is_minimal(int num) const
{
   if (changedSinceTC)
      transitive_closure();
   return (ordering.outdeg(nodePtrArray[num]) == 0);
}


/***************************************************************************
  Description : Removes the given node from all relations with other nodes.
                   (i.e. get_relation() will now return noRelation for
		    the given nodes).
  Comments    : Node must be minimal.
***************************************************************************/
void PartialOrder::delete_min_node(int num)
{
   if (!is_minimal(num))
      err << "PartialOrder::delete_min_node: Node " << num << " is not "
	     "minimal" << fatal_error;
   // Cannot delete edges while iterator is active.
   // Find edges that point to the minimal node
   DLList<EdgePtr> edgePtrList;
   EdgePtr edgePtr;
   forall_edges(edgePtr, ordering)
      if (ordering.target(edgePtr) == nodePtrArray[num])
	 edgePtrList.append(edgePtr);
   while (edgePtrList.length() > 0)
      ordering.del_edge(edgePtrList.remove_front());
   changedSinceTC = TRUE;
}


/***************************************************************************
  Description : Uses Leda's display routine to show a graph of the
                   partial order.
  Comments    :
***************************************************************************/
void PartialOrder::display(MLCOStream& stream) const
{
   ordering.print(stream.get_stream());
   stream << flush;
}
