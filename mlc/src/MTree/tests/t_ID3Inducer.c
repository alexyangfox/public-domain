// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests ID3Inducer (and TDDTInducer) methods.  Reads and
                   trains on descretized golf problem and outputs number
		   predicted correctly.  Reads and trains of
		   full_paity and  monks1.  Outputs structures to files
		   which can be diffed against the correct output.
  Doesn't test : 
  Enhancements : 
  History      : Richard Long                                       9/08/93
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <ID3Inducer.h>
#include <CtrInstList.h>
#include <CatTestResult.h>

RCSID("MLC++, $RCSfile: t_ID3Inducer.c,v $ $Revision: 1.26 $")

// If you compile with -DINTERACTIVE, it will show the monks graph in X.


void testCat(const MString& fileName)
{
   CtrInstanceList* bag = new CtrInstanceList(fileName);
   ID3Inducer inducer(fileName + " ID3 inducer");
   inducer.assign_data(bag);
   ASSERT(bag == NULL); // assign data gets ownership
   inducer.train();
   CatTestResult id3Result(inducer.get_categorizer(),
			   inducer.instance_bag().cast_to_instance_list(),
			   fileName);
   MLCOStream out1(fileName + ".out1");
   Mcout << "ID3 Inducer Results" << endl << id3Result;
   Mcout << "Id3 overall accuracy: "
	 << id3Result.accuracy(CatTestResult::Normal) << endl
	 << "Id3 memorized accuracy: "
	 << id3Result.accuracy(CatTestResult::Memorized) << endl
	 << "Id3 generalized accuracy: "
	 << id3Result.accuracy(CatTestResult::Generalized) << endl;

   id3Result.display_all();
}

// Don't use CatTestResult
void simple_ID3()
{
   ID3Inducer id3Inducer("t_ID3Inducer");

#ifndef MEMCHECK
   TEST_ERROR("TDDTInducer::was_trained: No decision tree categorizer. ",
	      id3Inducer.num_nodes());
   TEST_ERROR("CtrInducer::TS_with_counters(): Training data has not "
	      "been set", id3Inducer.TS_with_counters());
#endif

   id3Inducer.set_log_level(4);
   id3Inducer.set_debug(TRUE);
   MLCOStream logstream("t_ID3Inducer.out3");
   id3Inducer.set_log_stream(logstream);
   id3Inducer.read_data("t_ID3Inducer");
   id3Inducer.train();

   // To cover the get_categorizer() method.
   const Categorizer& categorizer = id3Inducer.get_categorizer();
   (void)categorizer;

   InstanceList bag("t_ID3Inducer");

   int numRight = 0;
   for (Pix bagPix = bag.first(); bagPix; bag.next(bagPix)) {
      InstanceRC instance = bag.get_instance(bagPix);
      if (instance.label_info().get_nominal_val(instance.get_label())
	  == id3Inducer.predict(instance))
	  numRight++;
   }
    
   cout << "Num right = " << numRight << endl;
   cout << "Num nodes = " << id3Inducer.num_nodes() << endl;
   id3Inducer.display_struct(Mcout);
}


void testOptions(const MString& fileName)
{
   CtrInstanceList* bag = new CtrInstanceList(fileName);
   ID3Inducer inducer(fileName + " ID3 inducer");
   inducer.set_user_options("ID3_");
   inducer.set_unknown_edges(FALSE);
   inducer.assign_data(bag);
   ASSERT(bag == NULL); // assign data gets ownership
   inducer.train();
   CatTestResult id3Result(inducer.get_categorizer(),
			   inducer.instance_bag().cast_to_instance_list(),
			   fileName);
   MLCOStream out1(fileName + ".out1");
   Mcout << "ID3 Inducer Results" << endl << id3Result;
   Mcout << "Id3 overall accuracy: "
	 << id3Result.accuracy(CatTestResult::Normal) << endl
	 << "Id3 memorized accuracy: "
	 << id3Result.accuracy(CatTestResult::Memorized) << endl
	 << "Id3 generalized accuracy: "
	 << id3Result.accuracy(CatTestResult::Generalized) << endl;
}

main()
{
   cout << "t_ID3Inducer executing" << endl;

   testCat("t_ID3Inducer2");
   
   
   putenv("SPLIT_BY=mutual");
   simple_ID3();

   // test for prunning with nominal dataset
   putenv("ID3_LOWER_BOUND_MIN_SPLIT=5");
   testOptions("monk1");
   putenv("ID3_LOWER_BOUND_MIN_SPLIT=1");

   ID3Inducer id3Inducer("my id3");
   id3Inducer.set_log_level(4);
   id3Inducer.set_debug(TRUE);
   MLCOStream logstream("t_ID3Inducer.out3");
   id3Inducer.set_log_stream(logstream);
   id3Inducer.read_data("t_ID3full_parity");
   id3Inducer.set_debug(FALSE);
   id3Inducer.train();
   MLCOStream out1("t_ID3Inducer.out1");
   MLCOStream out2("t_ID3Inducer.out2");
   id3Inducer.display_struct(out1);

   if (!centerline_true()) { // takes way too long...
      id3Inducer.read_data("monk1-full");
      id3Inducer.train();

#ifdef INTERACTIVE
      DotGraphPref pref;
      MLCOStream out3(XStream);
      id3Inducer.display_struct(out3,pref);
#endif INTERACTIVE

      DotPostscriptPref dpp;
      FloatPair fp(20.0, 40.0);
      dpp.set_graph_size(fp);
      id3Inducer.display_struct(out2,dpp);
   }

#ifndef MEMCHECK
   ID3Inducer emptyID3("Empty ID3");
   CtrInstanceList* emptyList = NULL;
   emptyID3.assign_data(emptyList); // for coverage
   emptyList = new CtrInstanceList(id3Inducer.instance_bag().get_schema());

   emptyID3.assign_data(emptyList);
   ASSERT(emptyList == NULL);
   TEST_ERROR("TDDTInducer::induce_decision_tree: zero instances",
	      emptyID3.train());
#endif

   // Test ID3 on a dataset with real attributes.
   ID3Inducer id3InducerReal("t_ID3InducerReal");
   id3InducerReal.set_log_level(0);
   id3InducerReal.set_c45_options();
   id3InducerReal.set_debug(TRUE);
   id3InducerReal.read_data("t_ID3Inducer-real");
   id3InducerReal.train();
   id3InducerReal.display_struct(Mcout);

   
#ifdef INTERACTIVE
   DotGraphPref pref;
   MLCOStream tmpout(XStream);
   id3InducerReal.display_struct(tmpout,pref);
#endif INTERACTIVE
   
   return 0; // return success to shell
}   
