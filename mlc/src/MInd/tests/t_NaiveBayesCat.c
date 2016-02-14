// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test NaiveBayesCat
  Doesn't test : 
  Enhancements :
  History      : Robert Allen                                      12/11/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <CtrInstList.h>
#include <MLCStream.h>  // MLC++ stream operations.
#include <NaiveBayesInd.h>
#include <CatTestResult.h>
#include <AugCategory.h>
#include <ConstCat.h>
#include <DisplayPref.h>

RCSID("MLC++, $RCSfile: t_NaiveBayesCat.c,v $ $Revision: 1.10 $")

/************************************************************************
  Test the inducer
***********************************************************************/
void test0_inducer_tests( NaiveBayesInd& ind )
{
   ind.OK(); 
   ind.set_log_level(4);
#ifndef MEMCHECK
   TEST_ERROR("NaiveBayesInd::was_trained: No categorizer,",
	      ind.was_trained( TRUE ));
#endif
   ind.display( Mcout );

   // test error on Xstream
#ifndef MEMCHECK
   MLCOStream xout(XStream);
   TEST_ERROR("NaiveBayesInd::display_struct: Xstream",
	      ind.display( xout ));

   TEST_ERROR("NaiveBayesInd::display_struct: Only ASCIIDisplay",
	      ind.display( Mcout, DisplayPref::DotGraphDisplay ));
#endif
 
   
}


void test1_construct_destruct(// CtrInstanceList& bag,
			       NaiveBayesInd& ind )
{
   ind.OK(); 
   ind.train();

   NaiveBayesCat nbc((const NaiveBayesCat &)ind.get_categorizer(), ctorDummy);

   nbc.OK(9);
//   ASSERT( bag.counters() == *(nbc.counters()) );
   
   // check copy constructor
   NaiveBayesCat * tempCat = new NaiveBayesCat( nbc, ctorDummy );
   tempCat->OK();
   ASSERT ( *tempCat == (const NaiveBayesCat &)ind.get_categorizer() );
   
   // check destructor
   delete tempCat;

   // run deep copy mechanism
   Categorizer * deepCat = nbc.copy();

   ASSERT ( *(NaiveBayesCat *)deepCat ==
             (const NaiveBayesCat &)ind.get_categorizer() );

   delete deepCat;
}

  
// running categorizer:
void test2_categorizing( NaiveBayesCat& nbc, CtrInstanceList& bag,
			 MString& filestring, MString& badstring )
{
   Mcout << "test2_categorizing" << endl;
   (void)badstring; // ifdef MEMCHECK, this isn't used
#ifndef MEMCHECK
   // check errors of instance coming from different schema than training bag.
   
   CtrInstanceList wrongBag(badstring);
   InstanceRC inst(wrongBag.get_instance(wrongBag.first()));  
   nbc.set_log_level(0); // or it outputs stuff that only gets executed when
			 // debuglevel is > 0
   TEST_ERROR("NaiveBayesCat::categorize: Schema of",
	      nbc.categorize( inst ));

   CtrInstanceList badBag("t_NaiveBayesCat_Bad");
   InstanceRC inst2(wrongBag.get_instance(badBag.first()));  
   TEST_ERROR("NaiveBayesCat::categorize: Schema of",
	      nbc.categorize( inst2 ));

#endif
   
   nbc.set_log_level(3);
   // run the standard handler to produce output.  Should be 100% accurate.
   // t_NaiveBayesCat_Con.test includes an extreme case of no matches
   // with .data file.
   CatTestResult result(nbc, bag, filestring);
   Mcout << result << endl;
   ASSERT ( result.num_correct() == result.num_test_instances() );
   

}

/* 
void test3_instance_tweeking( NaiveBayesInd& ind,
			      MString& fileOne, MString& fileRest)
{
   // need bag here to get a schema
   CtrInstanceList bag(fileRest);

   // check error from OK if no training data present
#ifndef MEMCHECK
   BagCounters emptyCounter( bag.get_schema() );
   Array2<NBNorm> * nbn =
      new Array2<NBNorm>(bag.get_schema().num_attr(), bag.num_categories());
   NaiveBayesCat NBC("test nb categorizer",
		     bag.num_categories(), emptyCounter, nbn, 0,
		     bag.get_schema().num_attr());
   TEST_ERROR("NaiveBayesCat::OK: Number of training", NBC.OK());
#endif

   // Check Incremental Adding
   // produce categorizer with only one instance
   NaiveBayesInd indIncr("test nb categorizer");
   indIncr.read_data(fileOne);
   indIncr.OK(); 
   indIncr.train();
   // add all instances to counter
   Pix pix = bag.first();
   for (; pix; bag.next(pix)) {
      indIncr.add_instance(bag.get_instance(pix));
   }
   // compare original categorizer to new one
   ASSERT( (const NaiveBayesCat &)indIncr.get_categorizer() ==
	   (const NaiveBayesCat &)ind.get_categorizer() );

   ASSERT(
      ((NaiveBayesCat &)(indIncr.get_categorizer())).num_train_instances()
	   == 14 );
   
   // Check Incremental Deletions
   // delete one instance and look for inequality
   Pix pixy = indIncr.TS_with_counters().first();
   indIncr.del_instance(pixy);
   ASSERT( !( (const NaiveBayesCat &)indIncr.get_categorizer() ==
	   (const NaiveBayesCat &)ind.get_categorizer() ));

   // eliminate all instances
   for (pixy = indIncr.TS_with_counters().first();
	pixy;
	pixy = indIncr.TS_with_counters().first() ) {
      indIncr.del_instance(pixy);
   }
   
   ASSERT(indIncr.was_trained(FALSE) == FALSE );
}

*/

void test4_returns( NaiveBayesCat& nbc, /* CtrInstanceList& bag, */
		    const Categorizer &baseNBC )
{

   // check operator== verification on other type of categorizer
   const AugCategory ac(UNKNOWN_CATEGORY_VAL, "any old value");

   ConstCategorizer CatX("test cat id()", ac);
   ASSERT( !(nbc == CatX) );
   
   // check operator== verification on base of NaiveBayes
   ASSERT( nbc == baseNBC );

   // check return of bag counters
//   ASSERT( nbc.counters()->label_num_vals() == bag.num_categories() );
}

void test5_display(NaiveBayesCat& nbc)
{
   // call the display 
   nbc.display_struct( Mcout );

   Mcout << "Number of training instances = " << nbc.num_train_instances()
      << endl;
   
   // test error on Xstream
#ifndef MEMCHECK
   MLCOStream xout(XStream);
   TEST_ERROR("NaiveBayesCat::display_struct: Xstream",
	      nbc.display_struct( xout ));

   TEST_ERROR("NaiveBayesCat::display_struct: Only ASCIIDisplay",
	      nbc.display_struct( Mcout, DisplayPref::DotGraphDisplay ));
#endif
   
}

int main ()
{

   // test everything for continous attributes, then for nominal only

   Mcout << "** Testing NaiveBayesCat" << endl;
 
   MString filestring("t_NaiveBayesCat_Con");
   MString wrongfile("t_NaiveBayesCat");
   
   Mcout << "**** Testing Contiguous DataFile t_NaiveBayesCat_Con" << endl;
   CtrInstanceList bag(filestring);
   
   NaiveBayesInd ind("test nb categorizer");
   test0_inducer_tests( ind );

   ind.read_data(filestring);

   
   test1_construct_destruct( ind );
   Mcout << "** Constructors and destructor OK" << endl;
   
   ind.OK(); 
   ind.train();
   Mcout << ind << endl;
   
   NaiveBayesCat nbc((const NaiveBayesCat &)ind.get_categorizer(), ctorDummy);
   
   test2_categorizing( nbc, bag, filestring, wrongfile );
   Mcout << "** Categorizing OK" << endl;
      
   // test3 removes all instances from the inducer, but nbc should have
   // its own copy and remain unchanged.
 // test3_instance_tweeking( ind, fileOne, fileRest );
   Mcout << "** Modifying training set OK" << endl;
   
   
   test4_returns( nbc, /* bag,*/ nbc );
   Mcout << "** operators OK" << endl;
   
   test5_display( nbc );
   Mcout << "** Display OK" << endl;
   

      
   wrongfile = filestring;
   filestring = MString("t_NaiveBayesCat");
   Mcout << endl
	 << "**** Testing Nominal Only DataFile t_NaiveBayesCat" << endl;
   CtrInstanceList bagN(filestring);

   NaiveBayesInd indN("test nb categorizer");
   indN.read_data(filestring);
      
   test1_construct_destruct( /* bagN,*/ indN );
   Mcout << "** Constructors and destructor OK" << endl;
   
   indN.OK(); 
   indN.train();
   NaiveBayesCat nbcN((const NaiveBayesCat &)indN.get_categorizer(),
		      ctorDummy);
      
   test2_categorizing( nbcN, bagN, filestring, wrongfile );
   Mcout << "** Categorizing OK" << endl;
      
   // test3 removes all instances from the inducer, but nbc should have
   // its own copy and remain unchanged.
 // test3_instance_tweeking( indN, fileOne, fileRest );
   Mcout << "** Modifying training set OK" << endl;
      
      
   test4_returns( nbcN, /* bagN, */ nbcN );
   Mcout << "** operators OK" << endl;
   
   test5_display( nbcN );
   Mcout << "** Display OK" << endl;
      
   
   // the following are to test the blocking of the copy constructor.
   // these should produce compile time errors
   
#ifdef CHECKCOPY
   NaiveBayesCat copyNbc1 = *nbcN;
   NaiveBayesCat copyNbc2( *nbcN );
#endif
   
   Mcout << "** NaiveBayesCat testing completed" << endl;

   return 0;
}
