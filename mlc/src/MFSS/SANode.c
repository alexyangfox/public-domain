// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : 
  Assumptions  :
  Comments     :
  Complexity   :
  Enhancements :
  History      : Brian Frasca                                       5/21/94
                   Initial revision (.h,.c)
***************************************************************************/
#include <basics.h>
#include <SANode.h>

RCSID("MLC++, $RCSfile: SANode.c,v $ $Revision: 1.2 $")


/***************************************************************************
  Description : Constructors.
  Comments    :
***************************************************************************/
SANode::SANode()
{
   node = NULL;
   numEvaluations = 1;
   isExpanded = FALSE;
   fitness = 0;
}

SANode::SANode(NodePtr nodePtr, Real initFitness)
{
   ASSERT(nodePtr != NULL);
   node = nodePtr;
   numEvaluations = 1;
   isExpanded = FALSE;
   fitness = initFitness;
}


/***************************************************************************
  Description : These comparison operators enable Array's sort function.
                SANode are sorted in order of fitness.
  Comments    :
***************************************************************************/
Bool SANode::operator<(const SANode& compareNode) const
{
   return fitness > compareNode.fitness;
}

Bool SANode::operator==(const SANode& compareNode) const
{
   return fitness == compareNode.fitness;
}

   
