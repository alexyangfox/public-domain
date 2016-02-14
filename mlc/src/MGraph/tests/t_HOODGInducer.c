// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test HOODGInducer
  Doesn't test :
  Enhancements : This file is temporarily stored because it is the
                   version used for ECML conference.
  History      : Ronny Kohavi                                       9/27/93
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <HOODGInducer.h>
#include <CatTestResult.h>
#include <InstList.h>

RCSID("MLC++, $RCSfile: t_HOODGInducer.c,v $ $Revision: 1.15 $")

#ifdef __OBJECTCENTER__
   MString datafile = "lenses";
#else
   MString datafile = "monk1-full";
#endif

main()
{
   cout << "t_HOODGInducer executing" << endl;
      
   HOODGInducer hoodg("test hoodg");

   InstanceBag *TS = new InstanceList(datafile);
   hoodg.assign_data(TS);
   cout << "Starting train" << endl;
   hoodg.train();
   cout << "End train" << endl;

   CatTestResult result(hoodg.get_categorizer(),
			hoodg.instance_bag().
			cast_to_instance_list(),
			datafile);
   Mcout << "HOODG Inducer Results" << endl << result;

   MLCOStream out1("t_HOODGInducer.out1");
   hoodg.display_struct(out1);

   MLCOStream out("t_HOODGInducer.out2");
   DotGraphPref pref;
   hoodg.display_struct(out,pref);

   return 0; // return success to shell
}   
