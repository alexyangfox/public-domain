// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a small test of linking with the MLC++ library.
// It should simply say "MLC++ setup OK"
// If the file can't be found, set the MLCPATH
//   environment variable to point to the directory for monk1-full.

#include <basics.h>
#include <ID3Inducer.h>
#include <CtrInstList.h>
#include <CatTestResult.h>

main()
{
   MString datafile("monk1");
   Mcout << "setupTest executing..." << endl;
   
   ID3Inducer inducer("ID3 Inducer");
   inducer.read_data(datafile);
   inducer.train();
   CatTestResult result(inducer.get_categorizer(),
                        inducer.instance_bag(), datafile);
   if (result.accuracy() > 0.82 || result.accuracy() < 0.80)
      err << "setupTest: test failed on " << datafile << fatal_error;

   Mcout << "setupTest OK" << endl;
   
   return 0; // return success to shell
}   
