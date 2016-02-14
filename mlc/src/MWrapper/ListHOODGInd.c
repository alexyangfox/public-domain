// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : A version of HOODG which uses an ordering specified from
                   the outside via the set_order function to determine the
		   order in which to project attriubtes.
  Assumptions  :
  Comments     : 
  Complexity   : 
  Enhancements : 
  History      : Dan Sommerfield and Ronny Kohavi                 5/31/95
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <ListHOODGInd.h>
#include <DestArray.h>
#include <ConstCat.h>
#include <AttrCat.h>

RCSID("MLC++, $RCSfile: ListHOODGInd.c,v $ $Revision: 1.6 $")


/***************************************************************************
  Description : Constructor
  Comments    :
***************************************************************************/
ListHOODGInducer::ListHOODGInducer(const MString& dscr,
				   CGraph* aCgraph)
   : HOODGInducer(dscr, aCgraph),
     tmpOrder(NULL),
     orderPosition(-1)
{
}

/***************************************************************************
  Description : train (see OODGInducer)
                We project the dataset onto the subset of features given,
		  then exchange the bag for training and restore when done.
		  The map forces OODGInducer to generate a categorizer
		  on the correct set of features.
  Comments    : Somewhat of an ugly hack.  We could have used ProjectInd
                  but it would make things slower and more complicated
		  for this type of research...
***************************************************************************/
void ListHOODGInducer::train()
{
   has_data();
   ao.compute_order(get_log_options(), *TS);
   const Array<int>& orderVector = ao.get_order();

   Array<int> defMap(orderVector,ctorDummy);
   defMap.sort(); // This gives the correct mapping vector!!
   set_default_map(defMap);

   BoolArray mask(0, TS->num_attr(), 0); // init to zero.
   for (int i = 0; i < orderVector.size(); i++)
      mask[orderVector.index(i)] = 1;

   InstanceBag *projTS = TS->project(mask);
   projTS->remove_conflicting_instances();

   orderPosition = orderVector.size()-1; // for best_split
   delete tmpOrder;
   tmpOrder = new Array<int>(0, orderVector.size());

   // Now create the best attributes on the projected set of features
   // For each feature in the order, we find its index in the map.
   for (i = 0; i < orderVector.size(); i++) {
      for (int j = 0; j < defMap.size() &&
                      orderVector.index(i) != defMap.index(j); j++)
          ; // NULL
      ASSERT(orderVector.index(i) == defMap.index(j)); // must be found
      tmpOrder->index(i) = j;
   }

   InstanceBag *oldTS = assign_data(projTS);
   HOODGInducer::train();
   delete assign_data(oldTS); // restore old one.
}


/***************************************************************************
  Description : Using orderPosition, retrieves the next attribute number
                  listed in the given order vector.  The bag ptr array is
		  ignored.
  Comments    :
***************************************************************************/
int ListHOODGInducer::best_split(const BagPtrArray&)
{
   ASSERT(tmpOrder);
   if (orderPosition < 0)
      err << "ListHOODGInducer::best_split: no more entries in order"
	  << fatal_error;
   int attr = tmpOrder->index(orderPosition);
   tmpOrder->index(orderPosition--) = -1;

   // Since OODG will project, we shift everything higher than
   // this number down by one.

   for (int i = 0; i <= orderPosition; i++)
      if (tmpOrder->index(i) > attr)
         tmpOrder->index(i)--;
   
   return attr;
}



/*****************************************************************************
  Description : Set user options.
  Comments    :
*****************************************************************************/
void ListHOODGInducer::set_user_options(const MString& prefix)
{
   get_attr_order_info().set_user_options(prefix);
}
