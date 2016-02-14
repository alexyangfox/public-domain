// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : A collection of routines to deal with arrays of 
                   destination bags (see ProjBag.c).
                 A destination bag is a bag consisting of instances.
                   It is numbered by a NominalValue.  The array assigns
                   each nominal value a destination bag.
  Assumptions  :
  Comments     : The collection is under a class DestBag for grouping
                   the routines together.  There are no data members.
  Complexity   : All routines take linear time in the size of the
                    arrays passed.
  Enhancements : Improve num_different_dests by a constant of two as
                   suggested in the class header.
                 Some of these calls can be computed together in one pass.
                   Perhaps this class should have a constructor that
                   computes everything and the calls would then just
                   return the information.
  History      : Ronny Kohavi                                       1/09/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <DestArray.h>

RCSID("MLC++, $RCSfile: DestArray.c,v $ $Revision: 1.7 $")

/***************************************************************************
  Description : Return the destination shared by all elements of the
                  desination array that do not have UNKNOWN_NOMINAL_VAL. 
                  If they do not share, return UNKNOWN_NOMINAL_VAL.
  Comments    : We assume that there is at least one destination in
                  the array.
***************************************************************************/

NominalVal DestArray::same_dest(const Array<NominalVal>& dests)
{
   // Once sameDest changes, every value must match it.
   NominalVal sameDest = UNKNOWN_NOMINAL_VAL; 
   for (int i = 0; i < dests.size(); i++) {
      if (dests.index(i) != UNKNOWN_NOMINAL_VAL) {
         if (sameDest == UNKNOWN_NOMINAL_VAL) // we found one
            sameDest = dests.index(i);
         else if (sameDest != dests.index(i))
            return UNKNOWN_NOMINAL_VAL;
      }
   }
   // Abort if everything was UNKNOWN_NOMINAL_VAL because the
   //   destination array should not have been created.
   ASSERT(sameDest != UNKNOWN_NOMINAL_VAL);
   return sameDest;
}


/***************************************************************************
  Description : Check if two destBag arrays are consistent, i.e. there
                  is no conflicting destination.
                Phrased differently, two bags are NOT consistent
                  if there exists a value (array index) for which
                  the destinations exist (i.e., not UNKNOWN) and are
                  different. 
  Comments    : This routine is time critical.  Profiling showed it
                  was executed millions of times and took 10% of the
                  overall time on the vote dataset.
***************************************************************************/

Bool DestArray::consistent_dests(const Array<NominalVal>& dests1, 
                                 const Array<NominalVal>& dests2)
{
   ASSERT(dests1.size() == dests2.size());


   // Note that we step back to avoid calling dest1.size() every iteration
   // Also, the order of the terms in the conjunction is important.
   for (int i = dests1.size() - 1 ; i >= 0 ; i--)
      if (dests1.index(i) != dests2.index(i) &&
          dests1.index(i) != UNKNOWN_CATEGORY_VAL &&
          dests2.index(i) != UNKNOWN_CATEGORY_VAL)
         return FALSE;

   return TRUE;
}
   
/***************************************************************************
  Description : Check if the known destinations of dests1 are the same
                   in dests2 (but not necessarily vice versa).
                   [This has been called "agree" in the ECML-94 paper.]
                Phrased differently, dest-array-1 is not included
                   in dest-array-2 if there is a value (array index)
                   for which there is a known destination (not UNKNOWN)
                   in dest-array-1 that is different in dest-array-2.
  Comments    : This is a stronger requirement than consistency (and
                   it is also asymmetric).  
***************************************************************************/

Bool DestArray::included_in_dest(const Array<NominalVal>& dests1, 
                                 const Array<NominalVal>& dests2)
{
   ASSERT(dests1.size() == dests2.size());

   for (int i = 0; i < dests2.size(); i++)
      if (dests1.index(i) != UNKNOWN_CATEGORY_VAL &&
          dests2.index(i) != dests1.index(i))
         return FALSE;

   return TRUE;
}

/***************************************************************************
  Description : Merge the two sets of destinations into the first.
                Return the number of new destination entries added.
                Any destinationsin the second and not in the first
                   is added to the first.
  Comments    : The dest arrays are assumed to be consistent.
***************************************************************************/

int DestArray::merge_dests(Array<NominalVal>& dests1, Array<Real>& destCount1,
               const Array<NominalVal>& dests2, const Array<Real>& destCount2)
{
   ASSERT(DestArray::consistent_dests(dests1, dests2));

   int newEntries = 0;
   
   for (int i = 0; i < dests1.size(); i++) {
      if (dests1.index(i) != UNKNOWN_CATEGORY_VAL &&
          dests1.index(i) == dests2.index(i)) {
         ASSERT(destCount1.index(i) > 0);         
         ASSERT(destCount2.index(i) > 0);         
         destCount1.index(i) += destCount2.index(i);
      }
      else if (dests1.index(i) == UNKNOWN_CATEGORY_VAL &&
          dests2.index(i) != UNKNOWN_CATEGORY_VAL) {
         dests1.index(i) = dests2.index(i);
         ASSERT(destCount2.index(i) > 0);
         destCount1.index(i) = destCount2.index(i);
         newEntries++;
      }
   }

   return newEntries;
}

/***************************************************************************
  Description : Find the number of destinations != UNKNOWN_CATEGORY_VAL.
  Comments    : We assume that there is at least one destination in
                  the array.
***************************************************************************/

int DestArray::num_dests(const Array<NominalVal>& dests)
{
   int count = 0;
   
   for (int i = 0; i < dests.size(); i++)
      count += (dests.index(i) != UNKNOWN_CATEGORY_VAL);

   ASSERT(count > 0); // must be > 0 destinations.
   return count;
}


/***************************************************************************
  Description : Find the number of different destinations in the
                  array, ignoring UNKNOWN_CATEGORY_VAL.
  Comments    :
***************************************************************************/

int DestArray::num_different_dests(const Array<NominalVal>& dests)
{
   // Array of counters.  We find the maximum destination and create
   //   an array of the appropriate size.  This allows for linear time
   //   computation of the number of different destinations.
   Array<Real> counts(UNKNOWN_CATEGORY_VAL,      
                         dests.max() - UNKNOWN_CATEGORY_VAL + 1, 0.0);
   for (int i = 0; i < dests.size(); i++)
      counts[dests.index(i)]++;

   int diff_dests = 0;
   for (i = FIRST_CATEGORY_VAL; i <= counts.high(); i++)
      if (counts[i])
         diff_dests++;

   return diff_dests;
}


/***************************************************************************
  Description : Find the number of new destinations a dest array would have
                  if merged with another one.
                Note that by new destinations we mean new destinations
                  which were previously UNKNOWN.  These destinations
                  may or may not be the destinations for other values.
  Comments    : We can improve efficiency a bit by simulating the
                  merge and not actually merging to a temporary.
***************************************************************************/

int DestArray::num_new_dests_from_merge(const Array<NominalVal>& dests1,
                                        const Array<NominalVal>& dests2)
{
   Array<NominalVal> mergedDests(dests1, ctorDummy);     // copy constructor
   Array<Real>        dummyCounts(dests1.low(), dests1.size(), 1.0);
   return merge_dests(mergedDests, dummyCounts, dests2, dummyCounts);
}

/***************************************************************************
  Description : Find the number of different destinations a dest array
                  would have if merged with another one.
  Comments    : We can improve efficiency a bit by simulating the
                  merge and not actually merging to a temporary.
***************************************************************************/

int DestArray::num_diff_dests_after_merge(const Array<NominalVal>& dests1,
                                          const Array<NominalVal>& dests2)
{
   Array<NominalVal> mergedDests(dests1, ctorDummy); // copy constructor
   Array<Real>        dummyCounts(dests1.low(), dests1.size(), 1.0);
   merge_dests(mergedDests, dummyCounts, dests2, dummyCounts);
   return num_different_dests(mergedDests);
}

      
