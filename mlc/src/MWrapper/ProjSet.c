// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The ProjSet class implements a set of projected instances.
                   This set represents a single node in the OODG being
		   constructed.  The node's destinations are kept track of
		   separately within each Projection.
  Assumptions  : All destinations (stored in Projection) within this
                   ProjSet must be compatible.  This restriction is
		   enforced by the project_down() function.
  Comments     : 
  Complexity   :
  Enhancements :
  History      : Dan Sommerfield                                   11/18/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <InstList.h>
#include <ProjLevel.h>
#include <ProjGraph.h>
#include <AttrCat.h>
#include <ConstCat.h>

RCSID("MLC++, $RCSfile: ProjSet.c,v $ $Revision: 1.2 $")


/***************************************************************************
  Description : Consistency checks.
  Comments    :
***************************************************************************/
void ProjSet::OK() const
{
   for(int i=0; i<projArray.size(); i++)
      projArray.index(i)->OK();

   ASSERT(idNum >= -1);
   if(nogoods) {
      ASSERT(color < maxColors);
      ASSERT(color != -1);
      ASSERT((*nogoods)[color] != TRUE);
      ASSERT(color <= colorsUsed);
      ASSERT(node);
   }
}

void ProjSet::check_consistency(const SchemaRC& schema,
				const ProjLevel *apparentSource,
				const ProjLevel *apparentDest) const
{
   for(int i=0; i<projArray.size(); i++)
      projArray.index(i)->check_consistency(schema, apparentSource,
					    apparentDest);
}


/***************************************************************************
  Description : Constructors
  Comments    :
***************************************************************************/
ProjSet::ProjSet(int displayLev)
   : projArray(0),
     color(1),
     nogoods(NULL),
     idNum(-1),
     displayLevel(displayLev)
{
}

ProjSet::ProjSet(Projection*& proj, int displayLev)
   : projArray(1),
     color(0),
     nogoods(NULL),
     idNum(-1),
     displayLevel(displayLev)

{
   // take ownership of the projection
   projArray[0] = proj;
   proj = NULL;
}


/***************************************************************************
  Description : Destructor
  Comments    :
***************************************************************************/
ProjSet::~ProjSet()
{
   // DBG(OK());
   delete nogoods;
   for(int i=0; i<projArray.size(); i++)
      delete projArray.index(i);
}

/***************************************************************************
  Description : Add a Projection to a ProjSet.  Takes ownership of the
                  projection.
  Comments    :
***************************************************************************/
void ProjSet::add(Projection*& proj)
{
   int last = projArray.size();
   projArray[last] = proj;
   proj = NULL;
}

/***************************************************************************
  Description : Append the projSet to this one.  Takes ownership of all
                  projections inside the set, and deletes the other
		  projSet.
  Comments    :
***************************************************************************/
void ProjSet::add(ProjSet*& projSet)
{
   projArray.append(projSet->projArray);
   for(int i=0; i<projSet->projArray.size(); i++)
      projSet->projArray[i] = NULL;
   delete projSet;
   projSet = NULL;
}

/***************************************************************************
  Description : Determine if two ProjSets conflict.  This is true if one
                  or more Projections in the set conflict.  This function
		  returns the final number of conflicts.
  Comments    :
***************************************************************************/
int ProjSet::conflicts_with(const ProjSet& other) const
{
   // go through each looking for conflicts
   // this is an m*n operation
   int conflicts = 0;
   for(int i=projArray.low(); i<=projArray.high(); i++)
      for(int j=other.projArray.low(); j<=other.projArray.high(); j++) {
	 if(projArray[i]->conflicts_with(*(other.projArray[j])))
	    conflicts++;
      }
   return conflicts;
}

/***************************************************************************
  Description : Assigns a color to this ProjSet (node).
  Comments    :
***************************************************************************/
void ProjSet::set_color(int col)
{
   // make sure color is not a nogood
   if((*nogoods)[col])
      err << "ProjSet::set_color: Color " << col << " is a nogood "
	 "for this node." << fatal_error;
   color = col;
}

/***************************************************************************
  Description : Declares a color to be nogood for this ProjSet (node)
  Comments    :
***************************************************************************/
void ProjSet::add_nogood(int col)
{
   if(!(*nogoods)[col]) {
      colorsUsed++;
      (*nogoods)[col] = TRUE;
   }
}

/***************************************************************************
  Description : Searches for the first available good color for this ProjSet
                  If all colors are nogood, returns -1.
  Comments    :
***************************************************************************/
int ProjSet::get_first_ok_color() const
{
   for(int i=nogoods->low(); i<=nogoods->high(); i++)
      if(!(*nogoods)[i]) return i;

   // no color available
   return -1;
}

/***************************************************************************
  Description : Resets a ProjSet for reuse in the coloring algorithm
  Comments    :
***************************************************************************/
void ProjSet::reset(int max)
{
   delete nogoods;
   maxColors = max;
   nogoods = new BoolArray(0, maxColors, FALSE);
   colorsUsed = 0;
   color = -1;
}

/***************************************************************************
  Description : Updates the destinations within the source of this ProjSet.
  Comments    :
***************************************************************************/
void ProjSet::update_source(Category newDest)
{
   // go through each Projection and update its source
   for(int i=projArray.low(); i<=projArray.high(); i++)
      projArray[i]->update_source(newDest);
}


/***************************************************************************
  Description : Project the entire set down a level, using the interpolation
                  rule that Projections which agree on the values of the
		  attributes mentioned in selector should be grouped
		  together.
  Comments    : We accomplish this function in O(m*n) time, where m
                  is the length of the selector and n is the number
		  of instances in the level.
		Here's the method:
		1. Call project_down on ALL the Projections in this set.
		   Group all the projections we get out of this into one
		   large ProjSet.
		2. split this ProjSet (and all the ones created through
		   splitting) by each attribute in the selector in turn.
		3, return the final Array of all the ProjSets we generate
		   on the last splitting round.
***************************************************************************/
Array<ProjSet *> *ProjSet::
project_down(const FeatureSet& set, ProjLevel& dest)
{
   // collect an array of all the projections we get when we project
   // downwards.
   DynamicArray<Projection *> allDownProjs(0);
   for(int i=projArray.low(); i<=projArray.high(); i++) {
      DynamicArray<Projection *> *downProj =
	 projArray[i]->project_down(set, dest);
      allDownProjs.append(*downProj);
      delete downProj;
   }
   ASSERT(allDownProjs.size() > 0);

   // place all projections into a giant ProjSet
   ProjSet *bigSet = new ProjSet(displayLevel);
   for(i=allDownProjs.low(); i<=allDownProjs.high(); i++)
      bigSet->add(allDownProjs[i]);

   // build an array to hold the ProjSets.  Start it with the one we just
   // created.
   DynamicArray<ProjSet *> *psa = new DynamicArray<ProjSet *>(1);
   psa->index(0) = bigSet;

   // for each attribute a in selector, Collect all the ProjSets attained by
   // splitting each ProjSet in the array on a.  Replace the array with
   // this new collection.  Never add NULL ProjSets to anything.
   const Array<int>& selector = set.get_feature_numbers();
   for(int a = selector.low(); a<=selector.high(); a++) {
      DynamicArray<ProjSet *> dest(0);
      for(int i=psa->low(); i<=psa->high(); i++) {
	 Array<ProjSet*> *split = ((*psa)[i])->split_by_attribute(selector[a]);
	 for(int splitElem = split->low(); splitElem <= split->high();
					  splitElem++)
	    if((*split)[splitElem])
	       dest[dest.high()+1] = (*split)[splitElem];
	 delete split;
	 delete (*psa)[i];
      }
      psa->truncate(0);
      psa->append(dest);
   }
   return psa;
}


/***************************************************************************
  Description : Split a Projection Set into several projection sets,
                  depending on the value of a specific attribute.
		  The attribute MUST be mentioned in the instSelector
		  of each Projection in the set.
		This projection set loses ownership of all projections.
  Comments    : 
***************************************************************************/
Array<ProjSet *> *ProjSet::split_by_attribute(int attrNum)
{
   // The array we pass in must have at least one element
   ASSERT(projArray.size() > 0);

   // Figure out how many attribute values are possible for the
   // attribute number and allocate the array accordingly
   const InstanceRC& inst = projArray[projArray.low()]->get_main_instance();
   AttrCategorizer attrCat(projArray[projArray.low()]->get_schema(),
			   attrNum, "dummy");
   Array<ProjSet *> *psa =
      new Array<ProjSet *>(UNKNOWN_CATEGORY_VAL, attrCat.num_categories()+1,
			   NULL);
   
   // now run through the projArray.  Categorize each Projection by
   // main instance.
   for(int i=projArray.low(); i<=projArray.high(); i++) {
      int loc = attrCat.categorize(projArray[i]->get_main_instance());
      if((*psa)[loc] == NULL)
	 (*psa)[loc] = new ProjSet(projArray[i], displayLevel);
      else
	 (*psa)[loc]->add(projArray[i]);
   }

   return psa;
}

/***************************************************************************
  Description : Connect all projections in the ProjSet to the level
                  as specified by level.  Also sets up the categorizer
		  for the node at this level.
  Comments    :
***************************************************************************/
void ProjSet::connect_to_level(CatGraph& graph, ProjLevel& level, int attr,
			       const SchemaRC& schema)
{
   AttrCategorizer attrCat(schema, attr, schema.attr_name(attr));

   // build the array.  Initialize to all -1 (no destination)
   // also set up a counts array to help find the majority destination later
   Array<int> destArray(UNKNOWN_CATEGORY_VAL, attrCat.num_categories()+1);
   Array<int> counts(UNKNOWN_CATEGORY_VAL, attrCat.num_categories()+1);
   for(int i=destArray.low(); i<=destArray.high(); i++) {
      destArray[i] = -1;
      counts[i] = 0;
   }

   // for each projection, fill into the array
   for(i=projArray.low(); i<=projArray.high(); i++)
      projArray[i]->fill_destinations(attrCat, destArray, counts);

   // find the majority destination by looking at counts
   int majorityDest = counts.low();
   for(i=counts.low()+1; i<=counts.high(); i++) {
      if(counts[i] > counts[majorityDest])
	 majorityDest = i;
   }
   
   // add an edge to the graph for each destination.  The destination
   // number serves as the index into the next level's ProjSets, which
   // give us the destination nodes to connect to
   for(i=destArray.low()+1; i<=destArray.high(); i++) {
      int index = destArray[i];
      if(index < 0)
	 // set to majority destination if unknown at this point
	 index = majorityDest;
      // hook up an edge to the destination node--we need the name of
      // the attribute here, too.
      const NominalAttrInfo& info = schema.attr_info(attr).cast_to_nominal();
      AugCategory *aCat = new AugCategory(i, info.get_value(i));
      graph.connect(get_node(), level.get_node(index), aCat);

      // assign a categorizer to the source node
      Categorizer *cat = new AttrCategorizer(schema, attr,
						schema.attr_name(attr));
      graph.set_categorizer(get_node(), cat);
   }      
}

/***************************************************************************
  Description : Connect all projections in the ProjSet to the leaf nodes
  Comments    :
***************************************************************************/
void ProjSet::connect_to_leaves(CatGraph& graph, Array<NodePtr>& leaves,
				int attr, const SchemaRC& schema)
{
   AttrCategorizer attrCat(schema, attr, schema.attr_name(attr));

   // build the array.  Initialize to all -1 (no destination)
   // also set up a counts array to help find the majority destination later
   Array<int> destArray(UNKNOWN_CATEGORY_VAL, attrCat.num_categories()+1);
   Array<int> counts(UNKNOWN_CATEGORY_VAL, attrCat.num_categories()+1);
   for(int i=destArray.low(); i<=destArray.high(); i++) {
      destArray[i] = -1;
      counts[i] = 0;
   }

   // for each projection, fill into the array
   for(i=projArray.low(); i<=projArray.high(); i++)
      projArray[i]->fill_destinations(attrCat, destArray, counts);

   // find the majority destination by looking at counts
   int majorityDest = counts.low();
   for(i=counts.low()+1; i<=counts.high(); i++) {
      if(counts[i] > counts[majorityDest])
	 majorityDest = i;
   }
   
   // add an edge to the graph for each destination.  The destination
   // number serves as the index into the next level's ProjSets, which
   // give us the destination nodes to connect to
   for(i=destArray.low()+1; i<=destArray.high(); i++) {
      int index = destArray[i];
      if(index < 0)
	 // set to majority destination if unknown at this point
	 index = majorityDest;
      // hook up an edge to the destination node--we need the name of
      // the attribute here, too.
      const NominalAttrInfo& info = schema.attr_info(attr).cast_to_nominal();
      AugCategory *aCat = new AugCategory(i, info.get_value(i));
      graph.connect(get_node(), leaves[index], aCat);

      // assign a categorizer to the source node
      Categorizer *cat = new AttrCategorizer(schema, attr,
						schema.attr_name(attr));
      graph.set_categorizer(get_node(), cat);
   }
}

/***************************************************************************
  Description : Get the set of instances from this ProjSet and place into
                  a bag.
  Comments    : Does not project.
***************************************************************************/
InstanceBag *ProjSet::get_instances() const
{
   ASSERT(projArray.size() > 0);
   InstanceList *instList = new
      InstanceList(projArray.index(0)->get_schema());

   // get main instance of each projection and add
   for(int i=projArray.low(); i<=projArray.high(); i++)
      instList->add_instance(projArray[i]->get_main_instance());

   return instList;
}


/***************************************************************************
  Description : Display a ProjSet.  Displays all information inside.
  Comments    :
***************************************************************************/
void ProjSet::display(MLCOStream& stream) const
{
   // display main instances in a list
   stream << "ProjSet ID:" << idNum << " --------------------" << endl;
   for(int i=projArray.low(); i<=projArray.high(); i++) {
      projArray[i]->display(stream);
      stream << "----------" << endl;
   }

   // display graph coloring info if needed
   if(nogoods) {
      stream << "nogood colors: " << nogoods->get_true_indexes() << endl;
      stream << "color: " << get_color() << endl;
      stream << "colors left: " << colors_left() << endl;
   }
}
DEF_DISPLAY(ProjSet);

/***************************************************************************
  Description : Display a ProjSet for the graph printout used to test the
                  graph-coloring algorithm.
  Comments    :
***************************************************************************/
void ProjSet::display_for_graph(MLCOStream& stream) const
{
   // display node color
   stream << "color: " << color << "\\n";
   
   // display node information for the LEDA graph
   for(int i=projArray.low(); i<=projArray.high(); i++) {
      projArray[i]->display_main_instance(stream);
      stream << "\\n";
   }
}
   

