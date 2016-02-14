// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : Tests ContinFilterInducer.
  Comments     :
  Enhancements :
  History      : Yeogirl Yun                                     7/5/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <CFInducer.h>
#include <CtrBag.h> 
#include <CtrInstList.h>
#include <PtronInducer.h>

const MString DATAFILE1 = "monk1-full";
const MString DATAFILE2 = "vote";

/*****************************************************************************
  Description : Show the result of a test.
  Comments    :
*****************************************************************************/
void show_result(const ContinFilterInducer& inducer,
		 const MString& datafile,
		 Real acc)
{

   Mcout << "-------------------------------------" << endl;
   Mcout << "DATAFILE : " << datafile << endl;
   Mcout << "NORMALIZATION : ";
   if (inducer.get_norm_method() == noNormalization)
      Mcout << "NO NORMALIZATION" << endl;
   else if (inducer.get_norm_method() == extreme)
      Mcout << "EXTREME" << endl;
   else if (inducer.get_norm_method() == normalDist)
      Mcout << "NORMAL DISTANCE" << endl;
   else
      err << "t_CFInducer.c : unknown normalization method : "  <<
	 inducer.get_norm_method() << fatal_error;

   Mcout << "CONVERSION : ";
   if (inducer.get_conv_method() == local)
      Mcout << "LOCAL" << endl;
   else if (inducer.get_conv_method() == binary)
      Mcout << "BINARY" << endl;
   else if (inducer.get_conv_method() == noConversion)
      Mcout << "NO CONVERSION" << endl;
   else
      err << "t_CFInducer.c : not legal conversion method for "
	 "ContinFilterInducer : " << inducer.get_conv_method() << fatal_error;

   Mcout << "ACCURACY : " << acc << endl;
   Mcout << "-------------------------------------" << endl << endl;   
}
      

void test_inducer(BaseInducer* cfInducer,
		  const MString& datafile,
		  Normalization normMethod,
		  Conversion convMethod)
{
   ContinFilterInducer continFilter("Contin Filter Inducer");
   continFilter.set_cf_sub_inducer(cfInducer);
   continFilter.set_norm_method(normMethod);
   continFilter.set_conv_method(convMethod);

   CtrInstanceList trainSet(datafile, ".names", ".data");
   CtrInstanceList testSet(datafile, ".names", ".test");
   
   Real acc = continFilter.train_and_test(&trainSet, testSet);
   show_result(continFilter, datafile, acc);
}


void test_option_combi_on_perceptron(const MString& datafile)
{
   test_inducer(new PerceptronInducer("Perceptron Inducer"),
		datafile, noNormalization, local);
   test_inducer(new PerceptronInducer("Perceptron Inducer"),
		datafile, extreme, local);
   test_inducer(new PerceptronInducer("Perceptron Inducer"),
		datafile, normalDist, local);
   test_inducer(new PerceptronInducer("Perceptron Inducer"),
		datafile, noNormalization, binary);
   test_inducer(new PerceptronInducer("Perceptron Inducer"),
		datafile, extreme, binary);
   test_inducer(new PerceptronInducer("Perceptron Inducer"),
		datafile, normalDist, binary);
}

int main()
{
   Mcout << "testing t_CFInducer.c ... " << endl;
   test_option_combi_on_perceptron(DATAFILE1);
   test_option_combi_on_perceptron(DATAFILE2);

   Mcout << "Success!" << endl;
   return 0;
}   
   
   
      
