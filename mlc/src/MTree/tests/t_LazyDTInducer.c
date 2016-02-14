// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : Tester for LazyDTInducer.
                 Run LazyDTInducer on several data files, using different
		   option values.
  Comments     :
  Enhancements :
  History      : Yeogirl Yun                                  June 2, 1995
                   Initial revision
***************************************************************************/


#include <basics.h>
#include <LazyDTInducer.h>
#include <CtrInstList.h>
#include <CatTestResult.h>

void print_result(const CatTestResult& result,
		  const LazyDTInducer& lazy,
		  const MString& dataFile)
{
   Mcout << "=================================================" << endl;
   Mcout << "Data File     : " << dataFile << endl;
   Mcout << "MIN SPLIT        : " << lazy.get_min_split() << endl;
   Mcout << "MUTUAL_INFO_RATE    : " << lazy.get_mutual_info_rate() << endl;
   Mcout << "USE ATTR EQ CAT     : ";
   if (lazy.get_use_attr_eq_cat_bool())
      Mcout << "Yes" << endl;
   else
      Mcout << "No" << endl;
   Mcout << "DEL RATE : " << lazy.get_del_rate() << endl;
   Mcout << "MULTI SPLIT : ";
   if (lazy.get_multi_split_bool()) {
      Mcout << "Yes" << endl;
      Mcout << "MULTI SPLIT RATE  : " << lazy.get_multi_split_rate() << endl;
   }
   else {
      Mcout << "No" << endl;
      Mcout << "MULTI SPLIT RATE  : Not Applicable" << endl;
   }
   Mcout << "PESSIMISTIC Z VAL    : " << lazy.get_pessimisticZ() << endl;
   Mcout << "PENALTY POWER : " << lazy.get_penalty_power() << endl;
   Mcout << result;
   Mcout << "=================================================\n" << endl;   
}   


void run_lazy_inducer(const MString& datafile,
		      Bool useAttrEq,
		      Bool useMultiSplit)
{
   LazyDTInducer lazy("Lazy DT Inducer");

   CtrInstanceList *trainSet = new CtrInstanceList(datafile);
   CtrInstanceList testSet("", datafile + ".names", datafile + ".test");

   delete lazy.assign_data(trainSet);
   // set options.
   lazy.set_use_attr_eq_cat_bool(useAttrEq);
   lazy.set_multi_split_bool(useMultiSplit);
   lazy.train();

   CatTestResult result(lazy.get_categorizer(),
			lazy.instance_bag(),
                        testSet);
   print_result(result, lazy, datafile);
}


int main()
{
   Mcout << "testing t_LazyDTInducer.c ... " << endl;
   
   run_lazy_inducer("monk1", TRUE, TRUE);
   run_lazy_inducer("monk1", TRUE, FALSE);
   run_lazy_inducer("monk1", FALSE, TRUE);	
   run_lazy_inducer("monk1", FALSE, FALSE);

   run_lazy_inducer("vote", TRUE, TRUE);
   run_lazy_inducer("vote", TRUE, FALSE);
   run_lazy_inducer("vote", FALSE, TRUE);	
   run_lazy_inducer("vote", FALSE, FALSE);      

   return 0;
}
