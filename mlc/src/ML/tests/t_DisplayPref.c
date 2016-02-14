// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : The methods for DisplayPref are very straightforward,
                   mostly getting and setting preferences.  Thus, this
		   module is very straightforward--tests to see if the
		   right values are set.
		 Also, the casts are checked to make sure appropriate
		   error messages are returned if the cast is illegal.
  Doesn't test : 
  Enhancements : 
  History      : Dave Manley                                       9/26/93
                   Initial revision
***************************************************************************/
#include <DisplayPref.h>
#include <basics.h>
#include <errorUnless.h>

RCSID("MLC++, $RCSfile: t_DisplayPref.c,v $ $Revision: 1.11 $")

main ()
{

   cout << "testing t_DisplayPref.c" << endl;
   
   FloatPair v(2.7, 1.3);
   
   DotPostscriptPref dpp;

   FloatPair retval = dpp.get_page_size();
   
   if (retval.x != defaultPageX || retval.y != defaultPageY)
      err << "t_DisplayPref.c: page size of: "
	  << retval.x << "," << retval.y 
	  << " does not match initialization values."
	  << fatal_error;

   if (dpp.get_graph_size().x != defaultGraphX ||
       dpp.get_graph_size().y != defaultGraphY)
      err << "t_DisplayPref.c: graph size of: "
	  << dpp.get_graph_size().x << "," << dpp.get_graph_size().y 
	  << " does not match initialization values."
	  << fatal_error;

   if (dpp.get_orientation() != DotPostscriptPref::DisplayLandscape)
      err << "t_DisplayPref.c: orientation value of: " 
	 << dpp.get_orientation()
	 << " does not match initialization value of: Landscape"
	 << fatal_error;

   
   dpp.set_graph_size(v);
   dpp.set_page_size(v);
   dpp.set_orientation(DotPostscriptPref::DisplayPortrait);
   retval = dpp.get_graph_size();
   
   if (retval.x != (float)2.7 || retval.y != (float)1.3)
      err << "t_DisplayPref.c: graph size of: "
	  << dpp.get_graph_size().x << "," << dpp.get_graph_size().y 
	  << " does not match set values of: 2.7,1.3"
	  << fatal_error;

   
   if (dpp.get_page_size().x != (float)2.7 ||
       dpp.get_page_size().y != (float)1.3)
      err << "t_DisplayPref.c: page size of: "
	  << dpp.get_page_size().x << "," << dpp.get_page_size().y 
	  << " does not match set values of: 2.7,1.3"
	  << fatal_error;
   	 
   FloatPair v2(2.7, -1.3);   
   TEST_ERROR("DotPostscriptPref::set_page_size: illegal page size",
	      dpp.set_page_size(v2));

   ASSERT(dpp.preference_type() ==  DisplayPref::DotPostscriptDisplay);
   ASSERT(dpp.typecast_to_DotPostscript().preference_type() ==
	  DisplayPref::DotPostscriptDisplay);

   #ifndef MEMCHECK
   TEST_ERROR("Attempt to cast DisplayPref to",
	      dpp.typecast_to_DotGraph().preference_type());
   #endif

   
   ASCIIPref ap;
   ASSERT(ap.preference_type() ==  DisplayPref::ASCIIDisplay);
   ASSERT(ap.typecast_to_ASCII().preference_type()==DisplayPref::ASCIIDisplay);

   #ifndef MEMCHECK
   TEST_ERROR("Attempt to cast DisplayPref to",
	      dpp.typecast_to_DotGraph().preference_type());
   #endif

   DotGraphPref dgp;
   ASSERT(dgp.preference_type() ==  DisplayPref::DotGraphDisplay);
   ASSERT(dgp.typecast_to_DotGraph().preference_type() ==
	  DisplayPref::DotGraphDisplay);

   #ifndef MEMCHECK
   TEST_ERROR("Attempt to cast DisplayPref to",
	      dgp.typecast_to_ASCII().preference_type());
   #endif
	
   return(0);
}




