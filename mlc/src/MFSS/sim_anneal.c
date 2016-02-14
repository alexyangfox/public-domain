// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : sim_anneal picks a slot in an array of real (accuracy)
                   using lambda as a parameter.
  Assumptions  :
  Comments     :
  Complexity   : 
  Enhancements :
  History      : Dan Sommerfield                                   12/20/94
                   Initial revision (.h,.c)
***************************************************************************/
#include <basics.h>
#include <math.h>
#include <Array.h>
#include <MRandom.h>
#include <LogOptions.h>

RCSID("MLC++, $RCSfile: sim_anneal.c,v $ $Revision: 1.2 $")

int sim_anneal(const Array<Real>& table, Real lambda, MRandom& randNumGen)
{
   // table must be greater than size one
   ASSERT(table.size() > 1);
   
   // initialize a new array
   Array<Real> array(table, ctorDummy);
   
   Real sum = 0;
   Real first = array[0];
   for(int slot = array.low(); slot <= array.high(); slot++) {
      array[slot] = exp((array[slot] - first) / lambda);
      sum += array[slot];
   }

   GLOBLOG(3, "sum: " << sum << "  table: " << array << endl);

   // throw a random number from 0 to sum
   Real val = randNumGen.real(0, sum);
   GLOBLOG(3, "number: " << val << endl);

   // pick the slot.  It should never fall out of the array.
   for(slot = array.low(); slot <= array.high(); slot++) {
      if(val <= array[slot])
	 break;
      else
	 val -= array[slot];
   }
   ASSERT(slot <= array.high());
   GLOBLOG(3, "slot: " << val << endl);

   return slot;
}


   
