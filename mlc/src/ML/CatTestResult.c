// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Provide summaries of running categorizers on test data.
                 This includes the option of loading the test data
                   from a file (or giving an existing bag), running
                   the categorizer on all instances, and storing the results.
                 Information can then be extracted quickly.
  Assumptions  : The training set and test set (if given as opposed to
                   loading it here) must not be deallocated as long
                   as calls to this class are being made, because pointers
                   are kept to those structures.
  Comments     : 
  Complexity   : O(n1 n2) for construction, where n1 is the size of
                    the training-set bag and n2 is the size of the
                    test set.
                 All display routines take time proportional to the
                    number of displayed numbers.
  Enhancements : Implement methods with NOT_IMPLEMENTED.
  History      : Yeogirl Yun
                   Implemented NOT_IMPLEMENTED parts.              12/27/94
		   Strengthened display_confusion_matrix()
                 Robert Allen                                      12/10/94
                   Add generalized vs memorized accuracy
                 Richard Long                                      10/01/93
                   Initial revision (.c)
                 Ronny Kohavi                                       9/13/93
                   Initial revision (.h)
***************************************************************************/

#include <basics.h>
#include <CatTestResult.h>
#include <InstList.h>
#include <AugCategory.h>
#include <TableCat.h>
#include <math.h>
#include <GetOption.h>

RCSID("MLC++, $RCSfile: CatTestResult.c,v $ $Revision: 1.52 $")

const MString defaultTestExt = ".test";

const int ACC_PRECISION = 2;


/***************************************************************************
  Description : Constructors.
  Comments    :
***************************************************************************/
CatOneTestResult::CatOneTestResult()
{
   instance = NULL;
   augCat = NULL;
   inTrainBag = FALSE;
}

CatCounters::CatCounters()
{
   numTestSet = 0;
   numCorrect = 0;
   numIncorrect = 0;
}


/***************************************************************************
  Description : Reads the data for the list and returns the number of
                   instances. 
  Comments    : This is necessary because the list constructor takes
                   only the schema and creates an empty list, but we also
		   need to dimension the arrays based on the number of
		   instances in the list during the initialization part of
		   the CatTestResult constructor.
***************************************************************************/
static int read_data_return_size(const InstanceList& bag,
				 const MString& dataFileName)
{
   ((InstanceList&)bag).read_data(dataFileName);
   return bag.num_instances();
}


/***************************************************************************
  Description : 
  Comments    : Protected.
***************************************************************************/
void CatTestResult::initialize(const Categorizer& cat)
{
   DBG(trainBag->get_schema().equal(testBag->get_schema(), TRUE));
   int i = 0;
   numCorrect = 0;
   for (Pix bagPix = testBag->first(); bagPix; testBag->next(bagPix), i++) {
      InstanceRC instance = testBag->get_instance(bagPix);
      LOG(2, "Instance: " << instance);
      const Category correctCat =
	 instance.label_info().get_nominal_val(instance.get_label());
      results[i].correctCat = correctCat;
      results[i].instance = new InstanceRC(instance);
      // Change label to UNKNOWN, so that categorizers don't cheat.
      DBG(AttrValue_ av;
	  instance.label_info().set_unknown(av);
	  instance.set_label(av));
      const AugCategory& predCat = cat.categorize(instance);
      confusionMatrix(correctCat, predCat)++;
      results[i].augCat = new AugCategory(predCat);

      catDistrib[predCat].numTestSet++;

      LOG(2, "Correct label: " 
	  << instance.get_schema().category_to_label_string(correctCat)
	  << " Predicted label: " << predCat.description() << ". ");

      if (predCat == correctCat) {
         // checks that strings match too
         DBG(if (predCat.description() != 
	       instance.get_schema().category_to_label_string(correctCat))
               err << "CatTestResult::initialize: labels match but not in "
                      " name. Categorizer=" << predCat.description()
                   << ". Instance="
                   << instance.get_schema().
	                   category_to_label_string(correctCat)<< fatal_error);
	 numCorrect++;
	 catDistrib[predCat].numCorrect++;
	 LOG(2, "Correct. ");
      } else {
	 catDistrib[predCat].numIncorrect++;
	 LOG(2, "Incorrect. ");
      }
      LOG(2, "No correct: " << numCorrect << '/' << i + 1 << endl);
      
   }
}


/***************************************************************************
  Description : Uses TableCategorizer as an interface to hash table to do quick
                  lookup on whether a test instance occurs in the training
		  set.  Only called when inTrainBag data is needed.
		Initializes class variable numOffTrain to number of test
		  cases found in training set.
  Comments    : Protected.  Constness is logical, not actual.
***************************************************************************/
void CatTestResult::initializeTrainTable() const
{
   int numTestInTrain = 0;
   TableCategorizer trainTable(*trainBag,
			       UNKNOWN_CATEGORY_VAL,
			       "Training Set Lookup Table");

   int i = 0;
   for (Pix bagPix = testBag->first(); bagPix; testBag->next(bagPix), i++) {
      InstanceRC instance = testBag->get_instance(bagPix);
      if (trainTable.categorize(instance) != UNKNOWN_CATEGORY_VAL) {
	 ((CatTestResult *)this)->results[i].inTrainBag = TRUE;
                                                   // constructor sets to FALSE
	 numTestInTrain++;
      }
   }
   ((CatTestResult *)this)->numOnTrain = numTestInTrain;
   ((CatTestResult *)this)->inTrainingSetInitialized = TRUE;
}


/***************************************************************************
  Description : NOT IMPLEMENTED.  Should check that results are consistent.
  Comments    :
***************************************************************************/
void CatTestResult::OK(int level) const
{
   if (level <= 0) {
      for (int i = catDistrib.low(); i <= catDistrib.high(); i++)
	 ASSERT(catDistrib[i].numCorrect + catDistrib[i].numIncorrect ==
		catDistrib[i].numTestSet);

      int sum = 0;
      for (i = results.low(); i <= results.high(); i++)
	 if (*results[i].augCat == results[i].correctCat)
	    sum++;
      ASSERT(sum == numCorrect);

      int numInstances = 0;
      for (i = confusionMatrix.start_col();
	   i <= confusionMatrix.high_col(); i++)
	 for (int j = confusionMatrix.start_row();
	      j <= confusionMatrix.high_row(); j++)
	    numInstances += confusionMatrix(j,i);

      ASSERT(numInstances == testBag->num_instances());
   }
}


/***************************************************************************
  Description : The constructor initializes the CatTestResult with the given
                  training bag and the given test bag or the test bag
		  whose instance info matches the training bag and
		  whose data is in the given file.
		The given categorizer is then used to categorize all
		  of the instances in the test set with appropriate
		  information tabulated.
  Comments    :
***************************************************************************/
CatTestResult::CatTestResult(const Categorizer& cat,
			     const InstanceBag& trainingBag,
			     const MString& testFile,
			     const MString& testExtension)
: logOptions("CTR"),
  trainBag(&trainingBag),
  testBag(new InstanceList(trainingBag.get_schema())),
  results(read_data_return_size(testBag->cast_to_instance_list(),
				testFile + testExtension)),
  catDistrib(UNKNOWN_CATEGORY_VAL, testBag->num_categories() + 1),
  confusionMatrix(UNKNOWN_CATEGORY_VAL, UNKNOWN_CATEGORY_VAL,
		  testBag->num_categories() + 1,
		  testBag->num_categories() + 1, 0)
{
   ownsTestBag = TRUE;
   initialize(cat);
   inTrainingSetInitialized = FALSE;
}

CatTestResult::CatTestResult(const Categorizer& cat,
			     const InstanceBag& trainingBag,
			     const InstanceBag& testingBag)
: logOptions("CTR"),
  trainBag(&trainingBag), 
  testBag(&testingBag),
  results(testBag->num_instances()),
  catDistrib(UNKNOWN_CATEGORY_VAL, testBag->num_categories() + 1),
  confusionMatrix(UNKNOWN_CATEGORY_VAL, UNKNOWN_CATEGORY_VAL,
		  testingBag.num_categories()+1, testingBag.num_categories()+1,
		  0)
{
   ownsTestBag = FALSE;
   initialize(cat);
   inTrainingSetInitialized = FALSE;
}


/***************************************************************************
  Description : Destructor.
  Comments    :
***************************************************************************/
CatTestResult::~CatTestResult()
{
   DBGSLOW(OK(0));
   for (int i = results.low(); i <= results.high(); i++) {
      delete results[i].augCat;
      delete results[i].instance;
   }
   if (ownsTestBag)
      // Casting away constness because we know that we allocated this space
      delete (InstanceBag*)testBag;
}


/***************************************************************************
  Description : Return the number of instances in the test bag.
  Comments    :
***************************************************************************/
int CatTestResult::num_test_instances() const
{
   return testBag->num_instances();
}


/***************************************************************************
  Description : Return the number of instances in the training bag.
  Comments    :
***************************************************************************/
int CatTestResult::num_train_instances() const
{
   return trainBag->num_instances();
}


/***************************************************************************
  Description : Number of test instances appearing in the training data.
                  Initializes flag for each test instance if not already
		  done.
  Comments    :
***************************************************************************/
int CatTestResult::num_on_train() const
{
   if ( ! inTrainingSetInitialized ) 
      initializeTrainTable();
   return numOnTrain;
}   


/***************************************************************************
  Description : Number of training instances not appearing in the test data.
  Comments    :
***************************************************************************/
int CatTestResult::num_off_train() const
{
   return num_test_instances() - num_on_train();
}


/***************************************************************************
  Description : Return the number of instances in the test bag that were
                  correctly/incorrectly categorized.
  Comments    :
***************************************************************************/
int CatTestResult::num_correct() const
{
   return numCorrect;
}

int CatTestResult::num_incorrect() const
{
   return num_test_instances() - num_correct();
}


/***************************************************************************
  Description : Returns ratio number of test instances correctly categorized
                  / number of test instances.  Test instance set defaults to
		  all test instance.  AccuracyType argument can be used to
		  partition test cases into those occuring in the training
		  set or not.
  Comments    :
***************************************************************************/
Real CatTestResult::accuracy(const CatTestResult::AccuracyType at) const
{
   if (testBag->no_instances())
      err << "CatTestResult::accuracy: No test instances.  This causes "
	     "division by 0" << fatal_error;

   switch (at) { 
      case CatTestResult::Normal:
	 return Real(num_correct()) / num_test_instances();

      case CatTestResult::Generalized: {
	 if (num_off_train() == 0)
	    err << "CatTestResult::accuracy(Generalized): All test instances "
	       "are also in training set.  This causes division by 0"
		<< fatal_error;

	 int numOffTrainCorrect = 0;
	 for (int i = results.low(); i <= results.high(); i++)
	    if ( (!results[i].inTrainBag) &&
		 ( *results[i].augCat == results[i].correctCat ) )
	       numOffTrainCorrect++;
	 return Real(numOffTrainCorrect) / num_off_train();
      }

      case CatTestResult::Memorized: {
	 if (num_on_train() == 0)
	    err << "CatTestResult::accuracy(Memorized): No test instances "
	       "are in the training set.  This causes division by 0"
	       << fatal_error;

	 int numOnTrainCorrect = 0;
	 for (int i = results.low(); i <= results.high(); i++)
	    if ( (results[i].inTrainBag) &&
		 ( *results[i].augCat == results[i].correctCat ) )
	       numOnTrainCorrect++;
	 return Real(numOnTrainCorrect) / num_on_train();
      }
      default:
	 err << "CatTestResult::accuracy unexpected accuracy type"
             << at << fatal_error;
	 return 1.0;
   } // end switch
}


/***************************************************************************
  Description : Returns the indicated LabelledInstance in the test set.
  Comments    :
***************************************************************************/
InstanceRC CatTestResult::get_instance(int num) const
{
   return *results[num].instance;
}

/***************************************************************************
  Description : Returns the AugCategory for the indicated instance in
                   the test set. The category value is the predicted value.
                num argument starts from 1.
  Comments    :
***************************************************************************/
const AugCategory& CatTestResult::label(int num) const
{
   if (num < 1 || num > testBag->num_instances())
      err << "CatTestResult::label: illegal num value : " << num << " ("
	 "should be in [1," << testBag->num_instances() << "])" << fatal_error;
	 
   int i = 1;
   for (Pix pix = testBag->first(); pix; testBag->next(pix), i++)
      if (i == num)
	 break;
   const InstanceRC& instance = testBag->get_instance(pix);

   for (i = results.low(); i <= results.high(); i++)
      if (*results[i].instance == instance)
	 break;
   ASSERT(i <= results.high());
   return *results[i].augCat;
}


/***************************************************************************
  Description : Returns the predicated AugCategory for the indicated
                   instance in the test set.
  Comments    :
***************************************************************************/
const AugCategory& CatTestResult::predicted_label(int num) const
{
   return *results[num].augCat;
}


/***************************************************************************
  Description : display_* show the instance and both labels (except for
                  display_correct_instances() which shows only one label).
		Instances which were in the training set say "(In TS)" on
		  the display line.
  Comments    :
***************************************************************************/
void CatTestResult::display_all_instances(MLCOStream& stream) const
{
   stream << "Displaying all instances ... " << endl;
   for (int i = results.low(); i <= results.high(); i++) 
	 stream << *results[i].instance;
}

void CatTestResult::display_incorrect_instances(MLCOStream& stream) const
{
   stream << "Incorrect classifications ... " << endl; 
   for (int i = results.low(); i <= results.high(); i++) 
      if (*results[i].augCat != results[i].correctCat)
	 stream << *results[i].instance;
}

void CatTestResult::display_correct_instances(MLCOStream& stream) const
{
   stream << "Displaying correct classifications ... " << endl;
   for (int i = results.low(); i <= results.high(); i++) 
      if (*results[i].augCat == results[i].correctCat)
	 stream << *results[i].instance;
}


/***************************************************************************
  Description : Displays the distribution of instances for each class.
  Comments    :
***************************************************************************/
void CatTestResult::display_category_distrib(MLCOStream& stream) const
{
   stream << "\nDisplaying category distribution ... " << endl;
   int i = 0;
   for (int col = confusionMatrix.start_col() + 1;
	col <= confusionMatrix.high_col(); col++, i++) {
      int num = 0;
      for (int row = confusionMatrix.start_row() + 1;
	   row <= confusionMatrix.high_row(); row++) 
	 num += confusionMatrix(row,col);
      AttrValue_ val;
      testBag->nominal_label_info().
	 set_nominal_val(val, FIRST_CATEGORY_VAL + i);
      stream << "Class "  << testBag->nominal_label_info().
	 attrValue_to_string(val) << " : " << num <<  endl;
   }
}


/***************************************************************************
  Description : The confusion matrix displays for row i column j, the number
                  of instances classified as j that should have been
		  classified as i.
  Comments    :
***************************************************************************/
void CatTestResult::display_confusion_matrix(MLCOStream& stream) const
{
   stream << "\nDisplaying confusion matrix... " << endl;
   int row, col;
   const char ach = 'a';
   int i;

   // figure out whether unknown classes are used.
   // for example, some test instance was classified as 'unknown' or
   // there are some test instances that is of 'unknown' class.
   Bool hasUnknowns = FALSE;
   for (i = confusionMatrix.start_row(); i <= confusionMatrix.high_row();
					 i++)
      if (confusionMatrix(i, confusionMatrix.start_col()) > 0) {
	 hasUnknowns = TRUE;
	 break;
      }

   if (!hasUnknowns)
      for (i = confusionMatrix.start_col() + 1;
	   i <= confusionMatrix.high_col(); i++)
	 if (confusionMatrix(confusionMatrix.start_col(), i) > 0) {
	    hasUnknowns = TRUE;
	    break;
	 }

   // set depending variables.
   int step, start;
   if (hasUnknowns) {
      step = 0;
      start = -1;
   }
   else {
      step = 1;
      start = 0;
   }

   for (i = start, row = confusionMatrix.start_row() + step;
	row <= confusionMatrix.high_row(); row++, i++)
      if (i == -1)
	 stream << " (?) ";
      else
	 stream << " (" << (char)(ach+i) << ") ";
   stream << "   <-- classified as " << endl;         
   for (row = confusionMatrix.start_row() + step;
	row <= confusionMatrix.high_row(); row++)
      stream << "---- ";
   stream << endl;
   
   for (i = start, row = confusionMatrix.start_row() + step;
	row <= confusionMatrix.high_row(); i++, row++) {
      for (col = confusionMatrix.start_col() + step;
	   col <= confusionMatrix.high_row(); col++)
	 stream << setw(4) << confusionMatrix(row, col) << " ";
      AttrValue_ val;
      testBag->nominal_label_info().
	 set_nominal_val(val, FIRST_CATEGORY_VAL + i);
      if (i == -1)
	 stream << "   (?): unknown class " << endl;
      else 
	 stream << "   (" << (char)(ach + i) << "): class "
		<< testBag->nominal_label_info().attrValue_to_string(val)
		<< endl;
   }
}


/***************************************************************************
  Description : Gives all available statistics (not displays)
  Comments    :
***************************************************************************/
void CatTestResult::display(MLCOStream& stream) const
{
   static Bool dispConfusionMatrix = 
      get_option_bool("DISP_CONFUSION_MAT",
      FALSE, "Display confusion matrix when displaying results on test",TRUE);

   static Bool dispMisclassifications = 
      get_option_bool("DISP_MISCLASS",
      FALSE, "Display misclassified instances",TRUE);


   stream << "Number of training instances: " << num_train_instances()
	  << endl;
   stream << "Number of test instances: " << num_test_instances()
          << ".  Unseen: " << num_off_train()
          << ",  seen " << num_on_train() << '.' << endl;
   DBG(ASSERT(num_off_train() + num_on_train() == num_test_instances()));
   Real confLow, confHigh;
   confidence(confLow, confHigh, accuracy(), num_test_instances());
   stream << "Number correct: " << num_correct()
          << ".  Number incorrect: " << num_incorrect() << endl;
   stream << "Generalization accuracy: ";
   if (num_off_train() > 0)
      stream << MString(accuracy(Generalized)*100,ACC_PRECISION) << '%';
   else
      stream << "unknown";
   stream << ".  Memorization accuracy: ";
   if (num_on_train() > 0)
       stream  << MString(accuracy(Memorized)*100,ACC_PRECISION) << '%' << endl;
   else
      stream << "unknown" << endl;      
   MString stdDev;
   if (num_test_instances() > 1)
      stdDev = MString(theoretical_std_dev(accuracy(), 
			      num_test_instances())*100,ACC_PRECISION) + "%";
   else
      stdDev = "Undefined";
   stream << "Accuracy: " << MString(accuracy()*100,ACC_PRECISION)
	  << "% +- " << stdDev
	  << " [" << MString(confLow*100, ACC_PRECISION) << "% - "
          << MString(confHigh*100, ACC_PRECISION) << "%]" << endl;

   if (dispConfusionMatrix)
      display_confusion_matrix();      

   if (dispMisclassifications)
      display_incorrect_instances();
}

DEF_DISPLAY(CatTestResult)


/***************************************************************************
  Description : Dumps everything (display + display_all_instances()).
  Comments    :
***************************************************************************/
void CatTestResult::display_all(MLCOStream& stream) const
{
   display(stream);
   display_incorrect_instances();
   display_correct_instances();
   display_confusion_matrix();
   display_category_distrib();
}

/***************************************************************************
  Description : Compute the estimated standard deviation according to
                  the binomial model, which assumes every test instance is
                  a Bernoulli trial, thus std-dev=sqrt(acc*(1-acc)/(n-1))
  Comments    : Static function
***************************************************************************/
Real CatTestResult::theoretical_std_dev(Real accuracy, int n)
{
   if (n <= 1)
      err << "CatTestResult: std-dev of less than two instances is undefined"
	  << fatal_error;
   
   return sqrt(accuracy*(1-accuracy)/(n-1));
}

/***************************************************************************
  Description : Compute the confidence interval according to the binomial
                  model.  Source is Devijver and Kittler.
  Comments    : Static function
                Reference (return) parameters appear early in the list
		  because we want to have a default value for z.
***************************************************************************/
void CatTestResult::confidence(Real& confLow, Real& confHigh,
			       Real accuracy, int n, Real z)
{
   Real z2 = z*z;
   Real sqrtTerm = z*sqrt(4*n*accuracy+z2 - 4*n*accuracy*accuracy);
   Real numer = 2*n*accuracy + z2;
   Real denom = 2*(n+z2);

   confLow = (numer - sqrtTerm)/denom;
   confHigh = (numer + sqrtTerm)/denom;
}


