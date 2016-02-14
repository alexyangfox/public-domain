// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Provide operations for working on projections of bags.
                 The main class, ProjBag provides a bag (actually a set)
                   of instances that do not contain a given attribute
                   contained in an array of original bags used to
                   construct it.
                   The main member is an array of InstanceProjection,
                   each element containing one instance (projected)
                   and ProjectionInfo (described below).
                   (InstanceProjection is derived from ProjectionInfo.)
                 ProjectionInfo is a base class that holds an array
                   of destinations, plus some counters.
                 ProjInfoWithBag is derived from ProjectionInfo, and
                   adds a InstanceBag, which holds the
                   instances that agree on the Projectioninfo.
  Assumptions  : We assume the deleted attribute is nominal.
  Comments     : 
  Complexity   : The constructor take O(n^2) where n is the number of
                    instances in the bag.
  Enhancements : ProjectionInfo.display() ignores the line width.
                    Could be improved.
                 Rewrite ProjBag class to have its own pixes.
                 Implement using hash table to get the complexity of
                    the constructor down.
		 Don't give access to ProjList.  Force accessor functions.
  History      : Ronny Kohavi                                       9/21/93
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <BagSet.h>
#include <ProjBag.h>
#include <DestArray.h>

extern const MString instanceWrapIndent;

RCSID("MLC++, $RCSfile: ProjBag.c,v $ $Revision: 1.41 $")

SET_DLLPIX_CLEAR(InstanceProjection*,NULL);
SET_DLLPIX_CLEAR(ProjInfoWithBag*,NULL);

/***************************************************************************
  Description : Make sure that numDests matches the count from destBag.
  Comments    :
***************************************************************************/

void ProjectionInfo::OK(int /* level */) const
{
   ASSERT(DestArray::num_dests(destBag) == numDests);
   ASSERT(destBag.size() == destCounts.size());
}

/***************************************************************************
  Description : Constructor for ProjectionInfo just zeros the counter
                  and creates an initialized array.
  Comments    :
**************************************************************************/
ProjectionInfo::ProjectionInfo(int numValues)
  : destBag(UNKNOWN_CATEGORY_VAL, numValues + 1, UNKNOWN_CATEGORY_VAL),
    destCounts(UNKNOWN_CATEGORY_VAL, numValues + 1, 0.0),
    weight(0.0), numDests(0)
{}

ProjectionInfo::ProjectionInfo(const ProjectionInfo& pi,
			       CtorDummy dummyArg)
   : destBag(pi.destBag, dummyArg), destCounts(pi.destCounts, dummyArg),
     numDests(pi.numDests), weight(pi.weight)
{}

ProjectionInfo::~ProjectionInfo()
{
   DBG(OK());
}

/***************************************************************************
  Description : Set a destination to a given value.
  Comments    : Update weight.
                Abort if this destination is already set.
***************************************************************************/

void ProjectionInfo::set_dest(NominalVal dest, Category val, Real weight)
{
   if (destBag[dest] != UNKNOWN_CATEGORY_VAL)
      err << "ProjectionInfo::set_dest: destination " << dest
          << "already set to " << val << ".  Can't set to " << val
          << fatal_error;
   destBag[dest] = val;
   destCounts[dest] += weight;
   numDests++;
}

/***************************************************************************
  Description : Merge the current destinations with the given ones
  Comments    :
***************************************************************************/

void ProjectionInfo::merge_dests(const Array<NominalVal>& dests,
                                 const Array<Real>& destCounts2)
{
   numDests += DestArray::merge_dests(destBag, destCounts, dests, destCounts2);
}



/***************************************************************************
  Description : Display ProjectionInfo given the AttributeInfo.
  Comments    : Ignores lineWidth at this stage.
***************************************************************************/

void ProjectionInfo::display(const NominalAttrInfo& attrInfo, 
                             MLCOStream& stream) const
{
   stream << "Known destination for " << weight << " instances:";
   for (Category i = UNKNOWN_CATEGORY_VAL;
        i < FIRST_CATEGORY_VAL + attrInfo.num_values(); i++)
      if (dest_bag()[i] != UNKNOWN_CATEGORY_VAL)
	 stream << attrInfo.get_value(i) << "->" << destBag[i]
		<< '(' << destCounts[i] << ") ";
   stream << endl;
}
   

/***************************************************************************
  Description : Constructors for InstanceProjection.
  Comments    :
***************************************************************************/
InstanceProjection::InstanceProjection(int numValues, const InstanceRC& inst)
  : ProjectionInfo(numValues), instance(inst)
{}

void InstanceProjection::display(MLCOStream& stream) const
{
   // Make sure to display the unlabelled instance. 
   // There's no meaning to the label 
   instance.display_unlabelled(stream);
   stream << endl;
}   

DEF_DISPLAY(InstanceProjection)


void InstanceProjection::display(const NominalAttrInfo& attrInfo, 
				 MLCOStream& stream) const
{
   display(stream);
   stream << instanceWrapIndent;
   ProjectionInfo::display(attrInfo, stream);
}
   

   

/***************************************************************************
  Description : Constructors, destructor for ProjInfoWithBag.
  Comments    : The constructor that creates ProjInfoWithBag from a ProjInfo
                  does NOT have any instances in the bag, hence the
                  weight is set there to 0.
***************************************************************************/

ProjInfoWithBag::ProjInfoWithBag(int numValues, const SchemaRC& schema)
   : ProjectionInfo(numValues), bag(new InstanceBag(schema))
{}

ProjInfoWithBag::ProjInfoWithBag(InstanceProjection& ip)
   : ProjectionInfo(ip, ctorDummy),  // ip derived from ProjectionInfo
     bag(new InstanceBag(ip.instance.get_schema()))
{
   // Note that we do not use add_projection because it updates the
   //    count which is already set by ProjectionInfo constructor.
   bag->set_weighted(TRUE);
   ip.instance.set_weight(ip.weight);
   bag->add_instance(ip.instance);
}

ProjInfoWithBag::~ProjInfoWithBag()
{
   DBG(OK());
   delete bag;
}

/***************************************************************************
  Description : Check that the weight matches the bag weight.
  Comments    :
***************************************************************************/

void ProjInfoWithBag::OK(int /* level */) const
{
   if (bag == NULL) { // be careful of bag->total_weight
      if (weight != 0)
	 err << "ProjInfoWighBag::OK(): no bag, but weight != 0" 
	     << fatal_error;
   } else   if (weight != bag->total_weight())
      err << "ProjInfoWithBag::OK(): weight: " << weight
	  << " != total weight: " << bag->total_weight();
}


/***************************************************************************
  Description : Add a  instance projection to ProjInfoWithBag.
  Comments    : Update weights.
***************************************************************************/

Pix ProjInfoWithBag::add_projection(InstanceProjection& ip)
{
   weight += ip.weight;
   ip.instance.set_weight(ip.weight);
   return bag->add_instance(ip.instance);
}   
   


/***************************************************************************
  Description : Find the given projected instance in our ProjBag.
                If not found, create bag, and initialize with our instance.
  Comments    : Private member.
***************************************************************************/

InstanceProjection&
ProjBag::find_or_create_proj(const InstanceRC& projInst)
{
   InstanceProjection* ip;

   // find our matching instance.
   const ProjListPix pix(find_no_label(projInst));
   if (pix) // Found it
      return *projList(pix);

   // Not found - create, add, and init.
   ip = new InstanceProjection(deletedAttrInfo.num_values(), projInst);
   projList.append(ip); // Add to linked list.
   return *ip;
}

/***************************************************************************
  Description : Project the given  instance bag onto our ProjBag,
                  giving all instances the given destination.
  Comments    : Private member
***************************************************************************/

void ProjBag::project_bag(const InstanceBag& bag, Category destination)
{
   ASSERT(destination != UNKNOWN_CATEGORY_VAL);
   for (Pix bagPix = bag.first(); bagPix; bag.next(bagPix)) {
      const InstanceRC instFromBag = bag.get_instance(bagPix);
      NominalVal val
	 = deletedAttrInfo.get_nominal_val(instFromBag[deletedAttrNum]);
      InstanceRC projInst(instFromBag.remove_attr(deletedAttrNum, schema));
      InstanceProjection& ip = find_or_create_proj(projInst);
      // merge new inst?
      if ((ip.dest_bag())[val] == UNKNOWN_CATEGORY_VAL) {
         ip.set_dest(val, destination, instFromBag.get_weight());
         ip.weight += instFromBag.get_weight(); 
      }
      else if ((ip.dest_bag())[val] == destination) // duplicate?
         ip.weight += instFromBag.get_weight();
      else {
	 Mcerr << "ProjBag::project_bag: Existing projection->" 
               << (ip.dest_bag())[val]
	       << ": " << ip.instance << "New Instance->" << destination
	       << ": " << instFromBag << "Projection is on attribute "
	       << deletedAttrInfo.name() << endl;
         err << "ProjBag::project_bag: Conflicting instances under "
            " projection" << fatal_error;
      }
   }
}



/***************************************************************************
  Description : Verify that all instances are accounted for, by
                   summing the counters and seeing that they agree
                   with the stored number.  Also verify that numDests
                   agrees with the actual number of destinations that
                   are UNKNOWN.
  Comments    :
***************************************************************************/

void ProjBag::OK(int /* level */) const
{
   DBG_DECLARE(Real sumWeight = 0;)
   for (ProjListPix pix(projList,1); pix; ++pix) {
      const InstanceProjection& ip = *projList(pix);
      DBG(sumWeight += ip.weight);
      ip.OK();
   }
   DBG(ASSERT(sumWeight == weightInstances));
}


/***************************************************************************
  Description : The constructor gets an array of bags and uses the
                  bag number as the destination.
  Comments    : 
***************************************************************************/

ProjBag::ProjBag(const BagPtrArray& bags, int attrNum) :
   deletedAttrInfo(bags.index(0)->attr_info(attrNum).cast_to_nominal(),
		   ctorDummy),
   deletedAttrNum(attrNum),
   schema(bags.index(0)->get_schema().remove_attr(attrNum))
{
   DBG(weightInstances = 0);
   for (Category cat = bags.low(); cat <= bags.high(); cat++) {
      // Check that the attribute infos for the deleted attribute match.
      // The InstanceInfo itself will be compared during the find.
      DBG(deletedAttrInfo.equal(bags[cat]->attr_info(attrNum), TRUE));
      if (!bags[cat]->no_instances()) {
         project_bag(*bags[cat], cat);
         DBG(weightInstances += bags[cat]->total_weight());
      }
   }
   DBGSLOW(OK());
}

ProjBag::~ProjBag()
{
   DBG(OK());
   while (!projList.empty()) {
      InstanceProjection* ip = projList.remove_front();
      delete ip;
   }
}


/***************************************************************************
  Description : Delete an instance from the projected 
                  instance list given the pix.
  Comments    :
***************************************************************************/

void ProjBag::del(ProjListPix& pix, int dir)
{
   DBG(weightInstances -= projList(pix)->weight);
   delete projList(pix);
   projList.del(pix, dir);  // dir is the direction (see GNU)
}


/***************************************************************************
  Description : Returns Pix for instance if it was found, NULL otherwise.
  Comments    : Use get_instance(pix) to get instance.
***************************************************************************/
ProjListPix ProjBag::find_no_label(const InstanceRC& instance) const
{
   for (ProjListPix pix(projList, 1); pix; ++pix)
      if (instance.equal_no_label(projList(pix)->instance))
         return pix;

   return pix; // it's clear now.
}


/***************************************************************************
  Description : Dump all projected instances and their matching array values.
  Comments    :
***************************************************************************/

void ProjBag::display(MLCOStream& stream) const
{
   for (ProjListPix pix(projList, 1); pix; ++pix) {
      const InstanceProjection& ip = *projList(pix);
      ip.display(deletedAttrInfo, stream);
   }
}

DEF_DISPLAY(ProjBag)
