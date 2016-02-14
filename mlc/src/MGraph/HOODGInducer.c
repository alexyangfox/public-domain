// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : A hill-climbing implementation of OODG.
                 We do a one-level lookahead to find the best split,
                   and find a cover by simple hill-climbing, starting
                   from the projected instances that have the most
                   defined destination, and greedily joining instances.
                 Instances are covered by least commitment.  First
                   those that are not consistent with any previous
                   ones are created.  Then instances that agree with
                   others are added to them.  Finally, instances that
                   are consistent are merged.
  Assumptions  :
  Comments     : 
  Complexity   : not computed
  Enhancements : Improve complexity for find_cover by using smarter
                   data structures.
                 Use graph coloring to find a good coloring of all nodes.
                   The color of a node determines its destination.
  History      : Ronny Kohavi                                       9/27/93
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <HOODGInducer.h>
#include <HOODGCIH.h>
#include <ProjStats.h>

RCSID("MLC++, $RCSfile: HOODGInducer.c,v $ $Revision: 1.23 $")
   
static CIHMinDests defaultHeuristic;
  

/***************************************************************************
  Description : Cover an instance only if it is a forced new bag.
                A forced new bag contradicts with all others.
  Comments    : Protected.
***************************************************************************/
Bool HOODGInducer::is_forced_new_bag(ProjInfoPtrList& pipl,
                             InstanceProjection& ip,
                             const NominalAttrInfo& deletedAttrInfo) const
{
   // Check if the instance is forced.
   for (ProjInfoPtrPix pix(pipl, 1); pix; ++pix) {
      ProjInfoWithBag& piwb = *pipl(pix);
      if (DestArray::consistent_dests(piwb.dest_bag(), ip.dest_bag()))
         return FALSE;
   }

   // Now that we know it is forced, create a new bag for it.
   ProjInfoWithBag* piwb = new ProjInfoWithBag(ip);
   
   LOG(3, "New (forced) projection.  Bag number is " << FIRST_NOMINAL_VAL +
          pipl.length() << "\n   ");
   IFLOG(3, ip.display(deletedAttrInfo, get_log_stream()));

   pipl.append(piwb);
   return TRUE;
}


/***************************************************************************
  Description : Cover the instance by merging it with a projected
                  instance it is consistent with.
                The exact heuristic is determined by
                   HOODGCoverInstanceHeuristic which is a class that
                   must have the members init() and is_better().
                   The function init() will be called once, and
                   is_better() will be called for every new candidate bag.
  Comments    : Protected
                We assume that the instance does not contradict all
                   instances in the list (see is_forced_new_bag).

***************************************************************************/


void HOODGInducer::cover_instance(ProjInfoPtrList& pipl,
                                  InstanceProjection& ip,
                                  const NominalAttrInfo& deletedAttrInfo) const
{
   ProjInfoPtrPix bestPix(pipl);
   int bestBagNum = -1;

   int bagNum = FIRST_CATEGORY_VAL;
   get_cover_instance_heuristic().init_instance_cover(&ip);
   for (ProjInfoPtrPix pix(pipl,1); pix; ++pix, bagNum++) {
      ProjInfoWithBag& piwb = *pipl(pix);
      if (get_cover_instance_heuristic().is_better(piwb)) {
         bestBagNum = bagNum;
         bestPix = pix;
      }
   }

   // CIH should return at least one bestPix.  Note however that the
   //   first piwb (first iteration) may not always cause CIH.is_better()
   //   to return TRUE because any piwb must first of all be consistent.
   if (!bestPix)
      err << "HOODGInducer::cover_instance: CoverInstanceHeuristic "
             "failed to return a value" << fatal_error;

        
   ProjInfoWithBag& piwb = *pipl(bestPix);
   IFLOG(3, 
      if (DestArray::included_in_dest(ip.dest_bag(), piwb.dest_bag()))
         get_log_stream() << "Projection included in bag ";
      else
         get_log_stream() << "Projection merged with bag ";

      get_log_stream() << bestBagNum << " result is \n   ";
   );

   piwb.merge_dests(ip.dest_bag(), ip.dest_counts());
   piwb.add_projection(ip);
   // Now display the merged projection info.
   IFLOG(3, piwb.display(deletedAttrInfo, get_log_stream()));

//   delete &bpaNew; // it erases the bags.
}

/***************************************************************************
  Description : Cover all contradicting instances that have the given
                  number of destinations (createCount).
                If we delete an instance which is equal to the current
                  pix, we advance the given posPix (some position in
                  the file).
                Return TRUE if we covered something.
  Comments    : These must be created as new nodes because they contradict.
                They may affect future merges/includes by the mere fact 
                  that they exist, and something can be merged with them.
***************************************************************************/

Bool HOODGInducer::cover_contradicting_instances(ProjBag& projBag, 
   int createCount, ProjInfoPtrList& pipl, ProjListPix& posPix) const
{
   int count = 0;
   
   ProjListPix pix(projBag.projList, 1);
   while (pix) {
      InstanceProjection& ip = *projBag.projList(pix);
      ASSERT(ip.num_dests() <= createCount && ip.num_dests() > 0);
      if (ip.num_dests() == createCount &&
          is_forced_new_bag(pipl, ip, projBag.deleted_attr_info())){
         if (posPix && pix == posPix)
            ++posPix;
         projBag.del(pix, 1);    // advance pix by 1
         ++count;
      }
      else
        ++ pix;
   }

   LOG(4, "HOODGInducer::cover_contradicting_instances: covered " <<
       count << endl);
   return (count != 0);
}   



/***************************************************************************
  Description : Constructor just passes the information on.
                The destructor frees the CoverInstanceHeuristic if
                   it's not our static default one.
  Comments    :
***************************************************************************/

HOODGInducer::HOODGInducer(const MString& dscr, CGraph* aCgraph)
   : OODGInducer(dscr, aCgraph)
{
   set_cover_instance_heuristic(defaultHeuristic);
   
}

HOODGInducer::~HOODGInducer()
{
   if (&get_cover_instance_heuristic() != &defaultHeuristic)
      delete coverInstanceHeuristic;
}


/***************************************************************************
  Description : init train, end train.
  Comments    : This call will be executed before training begins.
                It is meant to allow the heuristic to zero its
                  statistic counts and other data structures used.
***************************************************************************/

void HOODGInducer::init_train() {
   get_cover_instance_heuristic().set_log_level(get_log_level());   
   get_cover_instance_heuristic().init();
}

void HOODGInducer::end_train()  {
   IFLOG(1, get_cover_instance_heuristic().total_stats(get_log_stream()));
}

/***************************************************************************
  Description : Set/get CoverInstanceHeuristic
                The CoverInstanceHeuristic class allows replacing
                  the heuristic for covering an instance. 
                This method of choosing the heuristic was preferred over
                  inheritance because this allows subclassing the default
                  heuristic class instead of all of HOODG,
                  and because the heuristics need to keep track of private
                  data which HOODG doesn't really care about.
                  If we just gave a virtual method, we would have to
                  have the data members in HOODG.
  Comments    : Gets ownership of the CoverInstanceHeuristic.
***************************************************************************/

void HOODGInducer::set_cover_instance_heuristic(CoverInstanceHeuristic &cih)
{
   coverInstanceHeuristic = &cih;
}

CoverInstanceHeuristic& HOODGInducer::get_cover_instance_heuristic() const
{
   ASSERT(coverInstanceHeuristic);
   
   return *coverInstanceHeuristic;
}



/***************************************************************************
  Description : Find a cover for the given projection bag.
                We make a first pass through the file, creating all
                  instances that have all destinations
                  (probably zero since we don't support UNKNOWNS very
                  well at this stage).  During the pass, we keep track
                  of the highest number of destinations.  In the next
                  pass, we create those instances.
                We then cover instances according to the
                  CoverInstanceHeuristic class.
  Comments    : We assume all bags have the same Schema.
                This is actually checked by the ProjBag when doing
                  the projection.
***************************************************************************/

ProjInfoPtrList* HOODGInducer::find_cover(int attrNum,
   const BagPtrArray& bpa) const
{
   ASSERT(bpa.size() > 0);
   const NominalAttrInfo &attrInfo = bpa.index(0)->attr_info(attrNum).
                                                        cast_to_nominal();

   ProjBag projBag(bpa, attrNum);
   LOG(4, "Displaying projection bags for cover\n" << projBag);
   ProjInfoPtrList& pipl = *new ProjInfoPtrList;
 
   int createCount = attrInfo.num_values() + 1; // For unknown.
   get_cover_instance_heuristic().init_level_cover();
   while (!projBag.projList.empty()) {
      int highCount   = 0; // highest count less than createCount
      if (createCount <= attrInfo.num_values())
         LOG(3, "  Covering instances with " << createCount <<" dests"<< endl);

      // dummy to pass to cover_contradicting_instances().
      ProjListPix pix(projBag.projList); 
      cover_contradicting_instances(projBag,createCount, pipl, pix);
      ASSERT(!pix); // should not be changed by the above call.

      // Cover the rest of the instances.
      pix.first();
      while (pix) {
         InstanceProjection& ip = *projBag.projList(pix);
         ASSERT(ip.num_dests() <= createCount && ip.num_dests() > 0);
         if (ip.num_dests() == createCount) {
            IFLOG(3, ip.display(attrInfo, get_log_stream()));
            cover_instance(pipl, ip, projBag.deleted_attr_info());
            projBag.del(pix, 1); // advance pix by 1
#           ifndef NOLEASTCOMMITMENT
            if (cover_contradicting_instances(projBag, createCount, pipl,pix)){
               LOG(1, "Least commitment cover was useful" << endl);
            }
#           endif
         } else {
            if (ip.num_dests() > highCount)
               highCount = ip.num_dests();
         
            ++pix;
         }
      }
      createCount = highCount;
   }
   IFLOG(2, get_cover_instance_heuristic().level_stats(get_log_stream()));
   return &pipl;
}
      

/***************************************************************************
  Description : Find the number of irrelevant attributes at next level.
                The current level is defined by the ProjectionInfo List.
  Comments    : We add something to LOG(2) line.
***************************************************************************/

int HOODGInducer::num_irrelevant(const ProjInfoPtrList& pipl)
{
   int saveLogLevel = get_log_level();
   int newLogLevel = saveLogLevel - 2;
   if (newLogLevel < 0) newLogLevel = 0;
   set_log_level(newLogLevel);

   int numBags = pipl.length(); // (may be zero length)

   // Create a new bag array
   BagPtrArray& bpaNew = *new BagPtrArray(FIRST_CATEGORY_VAL,numBags);
   int bagNum = FIRST_CATEGORY_VAL;
   for (ProjInfoPtrPix bagPix(pipl, 1); bagPix; ++bagPix, ++bagNum){
      bpaNew[bagNum] = pipl(bagPix)->bag; // point to bag
      bpaNew[bagNum]->set_weighted(TRUE);
      ASSERT(!bpaNew[bagNum]->no_instances());
   }

   const SchemaRC schema = bpaNew.index(0)->get_schema();
   int nextLevelIrrelevant = 0;

   for (int attrNum = 0; attrNum < schema.num_attr(); attrNum++) {
      ProjInfoPtrList *piplNew;
      piplNew = find_cover(attrNum, bpaNew);
      ProjStats stats(*piplNew, get_log_options());
      if (stats.num_const_bags() == numBags)
         nextLevelIrrelevant++;

      while (!piplNew->empty())
	 delete piplNew->remove_front(); 
      delete piplNew; DBG(piplNew = NULL);
   } 
   set_log_level(saveLogLevel);
   LOG(2, nextLevelIrrelevant << " irrelevant at next level" << endl);

   return nextLevelIrrelevant;
}


/***************************************************************************
  Description : Find best split.
                We look at all attributes, and pick the attribute that
                   gives the smallest split (hill-climbing).
                Ties are broken by prefering higher-numbered
                  attributes.  The only motivation for this is that
                  since we're building bottom up, symmetric concepts
                  like parity split on attribute 0,1,2,3 from the top down.
                Irrelevant attributes are preferred, picking those
                  with the hightest number of irrelevancies at the next level.
  Comments    : We could do something like HOODGCIH for the exact heuristic
                  used.
***************************************************************************/
// Cast constness away because of GNU.
#define PIPLPTR ((ProjInfoPtrList*)pipl)


int HOODGInducer::best_split(const BagPtrArray& bpa) 
{
   // tmpHeuristic is used when running best_split.  We do not really
   //   want the real heuristic to be updated when testing splits.  
   static CIHMinDests tmpHeuristic;
   static Bool irrelevantLookahead = 
      (get_env_default("HOODGLOOKAHEAD", "no") == "yes");

   CoverInstanceHeuristic& saveCIH = get_cover_instance_heuristic();
   tmpHeuristic.set_log_level(saveCIH.get_log_level());
   set_cover_instance_heuristic(tmpHeuristic);


   int bestAttr = -1;
   int bestSize = INT_MAX;
   int bestNextLevelIrrelevant = -1;
   int bestConsistentChoices = INT_MAX;
   int bestInstanceCovers    = INT_MAX;
   int bestNumDiffDests      = INT_MAX;
   
   ASSERT(bpa.size() > 0);
   const SchemaRC schema = bpa.index(0)->get_schema();
   ASSERT(schema.num_attr() > 0); // or why are you calling bset_split?
   
   for (int attrNum = 0; attrNum < schema.num_attr(); attrNum++) {
      tmpHeuristic.init();
      LOG(2, "\nHOOGInducer::best_split: Testing split on attribute "
          << attrNum << " (" << schema.attr_name(attrNum) << ')' << endl);

      ProjInfoPtrList *pipl; // Passed to create_interior_nodes()

      // print less in find_cover
      int saveLogLevel = get_log_level();
      int newLogLevel = saveLogLevel - 2;
      if (newLogLevel < 0) newLogLevel = 0;
      set_log_level(newLogLevel);
      // Make sure the heuristic also has this logLevel;
      tmpHeuristic.set_log_level(newLogLevel);
      pipl = find_cover(attrNum, bpa);
//      prune_nodes(*pipl);
      set_log_level(saveLogLevel);   
      int numBags = PIPLPTR->length();

      int numConsistentChoices = tmpHeuristic.total_consistent_choices();
      int numInstanceCovers    = tmpHeuristic.total_instance_covers();

      IFLOG(2, tmpHeuristic.total_stats(get_log_stream()));
      ProjStats stats(*pipl, get_log_options());
      int currNumDiffDests = stats.num_diff_dests();


      // Note that it may be the case that numUnknowns != 0.  Thus
      //   that attribute may seem irrelevant because of "unknown"
      //   things.  A stronger irrelevancy requires that
      //   numUnknowns == 0
      if (irrelevantLookahead && stats.num_const_bags() == numBags) {//irrlvnt
         LOG(2, "Attribute " << attrNum << " (" << schema.attr_name(attrNum)
                << ") irrelevant." << endl);
         // find_num_irrelevant will add info to above log.
         int nextLevelIrrelevant = num_irrelevant(*pipl);

         // prefer later attributes (looks nicer)
         int result = bestNextLevelIrrelevant - nextLevelIrrelevant;
         if (result == 0) { // tie
            result =  bestInstanceCovers - numInstanceCovers;
            if (result == 0) { // tie
               result = bestConsistentChoices - numConsistentChoices;
            }
         }
         
         if (result >= 0) { // winner
            bestNextLevelIrrelevant = nextLevelIrrelevant;
            bestAttr = attrNum;
            bestInstanceCovers = numInstanceCovers;
            bestConsistentChoices = numConsistentChoices;
         }
      } 

      // prefer later attributes (nicer)
      if (bestNextLevelIrrelevant == -1) { // no irrelevant yet
         int result = bestSize - (numBags - stats.num_const_bags());
         if (result == 0) { // tie
            result = bestNumDiffDests - currNumDiffDests;
            if (result == 0) { // tie
               result = bestInstanceCovers - numInstanceCovers;
               if (result == 0) { // tie
                  result = bestConsistentChoices - numConsistentChoices;
               }
            }
         }
         if (result >= 0) { // winner
            bestAttr = attrNum;
            bestSize = numBags - stats.num_const_bags();
            bestInstanceCovers    = numInstanceCovers;
            bestConsistentChoices = numConsistentChoices;
            bestNumDiffDests      = currNumDiffDests;
         }
      }
      while (!pipl->empty())
	 delete pipl->remove_front(); 
      delete pipl; DBG(pipl = NULL);
   }
   ASSERT(bestAttr != -1);
   set_cover_instance_heuristic(saveCIH);

   return bestAttr;
}      


/***************************************************************************
  Description : Prune nodes which are not significant.
  Comments    :
***************************************************************************/

void HOODGInducer::prune_nodes(ProjInfoPtrList& pipl)
{
   (void)pipl;
   ASSERT(pipl.length() > 0);
   
   if (get_prune_ratio() == 0)
      return; // we are not suppose to prune (save time)
   
   Bool pruned;
   do {
      int  totalInstances = 0;
      Real totalWeight    = 0;
      Real maxWeight      = 0;
      Real maxRatio       = 0;
      Real sumRatio       = 0;
      int  maxInstances   = -1;
      int  numBags        = pipl.length();
      for (ProjInfoPtrPix pix(pipl, 1); pix; ++pix) {
         int numInstances = pipl(pix)->bag->num_instances();
         Real weight = pipl(pix)->bag->total_weight();
         totalInstances +=  numInstances;
         totalWeight    += weight;
         maxInstances = max(maxInstances, numInstances);
         maxWeight    = max(maxWeight, weight);
         int numDiffDests = DestArray::num_different_dests(pipl(pix)->
                                                           dest_bag());
         sumRatio     += (Real)weight / numDiffDests;
         maxRatio     = max(maxRatio, (Real)weight / numDiffDests);
      }

      Real avgRatio = sumRatio / numBags;
      ASSERT(maxInstances > 0);
      ASSERT(maxWeight > 0);

      Real threshold = (get_prune_ratio() * avgRatio);
      LOG(1, "HOODGInducer::prune_ratio: max="
          << maxInstances << '/' << maxWeight << " instances/weight. "
          << "Total=" << totalInstances << '/' << totalWeight << endl
          << "  Num bags=" << numBags
          << ". Avg/Max ratio =" << avgRatio << '/' << maxRatio 
          << ". Threshold=" << threshold << endl);


      pix.first();
      pruned = FALSE;
      while (pix) {
         int numInstances = pipl(pix)->bag->num_instances();
         Real weight = pipl(pix)->bag->total_weight();
         int  numDiffDests = DestArray::num_different_dests(
                                                      pipl(pix)->dest_bag());
         LOG(3, "Bag has " << numInstances << " instances, dests=" <<
            numDiffDests << " ratio " << (Real)weight /
             numDiffDests << endl);
         if ((Real)weight / numDiffDests <= threshold) {
            LOG(1, "HOODGInducer::prune_nodes: pruning bag with "
                << numInstances << '/' << weight 
                << " instances/weight, ratio="
                << (Real)weight/numDiffDests << endl);
            pipl.del(pix); 
            pruned = TRUE;
         }
         else  
            ++pix;
      }

      // This could happen with too high pruning...
      if (pipl.length() == 0)
         err << "HOODGInducer::prune_nodes: overpruned" << fatal_error;
   } while (pruned);
}


   

