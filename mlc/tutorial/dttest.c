// Tutorial program : Run and test ID3

#include <basics.h>
#include <ID3Inducer.h>
#include <CatTestResult.h>  // required for CatTestResult

main()
{
   ID3Inducer inducer("Xema");
   inducer.read_data("tutorial");
   inducer.train();

   // CatTestResult requires a categorizer, the training set, and the
   //    test set.  The default extension for test files is ".test"
   CatTestResult result(inducer.get_categorizer(),
                        inducer.instance_bag(), "tutorial");
   Mcout << result << endl;  // Give statistics.

   return 0;
}   
