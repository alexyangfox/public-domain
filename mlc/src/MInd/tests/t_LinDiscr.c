// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests LinDiscr.c
  Doesn't test : 
  Enhancements : 
  History      : George John                                         6/13/94
                   Name change PtronCat -> LinDiscr
                 George John                                         3/20/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <InstList.h>
#include <LinDiscr.h>
#include <InstanceRC.h>
#include <errorUnless.h>


main()
{
   InstanceList bag("t_LinDiscrSimple");
   
   Mcout << bag << endl;
   bag.normalize_bag(InstanceBag::extreme);
   Array<Real> a1(3);


   Mcout << "CREATING CATEGORIZER" << endl;
   LinearDiscriminant p1(bag.get_schema(),"PercepCat Test");

   p1.set_log_level(3);
   Mcout << "INITIAL WEIGHTS" << endl;
   p1.display_struct(Mcout);
   
   Mcout << "SETTING RANDOM WEIGHTS" << endl;
   p1.init_weights(a1);
   p1.set_weights(a1);
   p1.display_struct(Mcout);

   Mcout << "SETTING FIXED WEIGHTS" << endl;
   a1[0] = 1;
   a1[1] = 0;
   a1[2] = -1;
   
   p1.set_weights(a1);
   p1.display_struct(Mcout);


   Mcout << "SETTING WEIGHTS" << endl;
   a1[1] = 1;
   p1.set_weights(a1);
   p1.display_struct(Mcout);

   Mcout << "ADDING TO WEIGHTS <1 1 -1>" << endl;
   p1.add_to_weights(a1);
   p1.display_struct(Mcout);

   Mcout << "MULTIPLYING WEIGHTS <1 0 -1>" << endl;
   a1[0] = 1;
   a1[1] = 0;
   a1[2] = -1;
   p1.multiply_weights(a1);
   p1.display_struct(Mcout);

   Mcout << "TESTING CATEGORIZER" << endl;
   for(Pix p=bag.first(); p; bag.next(p))
     Mcout << " " << p1.categorize(bag.get_instance(p));
   Mcout << endl;


   //try copying
   Mcout << "COPYING CATEGORIZER" << endl;
   LinearDiscriminant copyp(p1,ctorDummy);

   Mcout << "CHECKING == OPERATOR" << endl;
   if (p1 == copyp) {
      Mcout << "copy is the same as original" << endl;
   } else {
      Mcout << "ERROR: copy is different than original" << endl;
   }

   if (p1 == p1) {
      Mcout << "original is the same as original" << endl;
   } else {
      Mcout << "ERROR: original is different than original" << endl;
   }
   
   Mcout << "TESTING CATEGORIZER" << endl;
   for(p=bag.first(); p; bag.next(p))
     Mcout << " " << p1.categorize(bag.get_instance(p));
   Mcout << endl;
   
   Mcout << "TESTING COPY" << endl;
   for(p=bag.first(); p; bag.next(p))
     Mcout << " " << copyp.categorize(bag.get_instance(p));
   Mcout << endl;


   //try sending it data with !=2 categories

   InstanceList bag2("t_LinDiscrThree");
   Mcout << "TRYING TO CREATE LINEAR DISCRIMINANT WITH 3 CLASSES" << endl;
   //this should cause a fatal error because has 3 classes
   Mcout << "Database has " << (bag2.get_schema()).num_label_values()
      << " classes" << endl;
   
   TEST_ERROR("LinearDiscriminant::LinearDiscriminant: Number of",
	      LinearDiscriminant p2(bag2.get_schema(),
				    "PercepCat Test2"));



   //try sending it nominal data

   InstanceList bag3("t_LinDiscrNom");
   Mcout << "TRYING TO CREATE PERCEPTRON WITH NOMINAL ATTINFO" << endl;
   //this should cause a fatal error because one attrInfo is nominal
   TEST_ERROR("LinearDiscriminant::LinearDiscriminant: Attribute",
	      LinearDiscriminant p3(bag3.get_schema(),
				       "PercepCat Test3"));



   //try sending it a set_weights array with base !=0, wrong size

   Array<Real> a2(1,4);
   a2[1] = 1;
   a2[2] = -1;
   a2[3] = 1;
   Mcout << "SETTING WEIGHTS WITH BOGUS ARRAY" << endl;
   TEST_ERROR("LinearDiscriminant::set_weights:",
	      p1.set_weights(a2));

   Array<Real> a3(4);
   a3[0] = 0;
   a3[1] = 1;
   a3[2] = -1;
   a3[3] = 1;
   Mcout << "SETTING WEIGHTS WITH BOGUS ARRAY" << endl;
   TEST_ERROR("LinearDiscriminant::set_weights:",
	      p1.set_weights(a3));

   Mcout << "ADDING BOGUS ARRAY TO WEIGHTS" << endl;
   DBGSLOW(TEST_ERROR("LinearDiscriminant::add_to_weights:",
	      p1.add_to_weights(a2)));
   DBGSLOW(TEST_ERROR("LinearDiscriminant::add_to_weights:",
	      p1.add_to_weights(a3)));
   Mcout << "MULTIPLYING BOGUS ARRAY WITH WEIGHTS" << endl;
   DBGSLOW(TEST_ERROR("LinearDiscriminant::multiply_weights:",
	      p1.multiply_weights(a2)));
   DBGSLOW(TEST_ERROR("LinearDiscriminant::multiply_weights:",
	      p1.multiply_weights(a3)));

   return 0; // return success to shell
}   
