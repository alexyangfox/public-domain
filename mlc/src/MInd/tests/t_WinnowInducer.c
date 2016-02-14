// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests WinnowInducer.c
  Doesn't test : 
  Enhancements : 
  History      :
                 Mehran Sahami                                       1/12/95
                   Initial revision based on t_PtronInducer.c
***************************************************************************/

#include <basics.h>
#include <InstList.h>
#include <WinnowInducer.h>
#include <errorUnless.h>
#include <CatTestResult.h>

main()
{
   InstanceList bag("t_WinnowInducerSimple");
   Mcout << "Read instances..." << endl;
   Mcout << bag << endl;

   Array<Real> a1(3);

   a1[0] = 4;
   a1[1] = -2;
   a1[2] = -0.7;

   WinnowInducer wi("WInducer Test1");

   Mcout << "INITIAL WINNOW" << endl;
   wi.set_log_level(3);

   Mcout << "TRYING TO SET BAD INITIAL WEIGHTS" << endl;
   TEST_ERROR("WinnowInducer::set_initialWeights: Initial weight",
	      wi.set_initialWeights(a1));

   a1[1] = 2;
   a1[2] = 1;

   Mcout << "TRYING TO SET BAD INITIAL THRESHOLD" << endl;
   TEST_ERROR("WinnowInducer::set_initialWeights: Initial weight",
	      wi.set_initialWeights(a1));

   a1[2] = -0.7;
   wi.set_initialWeights(a1);

   Mcout << "TRYING TO SET BAD MAX EPOCHS" << endl;
   TEST_ERROR("WinnowInducer::set_maxEpochs:",
	      wi.set_maxEpochs(-2));
   
   wi.set_maxEpochs(20);

   Mcout << "TRYING TO SET BAD ALPHA" << endl;
   TEST_ERROR("WinnowInducer::set_alpha: Alpha is being set",
	      wi.set_alpha(0.5));
   wi.set_alpha(2.0);

   Mcout << "TRYING TO SET BAD BETA" << endl;
   TEST_ERROR("WinnowInducer::set_beta: Beta is being set",
	      wi.set_beta(2.0));
   wi.set_beta(0.5);
   wi.read_data("t_WinnowInducerSimple");

   Mcout << "INITIAL WINNOW" << endl;
   TEST_ERROR("WinnowInducer::was_trained: No categorizer",
	      wi.display_struct(Mcout,DisplayPref::ASCIIDisplay));

   wi.train();
   Mcout << "TRAINED WINNOW" << endl;
   wi.display_struct(Mcout,DisplayPref::ASCIIDisplay);

   CatTestResult result(wi.get_categorizer(),
			wi.instance_bag(),
			bag);

   Mcout << "Winnow Inducer Results" << endl;
   result.display();


   //check get_categorizer
   Mcout << "CATEGORIZER IS: ";
   wi.get_categorizer().display_struct(Mcout,defaultDisplayPref);
   Mcout << endl;


   //try training without data
   Mcout << "TRYING TO TRAIN WITHOUT DATA" << endl;
   WinnowInducer wi2("WInducer Test2");
   //check has_data()
   Bool temp =  wi2.has_data(FALSE) ;

   Mcout << "Does WInducer Test3 have data: " << (temp?"yes":"no") << endl;
   
   TEST_ERROR("Inducer::has_data: Training data has not been set",
	      wi2.train());	      

   

   //try predicting without training
   WinnowInducer* wi3 = new WinnowInducer("WInducer Test3");
   wi3->read_data("t_WinnowInducerSimple");   
   Pix bagPix = bag.first();
   const InstanceRC& Inst = bag.get_instance(bagPix);

   Bool temp2 = wi3->was_trained(FALSE) ;

   Mcout << "Was WInducer Test3 trained? " <<(temp2?"yes":"no") << endl;

   Mcout << "TRYING TO PREDICT WITHOUT TRAINING" << endl;
   TEST_ERROR("WinnowInducer::was_trained",
	      wi3->predict(Inst));

   delete wi3;

   return 0; // return success to shell



}   
