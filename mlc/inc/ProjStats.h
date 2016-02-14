// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _ProjStats_h
#define _ProjStats_h 1

#include <ProjBag.h>
#include <LogOptions.h>

class ProjStats {
   LOG_OPTIONS;
   int numProjections;
   Real weightInstances;   // Total weight of represented instances.
   int numConstBags;
   int numUnknowns;
   int numDiffDests; // overall edges if we had multi-labelled edges
public:
   ProjStats(const ProjInfoPtrList& pipl, const LogOptions& logOptions);
   int num_const_bags() {return numConstBags;}
   int num_unknowns()   {return numUnknowns;}
   int num_diff_dests() {return numDiffDests;}
   int weight_instances()  {return weightInstances;}
};
#endif
