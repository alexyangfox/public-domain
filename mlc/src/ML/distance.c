// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Functions for calculating the distance between
                    two Instances and between an Instance and a
		    InstanceBag.  See function header for details.
  Assumptions  : 
  Comments     :
  Complexity   :
  Enhancements : Make the functions members of the correct class.
  History      : Richard Long                                       1/21/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <distance.h>
#include <BagSet.h>

RCSID("MLC++, $RCSfile: distance.c,v $ $Revision: 1.8 $")


/***************************************************************************
  Description : Returns the "distance" between two instances.  This is
                   simply the sum of the distances between each of the
		   corresponding attrValues in the instances.
  Comments    : The instances must have matching schemata.
***************************************************************************/
Real inst_dist(const InstanceRC inst1, const InstanceRC inst2)
{
   Real distance = 0;
   SchemaRC schema = inst1.get_schema();
   DBGSLOW(schema.equal(inst2.get_schema(), TRUE));
   for (int attrNum = 0; attrNum < schema.num_attr(); attrNum++) {
      const AttrInfo& ai = schema.attr_info(attrNum);
      distance += ai.distance(inst1[attrNum], inst2[attrNum]);
   }
   return distance;
}


/***************************************************************************
  Description : Returns the the value of the minimum distance between
                   the given instance and all of the instances in the
		   given bag.  numMin indicates the number of
		   instances in the bag that were the minimum distance
		   from the given instance.
  Comments    : The Schema for the bag must match the Schema
                   for the Instance.
***************************************************************************/
Real bag_dist(const InstanceBag& bag, const InstanceRC inst, int& numMin)
{
   numMin = 0;
   Real minDist = MAXDOUBLE;
   for (Pix pix = bag.first(); pix; bag.next(pix)) {
      Real dist = inst_dist(inst, bag.get_instance(pix));
      if (dist == minDist)
	 numMin++;
      else if (dist < minDist) {
	 minDist = dist;
	 numMin = 1;
      }
   }
   return minDist;
}

/***************************************************************************
  Description : Returns average distance between the given instance
                  and all of the instances in the given bag.  
  Comments    : The Schema for the bag must match the Schema
                   for the Instance.
***************************************************************************/
Real avg_bag_dist(const InstanceBag& bag, const InstanceRC inst)
{
   Real sum = 0;
   for (Pix pix = bag.first(); pix; bag.next(pix))
      sum += inst_dist(inst, bag.get_instance(pix));

   ASSERT(bag.num_instances()); // beware of division by zero.
   return (sum / bag.num_instances());
}
