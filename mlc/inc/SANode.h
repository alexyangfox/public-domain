// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _SANode_h
#define _SANode_h 1

#include <StateSpace.h> // for NodePtr

class SANode {
public:
   NodePtr node;
   int numEvaluations;
   Bool isExpanded;
   Real fitness;

   SANode();
   SANode(NodePtr nodePtr, Real initFitness);
   virtual ~SANode() {}

   Bool operator<(const SANode& compareNode) const;
   Bool operator==(const SANode& compareNode) const;

};


#endif
