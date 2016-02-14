// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _CatTestResult_h
#define _CatTestResult_h 1

#include <Array.h>
#include <Array2.h>
#include <AugCategory.h>
#include <LogOptions.h>

class AugCategory;
class InstanceBag;
class InstanceRC;
class Categorizer;

extern const MString defaultTestExt;

// CatOneTestResult and CatCounters should probably be nested classes,
//   but the compiler complains that templates on nested classes is
//   not implemented.  
struct CatOneTestResult {
   InstanceRC* instance;       // Pointer because InstanceRC
			       // constructor requires an argument, so then
			       // this could not be in an Array.
   AugCategory* augCat;        // Pointer in case AugCategory is subclassed.
   Category correctCat;        // What we should have guessed. 
   Bool inTrainBag;            // TRUE iff instance appeared in training bag.

   CatOneTestResult();
};

struct CatCounters {
   int numTestSet;      // Number of instances in test-set in this category.
   int numCorrect;      // Number of correctly categorized.
   int numIncorrect;

   CatCounters();
};


// if the InstanceBag pointers are changed to references,
// there is a problem in the initialization that leads to run-time errors
class CatTestResult {
 public:
   // enumerated type used by accuracy() to indicate set of test instances to
   //   report on.  Normal = all, Memorized = test instance in training set,
   //   Generalized = test instance not in training set.
   enum AccuracyType { Normal, Generalized, Memorized };
private:  
   NO_COPY_CTOR(CatTestResult);
   LOG_OPTIONS;

   const InstanceBag* trainBag;
   const InstanceBag* testBag;
   int numCorrect;
   Array<CatOneTestResult> results;
   Array<CatCounters> catDistrib;
   Array2<int> confusionMatrix;
   Bool ownsTestBag;
   Bool inTrainingSetInitialized;
   int numOnTrain;

protected:
   void initialize(const Categorizer& cat);
   void initializeTrainTable() const;
   
public:
   void OK(int level = 1) const;
   CatTestResult(const Categorizer& cat,
                 const InstanceBag& trainBag,
                 const MString& testFile,
	         const MString& testExtension = defaultTestExt);
   CatTestResult(const Categorizer& cat,
                 const InstanceBag& trainBag,
                 const InstanceBag& testBag);
   virtual ~CatTestResult();
   const InstanceBag& get_training_bag() const {return *trainBag;}
   const InstanceBag& get_testing_bag() const {return *testBag;}
   const Array<CatOneTestResult>& get_results() const {return results;}
   int num_test_instances() const;
   int num_train_instances() const;
   int num_on_train() const;	// number of training instances appearing
				// in the test data.
   int num_off_train() const;
   int num_correct() const;
   int num_incorrect() const;
   Real accuracy(const CatTestResult::AccuracyType at =
		 CatTestResult::Normal) const;
   virtual InstanceRC get_instance(int num) const;
   virtual const AugCategory& label(int num) const;
   virtual const AugCategory& predicted_label(int num) const;
   // display_* show the instance and both labels (except for
   //   display_correct_instances() which shows only one label).
   // Instances which were in the training set say "(In TS)" on
   //   the display line.
   virtual void display_all_instances(MLCOStream& stream = Mcout) const;
   virtual void display_incorrect_instances(MLCOStream& stream = Mcout) const;
   virtual void display_correct_instances(MLCOStream& stream = Mcout) const;
   // category distribution displays an array where each cell i
   //   consists of: (1) number of test instances in class i,
   //   (2) number of test instances correctly predicted as class i,
   //   (3) number of test instances incorrectly predicted as class i.
   virtual void display_category_distrib(MLCOStream& stream = Mcout) const;
   // The confusion matrix displays for row i column j, the number
   //   of instances classified as j that should have been classified
   //   as i.
   virtual void display_confusion_matrix(MLCOStream& stream = Mcout) const;
   // display gives statistics (not displays)
   virtual void display(MLCOStream& stream = Mcout) const;
   // display_all dumps everything (display + display_all_instances).
   virtual void display_all(MLCOStream& stream = Mcout) const;
   // theoretical_std_dev gives the estimated standard deviation according to
   //   the binomial model, which assumes every test instance is a Bernoulli
   //   trial, thus std-dev=sqrt(acc*(1-acc)/n)
   // we use a static function here so that classes which don't use
   // a CatTestResult (like BaseInducer can still access this function.
   static Real theoretical_std_dev(Real acc, int n);
   static void confidence(Real& confLow, Real& confHigh,
			  Real accuracy, int n, Real z = confidenceIntervalZ);
};

DECLARE_DISPLAY(CatTestResult);

#endif

