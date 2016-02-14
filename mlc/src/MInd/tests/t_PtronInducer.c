// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests PtronInducer.c
  Doesn't test : 
  Enhancements : 
  History      :
                 Mehran Sahami                                       1/12/95
		   Made changes to make file consistent with other
		   changes in MLC++ base classes and test multiply_weights
		   method.
                 George John                                         6/14/94
		   Code review mods
                 George John                                         3/20/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <InstList.h>
#include <PtronInducer.h>
#include <errorUnless.h>
#include <CatTestResult.h>

main()
{
   InstanceList bag("t_PtronInducerSimple");
   Mcout << bag << endl;

   Array<Real> a1(4);

   a1[0] = 1;
   a1[1] = 1;
   a1[2] = 1;
   a1[3] = 1;
   
   PerceptronInducer pi("PInducer Test1");
   pi.set_log_level(3);
   pi.set_initialWeights(a1);

   Mcout << "TRY TO SET BAD MAX EPOCHS" << endl;
   TEST_ERROR("PerceptronInducer::set_maxEpochs:",
	      pi.set_maxEpochs(-1));
   pi.set_maxEpochs(20);

   Mcout << "TRY TO SET BAD LEARNING RATE" << endl;
   TEST_ERROR("PerceptronInducer::set_learningRate:",
	      pi.set_learningRate(-1));

   pi.set_learningRate(.2);
   pi.read_data("t_PtronInducerSimple");

   Mcout << "INITIAL PERCEPTRON" << endl;
   TEST_ERROR("PerceptronInducer::was_trained: No categorizer",
	      pi.display_struct(Mcout,DisplayPref::ASCIIDisplay));

   pi.train();
   Mcout << "TRAINED PERCEPTRON" << endl;
   pi.display_struct(Mcout,DisplayPref::ASCIIDisplay);

   CatTestResult result(pi.get_categorizer(),
			pi.instance_bag(),
			bag);

   Mcout << "Perceptron Inducer Results" << endl;
   result.display();


   //check get_categorizer
   Mcout << "CATEGORIZER IS: ";
   pi.get_categorizer().display_struct(Mcout,defaultDisplayPref);
   Mcout << endl;


   //try training without data
   Mcout << "TRYING TO TRAIN WITHOUT DATA" << endl;
   PerceptronInducer pi2("PInducer Test2");
   //check has_data()
   Mcout << "Does PInducer Test3 have data? " << endl;
   Bool temp =  pi2.has_data(FALSE) ;
   TEST_ERROR("Inducer::has_data: Training data has not been set",
	      pi2.train());	      

   

   //try predicting without training
   PerceptronInducer* pi3 = new PerceptronInducer("PInducer Test3");
   pi3->read_data("t_PtronInducerSimple");   
   Pix bagPix = bag.first();
   const InstanceRC& Inst = bag.get_instance(bagPix);
   Mcout << "Was PInducer Test3 trained? " << endl;
   Bool temp2 = pi3->was_trained(FALSE) ;

   Mcout << "TRYING TO PREDICT WITHOUT TRAINING" << endl;
   TEST_ERROR("PerceptronInducer::was_trained",
	      pi3->predict(Inst));

   delete pi3;

   return 0; // return success to shell



}   
