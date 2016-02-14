// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the Partial Order class.  Note that the LEDA function
                    TRANSITIVE_CLOSURE() leaks.
  Doesn't test :
  Enhancements :
  History      : Richard Long                                       2/01/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <PartialOrder.h>

RCSID("MLC++, $RCSfile: t_PartialOrder.c,v $ $Revision: 1.5 $")


main()
{
   cout << "t_PartialOrder executing" << endl;
   PartialOrder po(5);
   po.set_less(1, 2);
   po.display();
   ASSERT(po.get_relation(2, 1) == PartialOrder::greaterThan);
   ASSERT(po.get_relation(1, 2) == PartialOrder::lessThan);
   ASSERT(po.get_relation(1, 3) == PartialOrder::noRelation);

   po.set_less(2, 4);
   po.set_less(0, 3);
   ASSERT(po.get_relation(1, 4) == PartialOrder::lessThan);
   ASSERT(po.is_minimal(0));
   po.display();
   po.delete_min_node(0);
   ASSERT(po.get_relation(0, 3) == PartialOrder::noRelation);
   po.display();

#ifndef MEMCHECK
   PartialOrder* badPO = new PartialOrder(2);
   TEST_ERROR("is out of bounds", badPO->set_less(10, 11));
   TEST_ERROR("less than itself", badPO->set_less(1, 1));
   badPO->set_less(0, 1);
   TEST_ERROR(" is not minimal", badPO->delete_min_node(1));
   badPO->set_less(1, 0);
   TEST_ERROR("PartialOrder::OK: The partial order contains a cycle",
	      delete badPO);
#endif
   
   return 0; // return success to shell
}   
