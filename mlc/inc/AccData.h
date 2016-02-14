// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _AccData_h
#define _AccData_h 1

#include <LogOptions.h>
#include <MRandom.h>
#include <StatData.h>


// AccData is a class which encapsulates statistical data collected
// during accuracy estimation
class AccData : public StatData {
   NO_COPY_CTOR(AccData);

   // real accuracy information
   Real realAccuracy;
   int numTestInstances;

   // evaluation cost (number of runs of inducer to get results)
   int evalCost;
   
public:
   static int defaultPrecision;
 
   virtual void OK() const;

   AccData() : realAccuracy(-1), numTestInstances(0), evalCost(0) { }
   
   virtual void set_real(Real realAcc, int n);
   virtual void insert_cost(int cost) { evalCost += cost; }
   
   virtual Real accuracy(Real trim = 0) const;
   virtual Real std_dev(Real trim = 0) const;
   virtual void confidence(Real& low, Real& high) const;

   virtual Real real_accuracy() const;
   virtual Real theo_std_dev() const;
   virtual void theo_confidence(Real& low, Real& high) const;
   virtual Real bias(Real trim = 0) const {
      return accuracy(trim) - real_accuracy(); }

   virtual Bool has_real() const { return realAccuracy >= 0.0; }     
   virtual void check_real() const;
   virtual Bool has_estimated() const { return size() > 0; }
   virtual void check_estimated() const;
   virtual Bool has_std_dev() const { return size() > 1; }
   virtual void check_std_dev() const;
   virtual Bool has_theo_std_dev() const
      { return (has_real() && numTestInstances > 1); }
   virtual void check_theo_std_dev() const;

   virtual void clear() {
      StatData::clear();  realAccuracy = -1;  numTestInstances = 0;
      evalCost = 0; }

   virtual int get_cost() const { return evalCost; }

   virtual void append(const StatData& other) { StatData::append(other); }
   virtual void append(const AccData& other);
   virtual void display(MLCOStream& stream = Mcout, Real trim = 0, 
			int precision = defaultPrecision) const;
   virtual void dot_display(MLCOStream& stream = Mcout, Real trim = 0,
			    int precision = defaultPrecision) const;
};

DECLARE_DISPLAY(AccData);


#endif
