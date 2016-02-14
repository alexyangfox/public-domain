// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _HOODGCIH_h
#define _HOODGCIH_h 1

#include <LogOptions.h>

class InstanceProjection;
class ProjInfoWithBag;

class CoverInstanceHeuristic {
   LOG_OPTIONS;
   // Note that not every instance goes through us.  Some are forced,
   // hence the number of calls at a level is less than the number of
   // projected instances.
   int totalNumInstanceCovers;   // calls to init_instance_cover.
   int numInstancesAtLevel;      // same but for this level only.
protected:
   const InstanceProjection* ip; // stores working instance
   const ProjInfoWithBag* bestPiwb; // pointer to "best" piwb until now.
   long  bestPiwbID;  // incremented everytime we is_better returns
                      //   TRUE (meaning that bestPiwb changed), and
                      //   every end_instance_cover.  Used
                      //   by heuristics to invalidate their cached values.
   
public:
   CoverInstanceHeuristic();
   virtual ~CoverInstanceHeuristic() {};
   virtual void init();
   virtual void init_instance_cover(const InstanceProjection *);
   virtual void end_instance_cover(); // optional (no need to call)
   virtual void init_level_cover();
   virtual void end_level_cover();    // optional (no need to call)
   virtual Bool is_better(const ProjInfoWithBag& piwb) = 0;
   virtual Bool CoverInstanceHeuristic::cache_invalid(long& givenPiwbID);
   virtual void level_stats(MLCOStream& ostream);
   virtual void total_stats(MLCOStream& ostream);
   virtual int  total_instance_covers();
};


// CIHMinDests prefers consistent instances with the minimal number of
//    new destinations.  This generalizes the preference for included
//    destinations over consistent but not included.
// The best piwb is stored so that tie-breakers can check if they are
//    better.  

typedef enum {winCIH, loseCIH, tieCIH} CIHResult;


class CIHMinDests : public CoverInstanceHeuristic {
   void _init();                    // our init
protected:
   // better_new_dests members
   long piwbBND;
   int  bestNewDests;

   // better_merged_dests members
   long piwbBMD;
   int bestDiffDests;

   // better_distance members
   long piwbBD;
   Real bestDistance;
   int  bestNumDistance;

   // better_avg_distance members
   long piwbBAD;
   Real bestAvgDistance;

   // Instance counters.
   int numIncludedChoicesInst;  
   int numConsNotIncChoicesInst; // consistent but not included

   // level counters
   int numIncludedChoicesLevel;
   int numConsNotIncChoicesLevel;

   // total counters
   int totalIncludedChoices;
   int totalConsNotIncChoices;

public:
   CIHMinDests();
   virtual ~CIHMinDests() {};
   virtual void init();
   virtual void init_instance_cover(const InstanceProjection *);
   virtual void end_instance_cover();
   virtual void init_level_cover();
   virtual void end_level_cover();
   virtual CIHResult better_new_dests(const ProjInfoWithBag&);
   virtual CIHResult better_merged_dests(const ProjInfoWithBag&);
   virtual CIHResult better_distance(const ProjInfoWithBag&);
   virtual CIHResult better_avg_distance(const ProjInfoWithBag& piwb);
   virtual Bool is_better(const ProjInfoWithBag& piwb);
   virtual void level_stats(MLCOStream& ostream);
   virtual void total_stats(MLCOStream& ostream);
   virtual int  total_included_choices();
   virtual int  total_cons_not_inc_choices();
   virtual int  total_consistent_choices();
};
   
#endif
