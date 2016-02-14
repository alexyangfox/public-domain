// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the RealDiscretizor and BinningRealDiscretizor.
  Doesn't test :
  Enhancements :
  History      : James Dougherty                                    11/20/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <InstList.h>
#include <BinningDisc.h>


RCSID("MLC++, $RCSfile: t_RealDiscretizor.c,v $ $Revision: 1.16 $")


/***************************************************************************
  Description : Fills the PtrArray<RealDiscretizor*> with
                  BinningRealDiscretizor* used to test the RealDiscretizor
		  and BinningRealDiscretizors.
  Comments    : 
***************************************************************************/
PtrArray<RealDiscretizor*>* test_creating_discretizors(const InstanceBag& bag,
						    int binCount)
{
   SchemaRC schema = bag.get_schema();
   PtrArray<RealDiscretizor*>* brd
      = new PtrArray<RealDiscretizor*>(schema.num_attr());
   PtrArray<RealDiscretizor*>* clones 
      = new PtrArray<RealDiscretizor*>(schema.num_attr());
   
   for(int k = 0; k < schema.num_attr(); k++){
      const AttrInfo& ai = schema.attr_info(k);
      if ( ai.can_cast_to_nominal() )
	 brd->index(k) = NULL;
      else if ( ai.can_cast_to_real()){
	 BinningRealDiscretizor* realDisc =
	    new BinningRealDiscretizor(k,binCount,schema);

#ifndef MEMCHECK
	 TEST_ERROR("BinningRealDiscretizor::BinningRealDiscretizor: "
	       "number of bins (-1) is negative",
		    BinningRealDiscretizor foo(k,-1,schema));
#endif
	 brd->index(k) = realDisc;
	 brd->index(k)->create_thresholds(bag);
	 RealDiscretizor* rdd =  brd->index(k);
	 rdd->set_description(rdd->get_description());
	 clones->index(k)
	    = new BinningRealDiscretizor((class BinningRealDiscretizor&)
					 *brd->index(k),
					 ctorDummy);
	 ASSERT(*clones->index(k) == *brd->index(k));
      }
      else 
	 err << "Unrecognized attribute type" << fatal_error;
   }
   delete clones;
   return brd;
}



/***************************************************************************
  Description : Tests that the RealDiscretizor and BinningRealDiscretizor
                  are working properly.
  Comments    :
***************************************************************************/
void test_real_discretizor()
{
   int fileNo = 0;
   const int numFiles = 2;
   MLCIStream  dataFileStream("t_RealDiscretizor-datasets.txt");
   Array<MString> dataFiles(numFiles);
   // Read in the names of the datasets that we are going to
   // discretize
   while(dataFileStream.get_stream().good() && fileNo < numFiles)
     dataFileStream >> dataFiles.index(fileNo++);
   dataFileStream.close();

   LogOptions logOptions;
   for(int i = 0; i < dataFiles.size(); i++) {
      Mcout << "*********************************************" << endl;
      Mcout << "Datafile: " << dataFiles.index(i) << endl;
      InstanceList bag(dataFiles.index(i));
      bag.display();
      Mcout << endl;

      //Create the Discretizors to be used in the process
      //(this must be done first)
      PtrArray<RealDiscretizor*>* brDisc
	 = create_binning_discretizors(logOptions, bag, 5); 
      //Create some for testing purposes
      #ifndef MEMCHECK
      test_creating_discretizors(bag,5);
      #endif

      Mcout << "------------Original Schema ----------------" << endl;
      bag.get_schema().display();

      // Create a new bag with the discretized attributes
      InstanceBag* newBag = discretize_bag(bag, brDisc);
      Mcout << "-------------- New Schema ------------------" << endl;
      (newBag->get_schema()).display();
      Mcout << "-----------Real Discretizors----------------" << endl;

      //Display each of the discretizors
      for(int k = 0; k < brDisc->size(); k++)
	 if(NULL != brDisc->index(k)){
	    brDisc->index(k)->display();


#ifndef MEMCHECK
    TEST_ERROR("RealDiscretizor::check_thresholds: method cannot be "
	       "called twice. ", brDisc->index(k)->create_thresholds(bag));
#endif
	 }
      
      Mcout << " ----------Discretized bag -----------------" << endl;
      newBag->display();

      delete  brDisc;
      delete newBag;
      Mcout << "*********************************************" << endl;
   }

   Mcout << "test_real_discretizor succesful on " << dataFiles.size()
	 << " files ... "<< endl;

}

/***************************************************************************
  Description : Tests the routine num_distinct_vals()
  Comments    :
***************************************************************************/
void test_num_distinct_vals()
{
   InstanceList bag("crx", ".names", ".all");
   int x = num_distinct_vals(bag, 1); //crx:attribute-1 (A2) has 3 
   ASSERT( 349 == x);
   
}


main()
{
   Mcout << "t_RealDiscretizor executing..." << endl;
//   test_num_distinct_vals();
   test_real_discretizor();
   Mcout << "t_RealDiscretizor succesful .." << endl;
   return 0;
}




