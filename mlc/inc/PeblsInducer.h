// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _PeblsInducer_h
#define _PeblsInducer_h 1

#include <BaseInducer.h>
#include <Inducer.h>
#include <MLCStream.h>
#include <LogOptions.h>

#define PEBLS_INDUCER 24

enum PeblsVotingScheme { MAJORITY, WEIGHTED_DISTANCE };

class PeblsInducer : public BaseInducer {
private:   
   MString pgmName;   

   // options.
   int discretizationLevels;
   int nearestNeighbors;
   PeblsVotingScheme votingScheme;

public:
   static MString defaultPgmName;
   static int defaultDiscretizationLevels;
   static int defaultNearestNeighbors;
   static PeblsVotingScheme defaultVotingScheme;
   
   PeblsInducer(const MString& description, 
		const MString& thePgmName = defaultPgmName);

   virtual ~PeblsInducer();
   virtual int class_id() const { return PEBLS_INDUCER; }
   virtual Real train_and_test(InstanceBag* trainingSet,
			       const InstanceBag& testList);
   virtual void set_pgm_name(const MString& name) { pgmName = name; }
   virtual MString get_pgm_name() const { return pgmName; }
   virtual void set_discretization_levels(int levels) {
      discretizationLevels = levels; }
   virtual int get_discretization_levels() const {
      return discretizationLevels; }
   virtual void set_nearest_neighbors(int num) { nearestNeighbors = num; }
   virtual int get_nearest_neighbors() const { return nearestNeighbors; }
   virtual void set_voting_scheme(PeblsVotingScheme value) {
      votingScheme = value; }
   virtual PeblsVotingScheme get_voting_scheme() const {
      return votingScheme; }
   virtual void set_user_options(const MString& prefix);
};
#endif





