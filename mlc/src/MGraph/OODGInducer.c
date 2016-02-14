// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : OODG - Oblivious, read-Once Decision Graphs.
                 This is a bottom-up method for building levelled
                   decision graphs by Ronny Kohavi (personal communication :-)
                 The algorithm starts with k bags, where k is the number
                   of values for the (nominal) label.  Each bag has a
                   corresponding node in the formed graph. A test attribute is
                   picked by best_split() and the instances are
                   projected on all other variables (so we have a bag
                   that has one fewer variable).  The projected
                   instances are then divided into groups which agree
                   on the destination node/bag given the value of the
                   variable tested.  We now have a smaller problem which
                   we solve recursively.
                 If at any stage the nodes that are formed are all
                   constant functions, where a constant function means that
                   all branches coming out of a node go to the same place),
                   the level is deleted altogether (irrelevant attribute).
                 See oodg.tex document by Ronny Kohavi for more information.
  Assumptions  :
  Comments     : The environment variable OODGREMOVEDUP (defaults to "yes")
                   if set to "yes," removes conflicting and duplicate
                   instances. 
  Complexity   : The main step (projection) takes the time of
                   ProjLIBag's constructor.
  Enhancements : Connect_by_val() should really connect to the
                   destination in our node with the most instances,
                   not to the default one.
                 When we look for the largest bag, it makes sense to
                    weigh the instances to see how many instances they
                    are worth at lower levels.  One projection may be
                    worth many instances below.
                 The weights are made long when printed in bags nodes.
                    Could be fixed to print real numbers.
  History      : Ronny Kohavi                                       9/18/93
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <OODGInducer.h>
#include <DestArray.h>
#include <ConstCat.h>
#include <AttrCat.h>

RCSID("MLC++, $RCSfile: OODGInducer.c,v $ $Revision: 1.42 $")



/***************************************************************************
  Description : Perform CtrInducer::OK.
  Comments    :
***************************************************************************/

void OODGInducer::OK(int /* level */) const
{
   Inducer::OK();
   if (decisionGraphCat)
      decisionGraphCat->OK();
}


/***************************************************************************
  Description : Constructor, destructor
  Comments    : 
***************************************************************************/
OODGInducer::OODGInducer(const MString& dscr, CGraph* aCgraph) :
   Inducer(dscr)
{
   cgraph = aCgraph; // save this until we actually construct the graph.
   decisionGraphCat = NULL;

   set_draw_const_nodes(FALSE);
   set_node_name_with_bag_num(FALSE);
   set_prune_ratio(((MString)get_env_default("OODG_PRUNE_RATIO", "0"))
		   .real_value()/100);

   defaultMap = NULL;
}
   
   
OODGInducer::~OODGInducer()
{
   DBG(OK());
   delete decisionGraphCat;
   delete defaultMap;
}

/***************************************************************************
  Description : Induce a decision graph.
  Comments    : Must be called after read_data().
                Erases any previously created DG.
***************************************************************************/
void OODGInducer::train()
{
   has_data();
   DBG(OK());

   delete decisionGraphCat; // remove any existing tree categorizer.
  
   if (get_env_default("OODGREMOVEDUP", "yes") == "yes") {
      int orgNumInstances = TS->num_instances();
      TS->remove_conflicting_instances();
      int newNumInstances = TS->num_instances();
      if (newNumInstances < orgNumInstances)
         LOG(1, "Removed duplicates/conflicting instances.  Num instances:"
           << newNumInstances << " down from " << orgNumInstances << endl);
   }


   init_train(); // Can be used to reset counters
   // The decisionGraph either creates a new graph, or gets ours.
   RootedCatGraph* dg = (cgraph == NULL) ? new RootedCatGraph
                                         : new RootedCatGraph(*cgraph);

   if (TS->num_instances() == 0)
      err << "OODGInducer::train: zero instances" << fatal_error;
   
   if (TS->get_schema().num_attr() == 0) { // Create single leaf
      Category majority = TS->majority_category();
      MString categoryString = TS->get_schema().
				       category_to_label_string(majority);
      MString descr(categoryString);
      AugCategory augMajority(majority, categoryString);
      LOG(2, "OODGInducer: no features" << endl);
      Categorizer* leafCat =  new ConstCategorizer(descr, augMajority);
      dg->set_root(dg->create_node(leafCat));
   }
   else {
      BagPtrArray* bpa = TS->split_by_label();
      NodeArray* na = create_leaf_nodes(*dg, *bpa);
   
      // attrMap maps the projected attribue numbers to the original ones.
      // if a default map was set, then use it to define the initial
      // mapping.  Otherwise, build and initialize a new one.
      Array<int> attrMap(TS->num_attr());
      if(defaultMap) {
	 if(defaultMap->size() != attrMap.size())
	    err << "OODGInducer::train: default attribute map has incorrect "
	       "size" << fatal_error;
	 for(int i=0; i<attrMap.size(); i++)
	    attrMap.index(i) = defaultMap->index(i);
      }
      else {
	 for (int i = 0; i < TS->num_attr(); i++)
	    attrMap[i] = i;
      }

      // induce_decision_graph() really does all the work.
      dg->set_root(induce_decision_graph(*dg, bpa, na, attrMap));
      ASSERT(bpa == NULL && na == NULL); // lost ownership
      end_train(); // Can be used to show statistics etc.
   }
   decisionGraphCat = new RDGCategorizer(dg, description(), 
					 TS->num_categories());
}



/***************************************************************************
  Description : Return TRUE iff the class has a valid decisionGraph
                  categorizer.
  Comments    :
***************************************************************************/

Bool OODGInducer::was_trained(Bool fatalOnFalse) const
{
   if (fatalOnFalse && decisionGraphCat == NULL)
      err << "OODGInducer::was_trained: No decision graph categorizer. "
             " Call train() to create categorizer" << fatal_error;
   return decisionGraphCat != NULL;
}

/***************************************************************************
  Description : Returns the categorizer that the inducer has generated.
  Comments    :
***************************************************************************/
const Categorizer& OODGInducer::get_categorizer() const
{
   was_trained(TRUE); // checks that categorizer exists
   ASSERT(decisionGraphCat != NULL);
   return *decisionGraphCat;
}

/***************************************************************************
  Description : Create the leafs which start the bottom-up induction.
  Comments    : Note that we create nodes even for label values that
                  do not have any instances with that label value.
                  This has disadvantages and advantages.  It may make
                  the graph look funny, but it's very important to
                  notice this fact!
***************************************************************************/

NodeArray *OODGInducer::create_leaf_nodes(RootedCatGraph& dg,
                                          const BagPtrArray& bpa) const
{
   NodeArray& na = *new NodeArray(FIRST_CATEGORY_VAL,
                         TS->label_info().cast_to_nominal().num_values());
   // Don't care about unknown labels at this stage.  Shouldn't be
   //   hard to support, but they make the graph ugly if they aren't
   //   there, and they're usually not.
   if (!bpa[UNKNOWN_CATEGORY_VAL]->no_instances())
      err << "OODGInducer::create_leaf_nodes: Unknown categories in label"
         " values are unsupported." << fatal_error;
   LOG(3, "OODGInducer::Create_leaf_nodes: bags are" << endl);
   for (int labelVal = na.low(); labelVal <= na.high(); labelVal++) {
      bpa[labelVal]->set_weighted(TRUE);
      // Make sure not to create a leaf with zero instances because
      //   the graph won't be connected (CatGraph check_reachable fails).
      if (bpa[labelVal]->num_instances() > 0) {
         MString catName;
         if (get_node_name_with_bag_num())
            catName = "Bag " + MString(labelVal,0) + "(" +
               MString(bpa[labelVal]->num_instances(),0) + "/" +
               MString((long)bpa[labelVal]->total_weight(),0) + "): ";

         const MString& labelStr = TS->get_schema().
                                  category_to_label_string(labelVal);
         catName += labelStr;
         Categorizer* constCat = new ConstCategorizer(catName,
                                    AugCategory(labelVal, labelStr));
         na[labelVal] = dg.create_node(constCat);
      }
      LOG(3, "   Bag " << labelVal << endl << *bpa[labelVal] << endl);
   }
   LOG(3, "Graph is:" << endl << dg << endl);
   return &na;
}

/***************************************************************************
  Description : Create a decision graph level from ProjInfoPtrList to
                  the given NodeArray, then redefines the NodeArray to be
                  the new NodeArray.
                It also converts the ProjInfoPtrList to a BagPtrArray,
                  eliminating the extra information there.
                defaultDestBag is used for all projected instances
                  which have UNDEFINED_CATEGORY_VAL destinations.
  Comments    : Gets ownership of the ProjInfoPtrList.
***************************************************************************/


// connect_const_dest connects a node to a constant destination,
//   independent of the attribute value.  Depending on the 
//   draw_const_node option, we either create the node and connect,
//   or just pass the node we're suppose to connect to the next level.

void OODGInducer::connect_const_dest(RootedCatGraph& dg,
	     Category bagNum, int numInstances,
	     Real totalWeight, const AttrInfo& splitAttrInfo,
	     const NodePtr destNode, NodePtr& newNode) const
{
   if (drawConstNodes) {
      // Note that we use the FIRST_CATEGORY_VAL as the edge number, because
      // the first edge added to a node must be this or UNKNOWN (can't use,
      // say, the destination node).
      AugCategory destCat(FIRST_CATEGORY_VAL, "Continue");
      MString catName;
      if (nodeNameWithBagNum)
        catName = "Bag " + MString(bagNum,0) + "(" +
           MString(numInstances,0) + "/" +
           MString((long)totalWeight,0) + "): ";

      catName += "Ignore " + splitAttrInfo.name();
      Categorizer* cat = new ConstCategorizer(catName, destCat);
      newNode = dg.create_node(cat);   
      AugCategory* aca = new AugCategory(FIRST_CATEGORY_VAL,
                       "Always (" + MString(totalWeight,0) + ")");
      dg.connect(newNode, destNode, aca);
   } else 
      newNode = destNode; 
}

// connect_by_val connects a node to destinations according to the
//    dest array.  All UNKNOWN destinations are sent to the default
//    destination as given.
void OODGInducer::connect_by_val(RootedCatGraph& dg,
	 Category bagNum, int numInstances,
	 Real totalWeight, const AttrInfo& splitAttrInfo,
	 int splitAttrNum, const Array<NominalVal>& dest,
         const Array<Real>& destCounts, const NodeArray& na, 
	 NominalVal defaultDest, NodePtr& newNode) const
{
   MString catName;
   if (nodeNameWithBagNum)
      catName = "Bag " + MString(bagNum,0) + "(" +
         MString(numInstances,0) + "/" +
         MString((long)totalWeight,0) + "): ";

   catName += splitAttrInfo.name();
   Categorizer* cat = new AttrCategorizer(splitAttrInfo, splitAttrNum,catName);
   newNode = dg.create_node(cat);

   DBG_DECLARE(Real summedWeight = 0;)
   for (Category val = dest.low(); val <= dest.high(); val++) {
      AttrValue_ aVal;
      splitAttrInfo.set_nominal_val(aVal, val);
      if (val != UNKNOWN_NOMINAL_VAL || dest[val] != UNKNOWN_NOMINAL_VAL) {
         NominalVal destCat = dest[val];
         if (destCat == UNKNOWN_NOMINAL_VAL) {
            LOG(1, "Unknown destination for value " << val
                << " setting to " << defaultDest << endl);
            destCat = defaultDest;
         }
         MString edgeLabel(splitAttrInfo.attrValue_to_string(aVal));
         DBG(summedWeight += destCounts[val]);
         if (nodeNameWithBagNum)
            edgeLabel += " (" + MString(destCounts[val],0) + ")";
         AugCategory* aca = new AugCategory(val, edgeLabel);
         dg.connect(newNode, na[destCat], aca);
      }
   }
   DBG(ASSERT(summedWeight == totalWeight));
}


BagPtrArray* OODGInducer::create_interior_nodes(RootedCatGraph& dg,
           const AttrInfo& splitAttrInfo, int splitAttrNum,
	   ProjInfoPtrList*& pipl,
           Category defaultDestBag, NodeArray*& na) const
{
   DBG((void)splitAttrInfo.cast_to_nominal()); // make sure it's nominal

   int numBags = pipl->length();
   ASSERT(numBags > 0);
   BagPtrArray *bpa = new BagPtrArray(FIRST_CATEGORY_VAL, numBags);
   
   const SchemaRC schema = pipl->front()->bag->get_schema();

   NodeArray* naNew = new NodeArray(FIRST_CATEGORY_VAL, pipl->length());
   int bagNum = FIRST_CATEGORY_VAL;
   for (ProjInfoPtrPix pix(*pipl,1); pix; ++pix, bagNum++) {
      ProjInfoWithBag& piwb = *(*pipl)(pix); 

      // Create the node and connect to the next level.  If all
      //    destinations are the same, we create a ConstCategorizer,
      //    otherwise and AttrCategorizer.
      NominalVal sameDest;
      if ((sameDest = 
           DestArray::same_dest(piwb.dest_bag())) != UNKNOWN_NOMINAL_VAL) 
         connect_const_dest(dg, bagNum, piwb.bag->num_instances(),
                            piwb.bag->total_weight(),
                            splitAttrInfo, (*na)[sameDest], (*naNew)[bagNum]);
      else
         connect_by_val(dg, bagNum, 
                        piwb.bag->num_instances(),
                        piwb.bag->total_weight(),
                        splitAttrInfo, splitAttrNum,
			piwb.dest_bag(), piwb.dest_counts(),
                        *na, defaultDestBag, (*naNew)[bagNum]);

      (*bpa)[bagNum] = piwb.bag; // transfer the bag over
      piwb.weight = 0;           // Make sure no weight is left.
      ASSERT(!piwb.bag->no_instances());
      piwb.bag = NULL;
      LOG(3, "OODGInducer::create_interior_nodes.  Displaying Bag " <<
          bagNum << endl << *(*bpa)[bagNum] << endl);
   }

   while (!pipl->empty())
      delete pipl->remove_front(); 
   delete pipl; DBG(pipl = NULL);
   delete na; na = naNew;

   LOG(3, "Graph is:" << endl << dg << endl);
   return bpa;
}
      


/***************************************************************************
  Description : Induce a decision graph in the given Cgraph.  Returns a
                  pointer to the root of the decision Graph.
  Comments    : Gets ownership of the BagPtrArray and the NodeArray.
                Calls itself recursively.
                The labelledInstanceInfo which is passed is pointed to
                  by the BagPtrArray (unless it is NULL).  After we
                  construct a new LabelledInstanceInfo for the
                  recursive call, we can free it. 
                The BagPtrArray contains the instances in each
                  destination node which is represented by the same
                  array index in the NodeArray.  (It would probably be
                  nicer to have a structure of both instead of passing
                  two arguments, but many functions like split return
                  only one).
                Note that we don't want find_cover() to call
                  create_interior_nodes() directly, because it shouldn't
                  touch the actual graph in case of lookaheads and such.
                  Similarly, we don't want find_cover to get ownership
                  of bpa because we may want to use it a few times for
                  splitting, each time looking at the result of find_cover()
		When doing projections, the attribute numbers are changing, so
    		  we must keep a map of the original attribute numbers, and
		  shift it whenever we project and delete an attribute.
		  Each entry i in the array, says what attribute i's number
		  was in the original dataset.
***************************************************************************/

// find_largest_bag() returns the number of the bag with the most instances.
// Equality is broken by picking earlier bags.
static int find_largest_bag(const BagPtrArray& bpa)
{
   int maxInstances = -1;
   int maxBag = -1;
   for (int i = bpa.low(); i <= bpa.high(); i++)
      if (bpa[i]->num_instances() > maxInstances) {
         maxBag = i;
         maxInstances = bpa[i]->num_instances();
      }
   ASSERT(maxBag != -1);

   return maxBag;
}
         

NodePtr OODGInducer::induce_decision_graph(RootedCatGraph& dg,
     BagPtrArray*& bpa, NodeArray*& na, Array<int>& attrMap) 
{
   has_data(TRUE);
   if (bpa->size() == 0)
      err << "OODGInducer::induce_decision_graph: No bags in BagPtrArray"
          << fatal_error;
   
   ASSERT(attrMap.size() >= (*bpa)[0]->num_attr());
   // If there is only one bag, we are done.
   if (bpa->size() == 1) {
      ASSERT(na->size() == 1);
      delete bpa; 
      NodePtr root = na->index(0);
      delete na; na = NULL;
      return root;
   }

   Category largestBagCat = find_largest_bag(*bpa);
   int attrNum = best_split(*bpa);

   ProjInfoPtrList *pipl; // Passed to create_interior_nodes()
   pipl = find_cover(attrNum, *bpa);
   int numNodes = pipl->length();
   prune_nodes(*pipl);
   LOG(1, "OODGInducer::Splitting on attribute " << attrNum << " ("
       << bpa->index(0)->get_schema().attr_name(attrNum)
       << "). There are " << pipl->length() << " nodes (were " 
       << numNodes << ')' << endl);
   const AttrInfo& splitAttrInfo = bpa->index(0)->get_schema().
                                              attr_info(attrNum);
   BagPtrArray* bpaNew = create_interior_nodes(dg, splitAttrInfo, 
					       attrMap[attrNum],
                                               pipl, largestBagCat, na); 

   // update attribute map
   for (int i = attrNum; i < (*bpa)[0]->num_attr() - 1; i++)
      attrMap[i] = attrMap[i+1];
   DBG(attrMap[i] = -1); // Make sure it's bad if accessed.

   DBG(ASSERT(pipl == NULL));
   delete bpa; bpa = NULL;

   return induce_decision_graph(dg, bpaNew, na, attrMap);// recursive call
}
           

/***************************************************************************
  Description : Returns the number of nodes (categorizers) in the
                  decision graph, and number of leaves.
  Comments    : The number of leaves is the number of categories if
                  they all appear in the data.
                The number of nodes depends on draw_const_nodes().
                  If there are constant nodes is usually larger.
***************************************************************************/
int OODGInducer::num_nodes() const
{
   was_trained(TRUE);
   return decisionGraphCat->num_nodes();
}

int OODGInducer::num_leaves() const
{
   was_trained(TRUE);
   return decisionGraphCat->num_leaves();
}



/***************************************************************************
  Description : get/set for options.
  Comments    :
***************************************************************************/

void OODGInducer::set_draw_const_nodes(Bool val)
{
   if (val != TRUE && val != FALSE)
      err << "OODGInducer::set_draw_const_nodes: Non Boolean value "
          << val << fatal_error;

   drawConstNodes = val;
}

Bool OODGInducer::get_draw_const_nodes() const
{
   return drawConstNodes;
}

void OODGInducer::set_node_name_with_bag_num(Bool val)
{
   if (val != TRUE && val != FALSE)
      err << "OODGInducer::set_node_name_with_bag_num: Non Boolean value "
          << val << fatal_error;
   
   nodeNameWithBagNum = val;
}

Bool OODGInducer::get_node_name_with_bag_num() const
{
   return nodeNameWithBagNum;
}

void OODGInducer::set_prune_ratio(Real ratio)
{
   if (ratio < 0 || ratio >= 1)
      err << "OODGInducer::set_prune_ratio: " << ratio << " is not"
         " in the range 0-1" << fatal_error;
   pruneRatio = ratio;
}

Real OODGInducer::get_prune_ratio() const
{
   return pruneRatio;
}


void OODGInducer::set_default_map(const Array<int>& map)
{
   delete defaultMap;
   defaultMap = new Array<int>(map, ctorDummy);
}

const Array<int>& OODGInducer::get_default_map()
{
   if (defaultMap == NULL)
      err << "OODGInducer::get_default_map: no map" << fatal_error;

   return *defaultMap;
}
