// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The OrderFSSState class represents a state in the search
                   space containing an ordered subset of features,
		   specifically for use with inducers which support
		   ordering.
  Assumptions  :
  Comments     :
  Complexity   :
  Enhancements :
  History      : Dan Sommerfield                                    6/04/95
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <OrderFSSState.h>
#include <ListHOODGInd.h>
#include <AttrOrder.h>
#include <Inducer.h>

//RCSID("MLC++, $RCSfile: OrderFSSState.c,v $ $Revision: 1.3 $")

/***************************************************************************
  Description : This function is called right before eval() invokes the
                  Inducer's train_and_test() function.  For this
		  inducer, we use it to set the ordering.
  Comments    :
***************************************************************************/
void OrderFSSState::pre_eval(AccEstInfo *globalInfo)
{
   AttrOrder& ao = globalInfo->inducer->cast_to_inducer().
                   get_attr_order_info();
   ao.set_order(get_info());
}

