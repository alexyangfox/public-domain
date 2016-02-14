// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the global function ::sim_anneal

  Doesn't test :
  Enhancements :
  History      : initial revision                                  12/20/94
                   Dan Sommerfield

***************************************************************************/

#include <basics.h>
#include <sim_anneal.h>
#include <MRandom.h>

RCSID("MLC++, $RCSfile: t_sim_anneal.c,v $ $Revision: 1.4 $")

const unsigned int seed = 4970798;

void t_sim_anneal(const Array<Real>& table, Real lambda, int times)
{
   Array<int> hist(table.low(), table.size(), 0);

   MRandom randNumGen(seed);

   // repeat test by getting a slot each time.  Accumulate the slots we
   // pick into the histogram.
   for(int i=0; i<times; i++) {
      int slot = sim_anneal(table, lambda, randNumGen);
      hist[slot]++;
   }

   // display the histogram, including percent of total
   Mcout << "Lambda: " << lambda << endl;
   for(i = hist.low(); i <= hist.high(); i++) {
      Mcout << "Slot " << i << " (" << table[i] << "): " << hist[i]
	 << " (" << MString(((Real)(hist[i])/(Real)times) * 100, 2)
	 << "%)" << endl;
   }
   Mcout << endl;
}

Real tab[] = {0.96, 0.94, 0.92, 0.90, 0.85, 0.80, 0.73, 0.62, 0.50};
Real tabeq[] = {0.92, 0.92, 0.92, 0.92, 0.92, 0.92, 0.92, 0.92, 0.92};
main()
{
   int size = (sizeof tab)/sizeof(Real);
   Array<Real> table(size);
   for(int i=0; i<size; i++)
      table[i] = tab[i];
   Mcout << "Table: " << table << endl;

   // run the test 1000 times for each lambda value
   t_sim_anneal(table, 0.02, 1000);
   t_sim_anneal(table, 0.05, 1000);
   t_sim_anneal(table, 0.10, 1000);
   t_sim_anneal(table, 0.20, 1000);

   // run again on tableeq
   size = (sizeof tabeq)/sizeof(Real);
   Array<Real> tableeq(size);
   for(i=0; i<size; i++)
      tableeq[i] = tabeq[i];
   Mcout << "Table-Equality: " << table << endl;

   // run the test 1000 times for each lambda value
   t_sim_anneal(tableeq, 0.02, 1000);
   t_sim_anneal(tableeq, 0.05, 1000);
   t_sim_anneal(tableeq, 0.10, 1000);
   t_sim_anneal(tableeq, 0.20, 1000);

   return 0;
}

   
   
