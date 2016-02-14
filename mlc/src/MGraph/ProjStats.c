// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Statistics about projection bags.
                 The constructor computes all the statistics and
                    everything else is just accessor functions.
                 The fields are as follows:
                   numConstBags - the number of bags for which all 
                                  destinations are the same (or UNKNOWN).
                   numUnknown   - total number of unknown destination.
                   numDiffDests - total number of different destination,
                                  i.e., for each node, sum the number
                                  of different destinations.  This is
                                  the number of edges that would have
                                  to be used if multi-labelled edges
                                  are used.
  Assumptions  : Each bag in the list has at least one instance.
  Comments     : 
  Complexity   : @@ fill in
  Enhancements : 
  History      : Ronny Kohavi                                       1/14/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <ProjStats.h>
#include <DestArray.h>
#include <BagSet.h>

RCSID("MLC++, $RCSfile: ProjStats.c,v $ $Revision: 1.7 $")


/***************************************************************************
  Description : Constructor
  Comments    : 
***************************************************************************/

ProjStats::ProjStats(const ProjInfoPtrList& pipl, const LogOptions& logOptions)
{
   set_log_options(logOptions);
   
   numUnknowns = numConstBags = numDiffDests = 0;
   numProjections = 0;
   weightInstances = 0.0;
   
   int bagNum = 0;
   LOG(3, "ProjStats::ProjStats statistics for Bags:" << endl);

   for (ProjInfoPtrPix pix(pipl, 1); pix; ++pix, ++bagNum) {
      const ProjInfoWithBag& piwb = *pipl(pix); 
      ASSERT(!piwb.bag->no_instances());
      
      Real bagWeight = piwb.weight;
      int bagProjections = piwb.bag->num_instances();
      weightInstances += bagWeight;
      numProjections += bagProjections;

      // Bags that agree on ONE destination are considered constant.
      int bagNumUnknowns = 0;
      if (DestArray::same_dest(piwb.dest_bag()) != UNKNOWN_NOMINAL_VAL)
         numConstBags++;

      bagNumUnknowns = piwb.dest_bag().size() - piwb.num_dests();
      // Do not count the destination for UNKNOWN_NOMINAL_VAL
      // (it was already subtracted if it is not empty)
      if (piwb.dest_bag().index(0) == UNKNOWN_NOMINAL_VAL)
         bagNumUnknowns--;

      ASSERT(bagNumUnknowns >= 0);
      int bagDiffDests = DestArray::num_different_dests(piwb.dest_bag());
      numDiffDests += bagDiffDests;
      numUnknowns += bagNumUnknowns;

      LOG(3, "  Bag: " << bagNum 
          << ", Unknowns: " << bagNumUnknowns  
          << ", Projections: " << bagProjections 
          << ", Overall instance weight: " << bagWeight 
          << ", Different dests: " << bagDiffDests << endl);
   }
   LOG(2, "ProjStats::ProjStats: Bags: " << pipl.length()
          << ", Projections: " << numProjections << ',' << endl
          << "  Overall instances weight " << weightInstances
          << ", Different dests: " << numDiffDests << endl);
}

