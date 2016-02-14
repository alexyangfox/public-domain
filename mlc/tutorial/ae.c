// Tutorial program : accuracy estimation

#include <basics.h>
#include <ID3Inducer.h>   
#include <NaiveBayesInd.h>
#include <CtrInstList.h>   
#include <OptionAccEst.h>  // For OptionAccEst

main()
{
   ID3Inducer    inducerDT("Xema-DT"); // Decision tree inducer
   NaiveBayesInd inducerNB("Xema-NB"); // Naive-Bayes inducer

   CtrInstanceList bag("breast");
   OptionAccEst oac;

   // Keep the inducer quiet since we'll be running it many times
   inducerDT.set_log_level(max(globalLogLevel - 3, 0));
   oac.estimate_accuracy(inducerDT, bag);
   Mcout << "Estimated accuracy for decision tree\n" << oac << endl;
   
   inducerNB.set_log_level(max(globalLogLevel - 3, 0));
   oac.estimate_accuracy(inducerNB, bag);
   Mcout << "Estimated accuracy for naive-bayes\n" << oac << endl;

   return 0;
}   
