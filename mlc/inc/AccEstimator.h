// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _AccEstimator_h
#define _AccEstimator_h 1

#include <LogOptions.h>
#include <MRandom.h>
#include <AccData.h>

extern const MString defaultDataExt;
extern const MString defaultNamesExt;
extern const MString defaultTestExt;

class BaseInducer;
class InstanceList;
class CatTestResult;


class AccEstimator {
   NO_COPY_CTOR(AccEstimator);
   LOG_OPTIONS;
   RAND_OPTIONS;
protected: 
   MString dumpStem;    // Don't dump if ""
   AccData *accData;  

   // dumpSuffix is the file stem suffix (before .{names,data,test}
   // to add to dumpStem.  It will not dump if dumpStem is ""
   virtual void dump_data(const MString& dumpSuffix,
			  const MString& descr,
			  const InstanceList& trainList,
			  const InstanceList& testList);

   // NULL value for localAccData means don't update accuracy.
   virtual void update_acc_data(int numCorrect, int numIncorrect,
				AccData *localAccData);

   // show how stratified this particular train/test pair are
   void show_stratification(const InstanceList& trainPart,
			    const InstanceList& testPart) const;
   
   // train_and_test should call dump_data
   // This is protected so we can test outside programs like C4.5
   virtual Real train_and_test(BaseInducer& inducer,
			       InstanceList* trainList,
  			       const InstanceList& testList,
			       const MString& dumpSuffix,
			       AccData *localAccData);

public:
   AccEstimator();
   virtual ~AccEstimator();
   virtual MString description() const = 0;
   virtual Real estimate_accuracy(BaseInducer& inducer,
				  InstanceList& dataList) = 0;

   // estimate accuracy from a series of files
   virtual Real estimate_accuracy(BaseInducer& inducer,
				  const MString& fileStem) = 0;
   // Estimate accuracy from a fileStem with assumed suffixes
   //    -#.{names,data,test} where # denotes a sequence number.
   virtual Real estimate_file_accuracy(BaseInducer& inducer, int numFiles,
				  const MString& fileStem,
				  AccData *localAccData);
   // Estimate one file from a fileStem
   virtual Real single_file_accuracy(BaseInducer& inducer,
				     const MString& fileStem,
				     AccData *localAccData);

   virtual void check_acc_data() const;
   virtual const AccData& get_acc_data() const;
   
   virtual void display_acc_data(MLCOStream& stream = Mcout,
				 Real trim = 0,
			 int precision = AccData::defaultPrecision) const;

   virtual Real accuracy(Real trim = 0) const;
   virtual Real accuracy_std_dev(Real trim = 0) const;
   
   virtual void dump_files(InstanceList& dataList, const MString& fileStem);

   virtual void display(MLCOStream& stream = Mcout, Bool descrip = FALSE,
			Real trim = 0.0, 
			int precision = AccData::defaultPrecision) const;
};

DECLARE_DISPLAY(AccEstimator);

#endif _AccEstimator_h










