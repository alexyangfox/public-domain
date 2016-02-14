// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Test the Discretizing Dispatcher.
  Assumptions  :
  Comments     :
  Complexity   :
  Enhancements :
  History      : James F. Dougherty                                02/27/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <DiscDispatch.h>
#include <errorUnless.h>

RCSID("MLC++, $RCSfile: t_DiscDispatch.c,v $ $Revision: 1.2 $")


/***************************************************************************
  Description : Displays the dispatchers information.
  Comments    :
***************************************************************************/
void DiscDisplay(DiscDispatch* discDispatcher)
{
   discDispatcher->OK();
   Mcout << "-------------------------------------------" << endl;
   for(int i = 0; i < discDispatcher->discretizors()->size(); i++)
      if (discDispatcher->discretizors()->index(i))
	 discDispatcher->discretizors()->index(i)->display();
   Mcout << endl << "-------------------------------------------" << endl;
}

/***************************************************************************
  Description : Creates a dispatcher and plays around with some of the
                  functionality it exhibits.
  Comments    :
***************************************************************************/
void test_dispatch()
{
   InstanceList bag("t_DiscDispatch");
   DiscDispatch* discDispatcher = new DiscDispatch();

   Mcout << "Entropy via MDL ... " << endl;
   //This uses the default of initVal = 0 ( and invokes MDL entropy)
   discDispatcher->set_disc_type(DiscDispatch::entropy);
   discDispatcher->create_discretizors(bag);
   DiscDisplay(discDispatcher);

   Mcout << "T2 ... " << endl;
   discDispatcher->set_disc_type(DiscDispatch::t2Disc);
   discDispatcher->create_discretizors(bag);
   DiscDisplay(discDispatcher);

   Mcout << "C4.5... " << endl;
   discDispatcher->set_disc_type(DiscDispatch::c45Disc);
   discDispatcher->create_discretizors(bag);
   DiscDisplay(discDispatcher);

   Mcout << "Regular entropy to 5 bins ... " << endl;
   //This uses our version of Entropy (ID3)
   discDispatcher->set_disc_type(DiscDispatch::entropy);
   discDispatcher->set_bin_heuristic(DiscDispatch::FixedValue);
   discDispatcher->set_initial_val(5);
   discDispatcher->create_discretizors(bag);
   DiscDisplay(discDispatcher);

   Mcout << "T2 to 5 bins... " << endl;
   discDispatcher->set_disc_type(DiscDispatch::t2Disc);
   discDispatcher->create_discretizors(bag);
   DiscDisplay(discDispatcher);


   Mcout << "Binning into 5 equal bins ..." << endl;
   //This uses binning with a default of 5 bins (as set previously)
   discDispatcher->set_disc_type(DiscDispatch::binning);
   discDispatcher->create_discretizors(bag);
   DiscDisplay(discDispatcher);

   Mcout << "Binning using k*Log(N) bins ... " << endl;
   //Invoke the binning discretization using k*Log(N) heuristic
   discDispatcher->set_initial_val(0);
   discDispatcher->create_discretizors(bag);
   DiscDisplay(discDispatcher);

   Mcout << "Binning using k*Log(N) bins using algoHeuristic... " << endl;
   discDispatcher->set_bin_heuristic(DiscDispatch::AlgoHeuristic);
   discDispatcher->create_discretizors(bag);
   DiscDisplay(discDispatcher);

   Mcout << "Binning via Holte's OneR using default SMALL" << endl;
   discDispatcher->set_disc_type(DiscDispatch::oneR);
   discDispatcher->create_discretizors(bag);
   DiscDisplay(discDispatcher);

   Mcout << "Binning via MDL heuristic for T2" << endl;
   discDispatcher->set_disc_type(DiscDispatch::t2Disc);
   discDispatcher->set_bin_heuristic(DiscDispatch::MDL);
   discDispatcher->create_discretizors(bag);
   DiscDisplay(discDispatcher);

   Mcout << "Binning via MDL heuristic for Uniform" << endl;
   discDispatcher->set_disc_type(DiscDispatch::binning);
   discDispatcher->set_bin_heuristic(DiscDispatch::MDL);
   discDispatcher->create_discretizors(bag);
   DiscDisplay(discDispatcher);

   Mcout << "7 bins for Uniform using set_disc" << endl;
   Array<int>* bins = new Array<int>(0, 1, 7);
   discDispatcher->set_disc_vect(bins);
   ASSERT(bins == NULL);
   discDispatcher->create_discretizors(bag);
   DiscDisplay(discDispatcher);   

   delete discDispatcher;
}


/***************************************************************************
  Description : 
  Comments    :
***************************************************************************/
void test_errors()
{
   InstanceList bag("t_DiscDispatch");
   DiscDispatch* discDispatcher = new DiscDispatch();   

   
   discDispatcher->create_discretizors(bag);

   InstanceList bag2("golf");

#ifndef MEMCHECK
   TEST_ERROR("DiscDispatch::discretize_bag: fatal error," ,
	      discDispatcher->create_discretizors(bag2));
#endif   

}







   
/***************************************************************************
  Description : Runs the tests
  Comments    :
***************************************************************************/
main()
{
   Mcout << "t_DiscDispatch executing ... " << endl;
   test_dispatch();
   Mcout << "testing complete." << endl;
   return 0;
}
   

