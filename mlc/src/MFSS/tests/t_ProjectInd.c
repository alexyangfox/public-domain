// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test ProjectInd Inducer.
  Doesn't test : 
  Enhancements :
  History      : Robert Allen                                       1/27/95
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <ID3Inducer.h>
#include <ProjectInd.h>
#include <CatTestResult.h>
#include <CtrInstList.h>
#include <BoolArray.h>
#include <TableInducer.h>
#include <errorUnless.h>


RCSID("MLC++, $RCSfile: t_ProjectInd.c,v $ $Revision: 1.7 $")


/*   Tester: Read monk1-full, project on attr 0,1,4, build decision tree.
           Build ProjectCat that projects on 0,1,4 and uses
           the decision tree categorizer (you'll have to copy the
           DTCategorizer).  Run CatTestResult on the testset
           and your categorizer and you should get 97.2%.  */


void test_batch_inducer()
{
   ProjectInd outerInd("ProjectInd: outer projecter");
   BaseInducer* wrappedIndPtr =
      new ID3Inducer("ProjectInd: wrapped ID3 Inducer");
   
   outerInd.set_wrapped_inducer(wrappedIndPtr);
   ASSERT(wrappedIndPtr == NULL);
   
   BoolArray* attr = new BoolArray(0, 6, FALSE);
   (*attr)[0] = TRUE;
   (*attr)[1] = TRUE;
   (*attr)[4] = TRUE;

   outerInd.set_project_mask(attr);
   ASSERT(attr == NULL);

   outerInd.read_data("monk1-full");
   outerInd.train();

   ASSERT(outerInd.can_cast_to_incr_inducer() == FALSE);

   InstanceList testSet("monk1-full", ".names", ".test");
   CatTestResult catResult(outerInd.get_categorizer(),
			   outerInd.instance_bag(), testSet);

   Mcout << "Projected Categorizer Results" << endl << catResult << endl;

   // test misc accessor methods and training twice:
   Mcout << "Wrapped Inducer: " << outerInd.get_wrapped_inducer().description()
	 << " used." << endl;
   ASSERT(!outerInd.can_cast_to_incr_inducer());
   BoolArray* attr2 = new BoolArray(0, 6, FALSE);
   outerInd.set_project_mask(attr2);
   outerInd.train();

   delete outerInd.release_wrapped_inducer();
   delete outerInd.release_categorizer();
   ASSERT(!outerInd.was_trained(FALSE));
}


/******************************************************************************
  Description  : This function test the correctness using TableInducer
                   without default category as the wrapped inducer.
  Comments     : Incremental functions not active.  See .c file header.
******************************************************************************/
/*
void test_incr_inducer()
{
   Mcout << endl << "   Testing incremental functions." << endl;
   ProjectInd outerInd("ProjectInd: outer projecter");
   ASSERT(!outerInd.can_cast_to_incr_inducer());
   Inducer* wrappedIndPtr =
      new TableInducer("ProjectInd: wrapped Tbl Inducer", FALSE);
   
   outerInd.set_wrapped_inducer(wrappedIndPtr);
   ASSERT(wrappedIndPtr == NULL);
   ASSERT(outerInd.can_cast_to_incr_inducer());
   
   
   BoolArray* attr = new BoolArray(0, 6, FALSE);
   (*attr)[0] = TRUE;
   (*attr)[1] = TRUE;
   (*attr)[4] = TRUE;

   outerInd.set_project_mask(attr);
   ASSERT(attr == NULL);

   outerInd.read_data("monk1-full");
   outerInd.train();
//   outerInd.train();  // need to rethink assumption of no data.
   
   InstanceList testSet("", "monk1-full.names", "monk1-full.test");
   CatTestResult catResult(outerInd.get_categorizer(),
			   outerInd.instance_bag(), testSet);

   Mcout << "Projected Table Categorizer Results" << endl << catResult << endl;

   ASSERT(outerInd.can_cast_to_incr_inducer());

   // test del_instance, add_instance
   int numInst = outerInd.instance_bag().num_instances();
   for (int i = 0; i < numInst; i++) {
      Pix pix = outerInd.instance_bag().first();
      const InstanceRC inst =
	 outerInd.cast_to_incr_inducer().del_instance(pix);
      const AugCategory& ac =
	 outerInd.get_categorizer().categorize(inst);
      outerInd.cast_to_incr_inducer().add_instance(inst);
      const AugCategory& ac2 =  outerInd.get_categorizer().categorize(inst);
      ASSERT((Category)ac2 != UNKNOWN_CATEGORY_VAL);
   }
   Mcout << "      Incremental Project Inducer Display:" << endl
	 << outerInd << endl;
}
*/

void test_errors()
{
   Mcout << endl << "   Testing error conditions." << endl;
   ProjectInd outerInd("ProjectInd: outer projecter");
   BaseInducer* wrappedIndPtr =
      new TableInducer("ProjectInd: wrapped Tbl Inducer", FALSE);
   wrappedIndPtr->read_data("monk1-full");
   
#ifndef MEMCHECK
   TEST_ERROR("ProjectInd::set_wrapped_inducer: Wrapped inducer shouldn't ",
	      outerInd.set_wrapped_inducer(wrappedIndPtr));
#endif
#ifndef MEMCHECK
   TEST_ERROR("ProjectInd::has_wrapped_inducer: No inducer set.  Call ",
	      const BaseInducer* rtnInd = outerInd.release_wrapped_inducer());
#endif
#ifndef MEMCHECK
   TEST_ERROR("ProjectInd::has_project_mask: No mask set.  Call ",
	      outerInd.has_project_mask(TRUE));
#endif
#ifndef MEMCHECK
   TEST_ERROR("ProjectInd::was_trained: No categorizer, ",
	      outerInd.was_trained(TRUE));
#endif
   
   InstanceBag *ts = wrappedIndPtr->release_data();
   delete wrappedIndPtr;
   wrappedIndPtr = new ID3Inducer("ProjectInd: wrapped ID3 Inducer");
   outerInd.set_wrapped_inducer(wrappedIndPtr);

   BoolArray* attrBad = new BoolArray(0, 5, TRUE);
   outerInd.set_project_mask(attrBad);

#ifndef MEMCHECK
   
   TEST_ERROR("ProjectInd::OK: Mask size (",
	      outerInd.assign_data(ts));
   ts = outerInd.release_data();

#endif


   BoolArray* attr = new BoolArray(0, 6, TRUE);
   outerInd.set_project_mask(attr);
   outerInd.assign_data(ts);
   ASSERT(ts == NULL);

   BoolArray* attrBad2 = new BoolArray(0, 7, TRUE);

#ifndef MEMCHECK
   TEST_ERROR("ProjectInd::OK: Mask size (",
	      outerInd.set_project_mask(attrBad2));
#endif
   attr = new BoolArray(0, 6, TRUE);
   outerInd.set_project_mask(attr);   // to allow gracefull destruction
   
   // Incremental functions not active:
//   outerInd.train();
//   Pix pix = outerInd.instance_bag().first();
//   InstanceRC inst = outerInd.instance_bag().get_instance(pix);
//#ifndef MEMCHECK
//   TEST_ERROR("ProjectInd::add_instance: Cannot add instance to ",
//	      outerInd.add_instance(inst));
//   TEST_ERROR("ProjectInd::del_instance: Cannot remove instance from ",
//	      outerInd.del_instance(pix));
//#endif

#ifdef MEMCHECK
   delete attrBad2;
#endif
}


main()
{
   Mcout << endl << "t_ProjectInd testing start." << endl;
   test_batch_inducer();
//   test_incr_inducer();
   test_errors();
   
   Mcout << "t_ProjectInd testing competed." << endl;
   return 0;
}
      




