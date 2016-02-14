// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : This file defines some HOODG heuristics for instance covering.
                 Usage is as follows:
                   init() is called to zero counters (statistics).
                   init_instance_cover() is called when a new instance
                     needs to be covered.
                   is_better() is called to determine if the current
                     instance is better than all others since
                     init_instance_cover().
                   init_level() allows getting statistics for a set of
                      instances, presumably forming a level.
                   level_stats() and total_stats() display statistics
                     for the level (since init_level) and for all 
                     instances (since init).
  Assumptions  : 
  Comments     : The end_instance_cover() and end_level_cover() calls
                   are optional and are automatically called by the init
                   and statistics routines.
                 The init calls and end calls are idempotent (may be called 
                   more than once without any counter errors).
  Complexity   :
  Enhancements :
  History      : Ronny Kohavi                                       1/09/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <HOODGInducer.h> // this includes HOODGCIH.h
#include <distance.h>

RCSID("MLC++, $RCSfile: HOODGCIH.c,v $ $Revision: 1.10 $")

/***************************************************************************
  Description : Constructor, init().  This initializes all variables.
                This should be called once when training begins.
  Comments    :
***************************************************************************/

CoverInstanceHeuristic::CoverInstanceHeuristic()
{
   init();
   bestPiwbID = 1;
}


void CoverInstanceHeuristic::init()
{
   ip = NULL;
   numInstancesAtLevel = totalNumInstanceCovers = 0;
}

/***************************************************************************
  Description : init_instance_cover / end_instance_cover,
                init_level_cover / end_level_cover update/zero
                  the appropriate counters.
  Comments    : A non-null ip means a cover_instance is in progress.
                The end_X routines must be idempotent in derived classes.
***************************************************************************/

void CoverInstanceHeuristic::init_instance_cover(const 
                                        InstanceProjection* anIP)
{
   if (anIP == NULL)
      err << "CoverInstanceHeuristic::init_instance_cover: NULL "
          << "projection" << fatal_error;

   end_instance_cover();  // end previous one, start new one.
   ip = anIP;
   LOG(4, "CoverInstanceHeuristic:init_instance_cover: got instance\n   "
       << *ip);
   bestPiwbID++;
   bestPiwb = NULL;
}

void CoverInstanceHeuristic::end_instance_cover()
{
   if (ip) { // OC warning about pointing to free memory can be ignored.
             // All we care about is whether it's not NULL.
      numInstancesAtLevel++;
      ip = NULL;
   }
}

void CoverInstanceHeuristic::init_level_cover()
{
   end_level_cover();
}


void CoverInstanceHeuristic::end_level_cover()
{
   end_instance_cover();
   totalNumInstanceCovers += numInstancesAtLevel;
   numInstancesAtLevel = 0;
}
   


/***************************************************************************
  Description : Determine if the bestPiwb changed given a counter which
                  is what the caller thinks is the id of the bestPiwb.
  Comments    : Very useful for heuristics that compute complex stuff
                  and want to cache their result of the "best."
***************************************************************************/

Bool CoverInstanceHeuristic::cache_invalid(long& givenPiwbID)
{
   ASSERT(givenPiwbID <= bestPiwbID);
   
   if (givenPiwbID < bestPiwbID) {
      givenPiwbID = bestPiwbID;
      return TRUE;
   } else
      return FALSE;
}
   

/***************************************************************************
  Description : Statistics
  Comments    : Statistics on level ends the instance_cover and total
                  statistics ends the level cover.
***************************************************************************/

void CoverInstanceHeuristic::level_stats(MLCOStream& ostream)
{
   end_instance_cover();

   ostream << "CoverInstanceHeuristic level summary:\n"
      << "  Number of unforced instance covers: " << numInstancesAtLevel
      << endl;
}   


void CoverInstanceHeuristic::total_stats(MLCOStream& ostream)
{
   end_level_cover();
   ostream << "CoverInstanceHeuristic total summary:\n"
        << "  Total number of unforced instance covers: "
        << totalNumInstanceCovers << endl;
}   

/***************************************************************************
  Description : Get specific statistics.
  Comments    :
***************************************************************************/
int CoverInstanceHeuristic::total_instance_covers()
{
   end_level_cover();
   return totalNumInstanceCovers;
}



/***************************************************************************
  Description  : The CIHMinDests heuristic is probably the simplest
                   reasonable heuristic.
                 It prefers consistent instances with the minimal number of
                   new destinations.  This generalizes the preference for
                   included destinations over consistent but not included.
                   This is a variant of "least commitment."
                 For tie-breaking, we prefer the bag with the closest
                   "distance" (see distance.[ch] @@temporary).
  Assumptions  : 
  Comments     : Note that tie-breaking after our distance metric breaker
                   is simply toward earlier instances.
  Complexity   : Takes DestArray::consistent_in_dest +
                   DestArray::num_new_dests_from_merge
  Enhancements :
  History      : Ronny Kohavi                                       1/10/94
                   Initial revision (.h,.c)
***************************************************************************/

/***************************************************************************
  Description : Constructor and initialization functions.
  Comments    :
***************************************************************************/

// Private.  
void CIHMinDests::_init()
{
   numIncludedChoicesInst     = -1;
   numConsNotIncChoicesInst   = -1;

   numIncludedChoicesLevel    =  0;
   numConsNotIncChoicesLevel  =  0;

   totalIncludedChoices       =  0;
   totalConsNotIncChoices     =  0;
}   


CIHMinDests::CIHMinDests()
{
   _init();
   piwbBND = piwbBMD = piwbBD = piwbBAD = 0;
}

void CIHMinDests::init()
{
   CoverInstanceHeuristic::init();
   _init();
}

void CIHMinDests::init_level_cover()
{
   CoverInstanceHeuristic::init_level_cover();
   numIncludedChoicesLevel   = 0;
   numConsNotIncChoicesLevel = 0;
}

void CIHMinDests::end_level_cover()
{
   CoverInstanceHeuristic::end_level_cover();
   
   // Update total counters
   totalIncludedChoices        += numIncludedChoicesLevel;
   totalConsNotIncChoices      += numConsNotIncChoicesLevel;
   numIncludedChoicesLevel   = 0;
   numConsNotIncChoicesLevel = 0;
}   
   

void CIHMinDests::
     init_instance_cover(const InstanceProjection* anIP)
{
   // Calls end_instance_cover()
   CoverInstanceHeuristic::init_instance_cover(anIP); 

   numIncludedChoicesInst    = -1;
   numConsNotIncChoicesInst  = -1;
}

void CIHMinDests::end_instance_cover()
{
   CoverInstanceHeuristic::end_instance_cover();
   
   numIncludedChoicesLevel    += max(numIncludedChoicesInst,     0);
   numConsNotIncChoicesLevel  += max(numConsNotIncChoicesInst, 0);

   numIncludedChoicesInst    = 0; // 0 or -1 (must not affect above sum)
   numConsNotIncChoicesInst  = 0; 
}
   
/***************************************************************************
  Description : Heuristics for preferring the BEST piwb to cover an instance.
                The idea is that each heuristic is called when all
                  higher level heuristics have reached a tie-breaker
                  and can't determine what is better.
                The heuristic can check the cache_invalid to determine
                  if their cached value is valid.
                Return win_cih, lose_cih, tie_cih.
  Comments    : Each heuristic that returns win_CIH should store 
                  the new values and increment its myPIwbID (the bestPiwb
                  counter will be incremented by the caller).
                  If the caller does not set BestPiwb to the piwb,
                  then the counter should be incremented twice.
                This is pretty ugly.  Is there a nicer way to do this?
***************************************************************************/

// This heuristic prefers the new instance if it adds fewer
//    "new" destinations to values not yet defined.
//    It generalizes the heuristic to prefer bags
//    where the current instance is "included" (i.e., 0 new destinations).
CIHResult CIHMinDests::better_new_dests(const ProjInfoWithBag& piwb)
{
   if (cache_invalid(piwbBND)) {
      if (bestPiwb)
         bestNewDests = DestArray::num_new_dests_from_merge(
                                       bestPiwb->dest_bag(), ip->dest_bag());
      else
         bestNewDests = INT_MAX;
   }
   
   int numNewDests = DestArray::num_new_dests_from_merge(piwb.dest_bag(),
                                                         ip->dest_bag());
   if (numNewDests == 0) // included instance
      numIncludedChoicesInst++;
   else                  // consistent but not included
      numConsNotIncChoicesInst++;

   if (numNewDests < bestNewDests) {
      LOG(4, "improved number of destinations to " << numNewDests << endl);
      bestNewDests = numNewDests;
      piwbBND++;
      return winCIH;
   } else if (numNewDests == bestNewDests)
      return tieCIH;
   else {
      LOG(4, "lost better_new_dests" << endl);
      return loseCIH;
   }
}

// This heuristic prefers an instance that has the fewest overall
//   different destinations when merged with another instance.

CIHResult CIHMinDests::better_merged_dests(const ProjInfoWithBag& piwb)
{
   if (cache_invalid(piwbBMD)) {
      if (bestPiwb)
         bestDiffDests = DestArray::num_diff_dests_after_merge(
                                   bestPiwb->dest_bag(), ip->dest_bag());
      else
         bestDiffDests = INT_MAX;
   }

   int diffDests = DestArray::num_diff_dests_after_merge(
                                   piwb.dest_bag(), ip->dest_bag());
   if (diffDests < bestDiffDests) {
      LOG(4, "improved number of different destinations to "
          << diffDests << endl);
      bestDiffDests = diffDests;
      piwbBMD++;
      return winCIH;
   } else if (diffDests == bestDiffDests) 
      return tieCIH;
   else  {
      LOG(4, "lost better_merged" << endl);
      return loseCIH;
   }
}
   
CIHResult CIHMinDests::better_distance(const ProjInfoWithBag& piwb)
{
   InstanceRC instance(ip->instance);

   if (cache_invalid(piwbBD)) {
      if (bestPiwb)
         bestDistance = bag_dist(*bestPiwb->bag, instance, bestNumDistance);
      else
         bestDistance = REAL_MAX;
   }

   int currNumDistance;
   Real currDistance = bag_dist(*piwb.bag, instance, currNumDistance);
   if (currDistance <  .5)
      LOG(1, "Distance is " << currDistance << ", count="
        << currNumDistance << endl);

   if (currDistance < 2 && currDistance < bestDistance ||
       (currDistance == bestDistance && currNumDistance > bestNumDistance)) {
      LOG(4, "improved distance to " << currDistance << " ("
          << bestNumDistance << " instances)" << endl);
      bestDistance    = currDistance;
      bestNumDistance = currNumDistance;
      piwbBD++;
      return winCIH;
   } else if (currDistance == bestDistance && 
              currNumDistance == bestNumDistance) 
      return tieCIH;
   else {
      LOG(4, "lost better_distance" << endl);
      return loseCIH;
   }
}

CIHResult CIHMinDests::better_avg_distance(const ProjInfoWithBag& piwb)
{
   InstanceRC instance(ip->instance);

   if (cache_invalid(piwbBAD)) {
      if (bestPiwb)
         bestAvgDistance = avg_bag_dist(*bestPiwb->bag, instance);
      else
         bestAvgDistance = REAL_MAX;
   }

   Real currAvgDistance = avg_bag_dist(*piwb.bag, instance);

   if (currAvgDistance < bestAvgDistance) {
      LOG(4, "improved average distance to " << currAvgDistance << endl);
      bestAvgDistance  = currAvgDistance;
      piwbBAD++;
      return winCIH;
   } else if (currAvgDistance == bestAvgDistance)
      return tieCIH;
   else {
      LOG(4, "lost better_avg_distance" << endl);
      return loseCIH;
   }
}


/***************************************************************************
  Description : Return true if the current instance is better than the
                  best one returned TRUE for.
  Comments    : See class description for a description of this function.
***************************************************************************/

Bool CIHMinDests::is_better(const ProjInfoWithBag& piwb)
{
   if (ip == NULL)
      err << "CIHMinDests::is_better: init_instance_cover not called"
          << fatal_error;

   LOG(4, "CIHMinDests::is_better: ");
   LOG(4, "\n   destinations for instance are:" << ip->dest_bag()
       << "\n   for bag are                  :" << piwb.dest_bag()
       << endl << "  ");
   // Return if not consistent.  We obviously canot merge with this instance.
   if (!DestArray::consistent_dests(ip->dest_bag(), piwb.dest_bag())) {
      LOG(4, "contradicting bag" << endl);
      return FALSE;
   }
   
   CIHResult result = better_new_dests(piwb);
   if (result == tieCIH) {
      result = better_distance(piwb);
      if (result == tieCIH) {
         result = better_merged_dests(piwb);
         if (result == tieCIH) {
//            result = better_avg_distance(piwb);
//            if (result == tieCIH) {
               LOG(5, "CIHMINDests::**** out of tie breakers ***" <<endl);
               return FALSE;
//          }
         }
      }
   }
   ASSERT(result != tieCIH);
   if (result == winCIH) {
      bestPiwb = &piwb;
      bestPiwbID++;
      return TRUE;
   }

   return FALSE;
}
      
/***************************************************************************
  Description : Statistics
  Comments    :
***************************************************************************/

void CIHMinDests::level_stats(MLCOStream& ostream)
{
   CoverInstanceHeuristic::level_stats(ostream);
   ostream << "  Consistent choices: "   << 
     numIncludedChoicesLevel + numConsNotIncChoicesLevel 
     << ", Included choices: "     << numIncludedChoicesLevel
     << ", Consistent & not incl: " << numConsNotIncChoicesLevel << endl;
}

void CIHMinDests::total_stats(MLCOStream& ostream)
{
   CoverInstanceHeuristic::total_stats(ostream);
   ostream << "  Consistent choices: "   << 
       totalIncludedChoices + totalConsNotIncChoices
       << ", Included choices: "     << totalIncludedChoices
       << ", Consistent & not incl: " << totalConsNotIncChoices << endl;
}

/***************************************************************************
  Description : Get specific statistics.
  Comments    :
***************************************************************************/

int CIHMinDests::total_included_choices()
{
   end_level_cover();
   return totalIncludedChoices;
}

int CIHMinDests::total_cons_not_inc_choices()
{
   end_level_cover();
   return totalConsNotIncChoices;
}


int CIHMinDests::total_consistent_choices()
{
   end_level_cover();
   
   return totalConsNotIncChoices + totalIncludedChoices;
}
