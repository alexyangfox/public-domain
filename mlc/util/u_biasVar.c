// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a utility for MLC++
// It is not part of the MLC++ library itself, but instead uses the
//   library to provide a useful function.

/***************************************************************************
  Description  : Compute the 0/1-loss bias-variance.
  Usage        : Define the DATAFILE and INDUCER.  The rest of the options
                   are nuisance.  The DATAFILE should be the file containing
		   all known instances (usually with a .all suffix).
                 The DATAFILE is split into train/test according to
		   the NUM_TEST_INT/TEST_FRACT options.
                 The training set is then subsampled TRAIN_TIMES with
                    INTERNAL_TRAIN_FRACT of the instances sampled
		    without replacement.  The inducer is trained
		    and tested on the test instances.  This gives
		    us all the desired probabilities conditioned on q.
  Comments     : There's a slight deviation from standard as far as variable
                    names.  We use underscores to emphasize subscripts
		    and conditioning variables in the names.
  Enhancements :
  History      : Ronny Kohavi                                       11/18/95
***************************************************************************/

/***************************************************************************

Throughout we have
   f  - target function
   m  - number of instances in training set
   q  - test instance.
   D* - original dataset
   D' - test set
   D_i- internal training sets.
   
***************************************************************************/   



#include <basics.h>
#include <Inducer.h>
#include <MRandom.h>
#include <env_inducer.h>
#include <CatTestResult.h>
#include <CtrInstList.h>
#include <math.h>
#include <GetOption.h>
#include <FileNames.h>
#include <StatData.h>

RCSID("MLC++, $RCSfile: u_biasVar.c,v $ $Revision: 1.0 $")


const int ACC_PRECISION = 4;
#define ROUND(x) MString(x, ACC_PRECISION)


const char *splitSeedHelp =
   "The seed for the initial split into train/test.";   
const int DEFAULT_SPLIT_SEED = 7258789;

const char *replicationsSeedHelp =
   "The seed for the generation of the internal training sets.";   
const int DEFAULT_REPLICATIONS_SEED = 9333126;

const char *testFractHelp =
   "The fraction of the instances to be reserved for the test set.";
const Real DEFAULT_TEST_FRACT = 0.6667;

const char *trainTimesHelp =
   "The number of times to generate samples and train the inducer";
const int DEFAULT_TRAIN_TIMES = 10;

const char *repeatTimesHelp =
   "The number of times to repeat generating internal samples";
const int DEFAULT_REPEAT_TIMES = 1;

const char *internalTrainFractionHelp =
   "The fraction of the instances to be used for internal training sets"
   "  (after the test instances have been removed).";
const Real DEFAULT_INTERNAL_TRAIN_FRACTION = 0.5;

const char *numTestInstHelp =
   "The number of test instances to hold out.  If 0, the fraction "
   " will be requested.";
const int DEFAULT_NUM_TEST_INST = 0;

const char *covarDecompHelp =
   "There are two decompositions possible.  The standard one does not"
   " use the covariance term.  The second decomposition, does.";
const Bool DEFAULT_COVAR = FALSE;

/***************************************************************************
  Description : Get the inducer
  Comments    :
***************************************************************************/

static Inducer *get_inducer()
{
   BaseInducer *baseInducer = env_inducer();
   if (!baseInducer->can_cast_to_inducer())
      err << "Only full inducers are supported, not base inducers" 
	  << fatal_error;

   int indLogLevel = get_option_int("INDUCER_LOGLEVEL", globalLogLevel - 2);
   baseInducer->set_log_level(indLogLevel);

   return &baseInducer->cast_to_inducer();
}

/***************************************************************************
  Description : Get the training and test files
  Comments    :
***************************************************************************/

static void get_lists(CtrInstanceList*& train, InstanceList*& test)
{
   FileNames files;

   MString dataFile = files.data_file();
   MString namesFile = files.names_file();
   MString rootName = MString(dataFile, dataFile.index("."));
   if (MString((const char *)dataFile + rootName.length()) != ".all")
      Mcerr << "Warning: data file should contain all known instances."
	    << endl << dataFile << " does not have a '.all' suffix." << endl;

   InstanceList allData("", namesFile, dataFile);
   int numAllInstances = allData.num_instances();

   int numTestInstances = get_option_int("NUM_TEST_INST", 
			     DEFAULT_NUM_TEST_INST, numTestInstHelp, TRUE);

   if (numTestInstances == 0) {
      Real testFract = get_option_real_range("TEST_FRACT", DEFAULT_TEST_FRACT,
					     0.0, 1.0, testFractHelp, TRUE);
      numTestInstances = numAllInstances*testFract + 0.5;
      if (numTestInstances == 0)
	 err << "Cannot test on zero instances" << fatal_error;
   }

   int numTrainInstances;
   if (numTestInstances > 0) 
      numTrainInstances = numAllInstances - numTestInstances;
   else {
      numTrainInstances = -numTestInstances;
      numTestInstances = numAllInstances - numTrainInstances;
      GLOBLOG(1, "Sampling from training set of size " 
	      << numTrainInstances << " instances." << endl);
      ASSERT(numTrainInstances + numTestInstances == numAllInstances);
   }

   if (numAllInstances - numTestInstances < 1)
      err << "Training is meaningless with " <<
	 numAllInstances - numTestInstances << " instances" << fatal_error;
   
   unsigned int seed = 
      get_option_int("SPLIT_SEED", DEFAULT_SPLIT_SEED, splitSeedHelp, TRUE);
   MRandom splitRandom(seed);

   train = new CtrInstanceList(allData.get_schema());
   test = &allData.independent_sample(numTestInstances, train, &splitRandom)->
           cast_to_instance_list();
   ASSERT(test->num_instances() == numTestInstances);
   ASSERT(train->num_instances() == numTrainInstances);
   Mcout << "File: " << files.data_file()
           << ".  Training set instances: " << train->num_instances()
	   << ". Testing set instances: " << test->num_instances() << endl;
}   

/***************************************************************************
  Description : Compute the probability of each class in bag.
  Comments    : Note that the probabilities are 0/1 now.
***************************************************************************/

static Array2<Real>* compute_prob_yf_q(const InstanceBag& test)
{
   int numInstances = test.num_instances();

   Array2<Real>* prob_yf_q = 
      new Array2<Real>(numInstances, test.num_categories(), 0.0);

   int inst = 0;
   for (Pix pix = test.first(); pix; test.next(pix), inst++) {
      InstanceRC instance = test.get_instance(pix);
      const Category cat =
	 instance.label_info().get_nominal_val(instance.get_label());
      (*prob_yf_q)(inst, cat) = 1;
   }

   ASSERT(inst == numInstances);
   return prob_yf_q;
}
      

/***************************************************************************
  Description : Train inducer multiple times on samples and test on
                  the single test set. 
                Return prob(Y_H|q,f,m) already marginalized over d.
	        output prob(err|q,f,m).
  Comments    :
***************************************************************************/

static Array2<Real>*
multi_train(Inducer& inducer, const CtrInstanceBag& train,
	    const InstanceBag& test, Real& acc,
            Array<Real>*& err_q, int trainTimes, Real internalTrainFract,
	    MRandom& replicationRandom)
{
   int numTrainInst = train.num_instances();
   int numTestInst = test.num_instances();
   int numCategories = train.num_categories();
   ASSERT(numCategories == test.num_categories());
   
   Array2<Real>* prob_yh_q = new Array2<Real>(numTestInst, numCategories,
					       0.0);
   err_q = new Array<Real>(0, test.num_instances(), 0);

   int numInternTrainInst = numTrainInst * internalTrainFract + 0.5;

   acc = 0;
   for (int trainNum = 0; trainNum < trainTimes; trainNum++) {
      if (numInternTrainInst < 1)
	 err << "Internal training is meaningless with " <<
	    numInternTrainInst << " instances" << fatal_error;

      InstanceBag *internalTrain = train.independent_sample(numInternTrainInst,
						    &replicationRandom);
      ASSERT(internalTrain->num_instances() == numInternTrainInst);
      inducer.assign_data(internalTrain);
      inducer.train();
      CatTestResult result(inducer.get_categorizer(),
			   inducer.instance_bag(), test);
      GLOBLOG(2, "Results from sample " << trainNum << " are:" << endl
             << result << endl);
      acc += result.accuracy() / trainTimes;
      const Array<CatOneTestResult>& res = result.get_results();

      int numTestInst = test.num_instances();
      ASSERT(numTestInst == res.size());
      for (int inst = 0; inst < numTestInst; inst++) {
         (*prob_yh_q)(inst, *res[inst].augCat) += Real(1)/trainTimes;
	 err_q->index(inst) += (*res[inst].augCat != res[inst].correctCat)/
	                     Real(trainTimes);
      }
   }
  return prob_yh_q;
}
   
/***************************************************************************
  Description : Compute the bias-variance without the covariance term.
  Comments    : E(C|f,m) = sum_q P(q) E(C|f,m,q)
                   and
                E(C|f,m,q) = 1-sum_y P(Y_F = y, Y_H = y | f,m,q) (def)
                Everything below is conditioned on f,m,q
   		   = sum_y -P(Y_F=y, Y_H=y) + sum_y P(Y_H=y)P(Y_F=y) +
                     sum_y [-P(Y_H=y)P(Y_f=y) + .5 P^2(Y_F=y) + .5 P^2(Y_H=y)]+
                     .5 - .5 sum_y P^2(Y_F=y) +
		     .5 - .5 sum_y P^2(Y_H=y)

                   = sum_y [P(Y_H=y)P(Y_F=y) - P(Y_F=y,Y_H=y) +    "-Cov"
                     .5 sum_y [P(Y_H=y) - P(Y_F=y)]^2  +           "Bias^2"
                     .5 (1-sum_y P^2(Y_F=y) +                      "sigma^2"
                     .5 (1-sum_y P^2(Y_H=y)                        "Variance"

                  P(Y_F, Y_H | f,m,q)
		  = P(Y_F|f,m,q)*P(Y_H | Y_F,f,m,q)
                  = P(Y_F|f,m,q)*P(Y_H | f,m,q).
		  so the covariance term cancels.

                Note that P(Y_H=h|f,m,q,dataset) is 0/1 right now,
                but P(Y_H|f,m,q) is not 0/1 because we marginalize over d.
***************************************************************************/

static void bias_var(const Array2<Real>& prob_yh_q,
     const Array2<Real>& prob_yf_q, Real expectedErr, int trainTimes,
     StatData& sigmaData,  StatData& biasData,  StatData& varData,
     StatData& eSigmaData, StatData& eBiasData, StatData& eVarData)
{
   int numInstances = prob_yh_q.num_rows();
   ASSERT(numInstances == prob_yf_q.num_rows());
   
   int numCat = prob_yh_q.num_cols();
   ASSERT(numCat == prob_yf_q.num_cols());

   // sigma^2 = 1-sum_y P^2(Y_F)
   // bias^2  = sum_y (P(Y_F) - P(Y_H))^2
   // variance= 1-sum_y P^2(Y_H)

   Real sigma = 0;
   Real bias  = 0;
   Real var   = 0;
   Real eBias = 0;
   Real eVar = 0;
   // Outer loop marginalizes over q
   for (int inst = 0; inst < numInstances; inst++) {
      Real qSigma = .5;
      Real qBias  = 0;
      Real qVar   = .5;
      Real qEBias  = 0;
      Real qEVar   = .5;
      for (int cat = 0; cat < numCat; cat++) {
	 Real pyf_q = prob_yf_q(inst, cat);
	 Real pyh_q = prob_yh_q(inst, cat);
	 ASSERT(pyf_q >= 0); ASSERT(pyh_q >= 0);
	 qSigma -= .5 * pyf_q * pyf_q;
	 qBias  += .5 * (pyf_q - pyh_q) * (pyf_q - pyh_q);
         // pyf_q is fixed and does not depend on the number of internal
	 // replications.  It's therefore accurate.  Pyh_q on the other
	 // hand is estimated and pyh_q^2 will be "too small"
         Real var = pyh_q*(1-pyh_q)/(trainTimes-1);
	 Real b = pyf_q*pyf_q- 2*pyf_q*pyh_q + pyh_q*pyh_q-var;
	 qEBias += .5 * b;
	 qVar   -= .5 * pyh_q * pyh_q;
	 Real v = -var + pyh_q*pyh_q;
	 qEVar -= 0.5 * v;
      }
      Real qSum = qSigma + qBias + qVar;
      Real qESum = qSigma + qEBias + qEVar;
      GLOBLOG(3, "Instance " << MString(inst + 1, 6)
	      <<  ": sigma=" << ROUND(qSigma)  
	      << " bias="    << ROUND(qBias)
	      << " Ebias="   << ROUND(qEBias)
	      << " var="     << ROUND(qVar)
	      << " Evar="    << ROUND(qEVar)
	      << ".  sum="   << ROUND(qSum)
	      << ". Esum="   << ROUND(qESum)
	      << endl);
      sigma += qSigma / numInstances; // 1/numInstances = p(q)
      bias += qBias / numInstances;
      var += qVar / numInstances;
      eBias += qEBias / numInstances;
      eVar += qEVar / numInstances;
   }

   Real sum = sigma + bias + var;
   Real Esum = sigma + eBias + eVar;
   if (fabs(sum - expectedErr) > 0.00001)
      err << "Bad error computation.  Expecting " << expectedErr <<
	     " and seeing " << sum << fatal_error;
   
   Mcout << "Sigma^2 = " << ROUND(sigma)
	   << ".   Bias^2  = " << ROUND(bias)
	   << ".   Variance= " << ROUND(var)
	   << ".   Err  = " << ROUND(sum) << endl;
   Mcout << "Sigma^2 = " << ROUND(sigma)
	   << ".  EBias^2  = " << ROUND(eBias)
	   << ".  EVariance= " << ROUND(eVar)
	   << ".  EErr  = " << ROUND(Esum) << endl << endl;

   sigmaData.insert(sigma);
   biasData.insert(bias);
   varData.insert(var);
   eSigmaData.insert(sigma);
   eBiasData.insert(eBias);
   eVarData.insert(eVar);
}

/***************************************************************************
  Description : Compute the bias-variance with the covariance term.
  Comments    : E(C|f,m) = 1-sum_y P(Y_F = y, Y_H = y | f,m) (def)

                Everything below is conditioned on f,m
   		   = sum_y -P(Y_F=y, Y_H=y) + sum_y P(Y_H=y)P(Y_F=y) +
                     sum_y [-P(Y_H=y)P(Y_f=y) + .5 P^2(Y_F=y) + .5 P^2(Y_H=y)]+
                     .5 - .5 sum_y P^2(Y_F=y) +
		     .5 - .5 sum_y P^2(Y_H=y)

                   = sum_y [P(Y_H=y)P(Y_F=y) - P(Y_F=y,Y_H=y) +    "-Cov"
                     .5 sum_y [P(Y_H=y) - P(Y_F=y)]^2  +           "Bias^2"
                     .5 (1-sum_y P^2(Y_F=y)) +                     "sigma^2"
                     .5 (1-sum_y P^2(Y_H=y))                       "Variance"

                Now, however, the covariance term does not cancel
		because P(Y_F,Y_H) doesn't factor without conditioning on q.
                Intuitively, P(Y_H|Y_F,f,m) != P(Y_F|f,m) because knowing Y_F
		may tell us a lot about which q's generated that Y_F.
		In the extreme case, if f(x) = x (single attribute), we know
                q from Y_F.

                P(y_F) = sum_q P(y_F|q)*p(q)  where q in D'

                P(y_H) = sum_q,j P(y_H|q,D_j) * P(q,D_j)
                       where i ranges from 1 to |D'| and j ranges from 1 to k.

                Note that P(y_H|q_i,D_j) is Boolean for the deterministic
                non-probabilistic classifiers used.

                P(y_F,y_H) = sum_q,j P(y_F,y_H | q,D_j) * p(q,D_j)
                   = sum_q,j P(y_F,y_H | q,D_j) * p (q,D_j)
                   = sum_q,j P(y_F | y_H, q,D_j) * p(Y_h | q, D_j)*p(q,D_j)
                   = sum_q,j p(y_F | q_i) * p(Y_h | q, D_j) * p(q,D_j)

                Cov = sum_q P(y_F|q)*p(q) * sum_q,j P(y_H|q,D_j) * P(q,D_j) -
                      sum_q,j p(y_F | q) * p(Y_h | q, D_j) * p(q,D_j)

**********************************************************************/

static void cover_bias_var(const Array2<Real>& prob_yh_q,
		    const Array2<Real>& prob_yf_q, Real expectedErr)
{
   int numInstances = prob_yh_q.num_rows();
   ASSERT(numInstances == prob_yf_q.num_rows());
   
   int numCat = prob_yh_q.num_cols();
   ASSERT(numCat == prob_yf_q.num_cols());

   // Compute prob(Y_H), prob(Y_F)
   Array<Real> prob_y_f(0, numCat, 0.0);
   Array<Real> prob_y_h(0, numCat, 0.0);
   for (int inst = 0; inst < numInstances; inst++) {
      for (int cat = 0; cat < numCat; cat++) {
	 prob_y_f[cat] += prob_yf_q(inst, cat)/numInstances;
         prob_y_h[cat] += prob_yh_q(inst, cat)/numInstances;
      }
   }

   // Compute prob(y_HF).  Note that although we only need
   // the diagonal, we compute everything to make sure it adds to 1.
   Array2<Real> prob_y_fh(numCat, numCat, 0.0);
   for (inst = 0; inst < numInstances; inst++) {
      for (int catH = 0; catH < numCat; catH++) {
         for (int catF = 0; catF < numCat; catF++) {
            // prob(Y_F, Y_H|f,m,q) is prob(Y_F|f,m,q)*prob(Y_H|f,m,q)
   	    // i.e., it factors given f,m,q but not given f,m
	    prob_y_fh(catH,catF) += prob_yh_q(inst,catH)*
	                            prob_yf_q(inst,catF)/numInstances;
	 }
      }
   }


   GLOBLOG(3, "Prob(Y_F|f,m)=" << prob_y_f << endl);
   GLOBLOG(3, "Prob(Y_H|f,m)=" << prob_y_h << endl);
   GLOBLOG(3, "Prob(Y_FH|f,m)=" << prob_y_fh << endl);

   // Verify that things add up to 1
   Real sumFH = 0;
   for (int i = 0; i < prob_y_fh.size(); i++)
      sumFH += prob_y_fh.index(i);
   ASSERT(fabs(sumFH - 1) < 0.00001);


   Real sigma = 0.5;
   Real bias  = 0;
   Real var   = 0.5;
   Real covar = 0;

   for (int cat = 0; cat < numCat; cat++) {
      sigma -= 0.5*prob_y_f[cat]*prob_y_f[cat];
      bias  += 0.5*(prob_y_h[cat] - prob_y_f[cat])*
	           (prob_y_h[cat] - prob_y_f[cat]);
      var   -= 0.5*prob_y_h[cat]*prob_y_h[cat];
      covar += prob_y_fh(cat,cat) - prob_y_f[cat]*prob_y_h[cat];
   }
   
   Real sum = sigma + bias + var - covar;
   if (fabs(sum - expectedErr) > 0.00001)
      err << "Bad error computation.  Expecting " << expectedErr <<
	     " and seeing " << sum << fatal_error;
   
   Mcout << "Sigma^2 = " << ROUND(sigma)
	   << ".  Bias^2  = " << ROUND(bias)
	   << ".  Variance= " << ROUND(var)
	   << ".  -Covar= " << ROUND(-covar)
	   << ".  Err  = " << ROUND(sum) << endl;
}

/***************************************************************************
  Description : Compute bias/variance/covariance decompositions.
  Comments    :
***************************************************************************/


main()
{
   Inducer* inducer = get_inducer();
   StatData sigmaData;
   StatData biasData;
   StatData varData;
   StatData eSigmaData;
   StatData eBiasData; // e is for expected (corrected bias)
   StatData eVarData;

   CtrInstanceList* train;
   InstanceList* test;
   get_lists(train, test);

   int repeatTimes = get_option_int("REPEAT_TIMES", DEFAULT_REPEAT_TIMES,
				 repeatTimesHelp, TRUE);
   unsigned int seed = 
      get_option_int("REPLICATION_SEED", DEFAULT_REPLICATIONS_SEED, 
      replicationsSeedHelp, TRUE);
   MRandom replicationRandom(seed);
   int trainTimes = get_option_int("TRAIN_TIMES", DEFAULT_TRAIN_TIMES,
				   trainTimesHelp, TRUE);
   if (trainTimes < 2)
      err << "Must train at least twice" << fatal_error;
   
   Real internalTrainFract = get_option_real_range("INTERNAL_TRAIN_FRACT",
         DEFAULT_INTERNAL_TRAIN_FRACTION, 0.0, 1.0,
	 internalTrainFractionHelp, TRUE);

   Bool covarDecomp = get_option_bool("COVAR_DECOMP", DEFAULT_COVAR,
				      covarDecompHelp, TRUE);


   for (int i = 0; i < repeatTimes; i++) {
      Array2<Real>* prob_yf_q = compute_prob_yf_q(*test);
      GLOBLOG(3, "prob(y_F | f,m,q) is " << *prob_yf_q << endl);

      Real acc;
      Array<Real>* err_q;
      Array2<Real>* prob_yh_q = multi_train(*inducer, *train, *test,
		    acc,err_q, trainTimes, internalTrainFract, replicationRandom);
      GLOBLOG(3, "Average error for each q: " << endl << *err_q << endl);
      GLOBLOG(3, "prob(Y_H | f,m,q) is " << *prob_yh_q << endl);

      if (covarDecomp)
	 cover_bias_var(*prob_yh_q, *prob_yf_q, 1 - acc);
      else
	 bias_var(*prob_yh_q, *prob_yf_q, 1 - acc, trainTimes,
		  sigmaData,  biasData,  varData,
		  eSigmaData, eBiasData, eVarData);
      delete prob_yh_q;
      delete err_q;
      delete prob_yf_q;
   }

   if (repeatTimes != 1 && !covarDecomp) {
      Mcout << "Average  bias=" << ROUND(biasData.mean())
	    << "+-" << ROUND(biasData.std_dev_of_mean())
	    << ". Average  var=" << ROUND(varData.mean())
	    << "+-" << ROUND(varData.std_dev_of_mean())
	    << endl;

      Mcout << "Average Ebias=" << ROUND(eBiasData.mean())
	    << "+-" << ROUND(eBiasData.std_dev_of_mean())
	    << ". Average Evar=" << ROUND(eVarData.mean())
	    << "+-" << ROUND(eVarData.std_dev_of_mean())
	    << endl;

#     define ROUNDM(x) MString(x/repeatTimes, ACC_PRECISION+2)
      Mcout << "MSE:  "
	 " bias="  << ROUNDM(biasData.squaredError(eBiasData.mean())) <<
	 " var="   << ROUNDM(varData.squaredError(eVarData.mean())) <<
	 " Ebias=" << ROUNDM(eBiasData.squaredError(eBiasData.mean())) <<
	 " Evar="  << ROUNDM(eVarData.squaredError(eVarData.mean())) << endl;
   }
   
   delete test;
   delete train;
   delete inducer;
   return 0; // return success to shell
}   



