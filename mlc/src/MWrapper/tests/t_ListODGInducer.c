// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests ListODGInducer (and TDDTInducer) methods. Reads and
                    trains of monk1.
  Doesn't test : 
  Enhancements : 
  History      : Chia-Hsin Li                                       12/30/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <ID3Inducer.h>
#include <CtrInstList.h>
#include <CatTestResult.h>
#include <EntropyODGInducer.h>
#include <ListODGInducer.h>

RCSID("MLC++, $RCSfile: t_ListODGInducer.c,v $ $Revision: 1.3 $")

// If you compile with -DINTERACTIVE, it will show the monks graph in X.

main()
{
   cout << "t_ListODGInducer executing" << endl;

   MString datafile = "monk1-full";
   EntropyODGInducer eInducer("Entropy ODG Inducer");
   ListODGInducer lInducer("ListODGInducer", &eInducer);

   // EntropyODGInducer.
   eInducer.read_data(datafile);
   eInducer.set_post_proc(none);
   eInducer.train();
   Mcout << "EntropyODGInducer: Graph has " << eInducer.num_nodes()
	 << " nodes, and " << eInducer.num_leaves() << " leaves." << endl;
   CatTestResult eResult(eInducer.get_categorizer(),
			eInducer.instance_bag(), datafile);
   Mcout << eResult << endl << endl;
   
   // ListODGInducer.
   lInducer.read_data(datafile);
   lInducer.set_split_info_array(eInducer.split_info_array());
   lInducer.set_post_proc(none);
   lInducer.train();
   Mcout << "ListODGInducre: Graph has " << lInducer.num_nodes()
	 << " nodes, and " << lInducer.num_leaves() << " leaves." << endl;
   CatTestResult lResult(lInducer.get_categorizer(),
			 lInducer.instance_bag(), datafile);
   Mcout << lResult << endl << endl;
   
#ifdef INTERACTIVE
     DotGraphPref pref;
     MLCOStream out(XStream);
     lInducer.display_struct(out,pref);
#endif

   MLCOStream out("t_ListODGInducer.out1");
   DotGraphPref pref;
   lInducer.display_struct(out,pref);
   
   return 0; // return success to shell
}   

