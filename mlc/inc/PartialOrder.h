// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.


#ifndef _PartialOrder_h
#define _PartialOrder_h 1

#include <LEDA/graph.h>
#include <Array.h>
#include <MLCStream.h>
#include <basics.h>

typedef node NodePtr;
typedef edge EdgePtr;
typedef Array<NodePtr> NodePtrArray;

class PartialOrder {
   graph ordering;
   NodePtrArray nodePtrArray;
   Bool changedSinceTC;


   NO_COPY_CTOR2(PartialOrder, nodePtrArray(0));
   void transitive_closure() const;
   
public:
   // Since this is defined within the class usage is PartialOrder::lessThan
   enum Relation {lessThan, greaterThan, noRelation};

   virtual void OK(int level = 0) const;
   PartialOrder(int numInOrder);
   virtual ~PartialOrder();
   // Sets a to be less than b
   virtual void set_less(int a, int b);

   // Returns appropriate TRANSITIVE relationship between two arguments
   virtual Relation get_relation(int a, int b) const;

   // Returns TRUE iff no node is less than the given one.
   virtual Bool is_minimal(int) const;
   // Removes the given node from all relations with other nodes.
   virtual void delete_min_node(int);
   virtual void display(MLCOStream& stream = Mcout) const;
};

#endif
