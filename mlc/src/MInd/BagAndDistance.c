#include <basics.h>
#include <BagAndDistance.h>


/*****************************************************************************
  Description : Constructor. Creates a bag of the instance and sets the
                  distance.
  Comments    :
*****************************************************************************/
BagAndDistance::BagAndDistance(const InstanceRC& instance, Real dist)
{
   bag = new InstanceBag(instance.get_schema());
   distance = dist;
   insert(instance,dist);
}


/*****************************************************************************
  Description : Distance interfaces.
  Comments    :
*****************************************************************************/
Real BagAndDistance::get_distance() const
{
   return distance;
}

void BagAndDistance::set_distance(Real dist)
{
   distance = dist;
}



/*****************************************************************************
  Description : Inserts the given instance to the bag or creates a bag with
                  the instance.
  Comments    :
*****************************************************************************/
void BagAndDistance::insert(const InstanceRC& instance, Real dist)
{
   if (bag->num_instances() == 0) 
      bag->add_instance(instance);
   else {
      if (distance != dist)
	 err << "BagAndDistance::insert: different distant instance is"
	    " being inserted into BagAndDistance. The inserted"
	    " distance : " << dist << " Bag distance : " << distance
	     << fatal_error;
      bag->add_instance(instance);
   }
}


/*****************************************************************************
  Description : Returns the bag.
  Comments    :
*****************************************************************************/
const InstanceBag& BagAndDistance::get_bag() const
{
   return *bag;
}


/*****************************************************************************
  Description : Comparison methods. 
  Comments    :
*****************************************************************************/
int BagAndDistance::operator<(const BagAndDistance& element) const
{
   return (distance < element.distance);
}

int BagAndDistance::operator>(const BagAndDistance& element) const
{
   return (distance > element.distance);
}

int BagAndDistance::operator==(const BagAndDistance& element) const
{
   return (distance == element.distance);
}



/*****************************************************************************
  Description : Assignment operator.
  Comments    :
*****************************************************************************/
BagAndDistance& BagAndDistance::operator=(const BagAndDistance& elem)
{
   if (this != &elem) {
      delete bag;
      bag = new InstanceBag(*elem.bag, ctorDummy);
      distance = elem.distance;
   }
   return *this;
}


/*****************************************************************************
  Description : Displays bag and distance.
  Comments    :
*****************************************************************************/
void BagAndDistance::display(MLCOStream& stream, Bool normalizeReal) const
{
   stream << "Bag contents : " << endl;
   bag->display(stream, FALSE, normalizeReal);
   stream << endl << "Distance : " << distance << endl;
}
	 
DEF_DISPLAY(BagAndDistance)
