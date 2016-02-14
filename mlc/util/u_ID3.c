// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a utility for MLC++
// It is not part of the MLC++ library itself, but instead uses the
//   library to provide a useful function.

/***************************************************************************
  Description  : Run ID3.
  Usage        : Environment variables to set:
                   DATAFILE
		   UNKNOWN_EDGES (create tree with UNKNOWN edges.
		                  default for ID3 is no).
		   DEBUG_ID3     (yes/no show debug information like mutual 
		                  info, number of instances.  Default: no).
                   DISPGRAPH     (if yes, displayes the graph. Default:no).
  Enhancements :
  History      : Ronny Kohavi                                       2/20/94
                   Made the tester a utility.
                 Richard Long                                       9/08/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <ID3Inducer.h>
#include <CtrInstList.h>
#include <CatTestResult.h>
#include <GetOption.h>
#include <FileNames.h>

RCSID("MLC++, $RCSfile: u_ID3.c,v $ $Revision: 1.18 $")

const MString dispGraphHelp = "This option determines whether or not to "
  "display the graph created by ID3.";


main()
{
   FileNames files;

   ID3Inducer inducer("ID3 Inducer");
   inducer.set_user_options("ID3_");
   inducer.set_unknown_edges(FALSE);
   
   inducer.read_data("", files.names_file(), files.data_file());
   inducer.train();
   Mcout << "Tree has " << inducer.num_nodes() << " nodes, and "
          << inducer.num_leaves() << " leaves." << endl;

   if(files.test_file()) {
      CatTestResult result(inducer.get_categorizer(),
			   inducer.instance_bag(),
			   "", files.test_file());
      Mcout << result << endl;
   }
   
   if (get_option_bool("DISPGRAPH", FALSE, dispGraphHelp, TRUE)) {
     DotGraphPref pref;
     MLCOStream out(XStream);
     inducer.display_struct(out,pref);
  }

// The following can better print the stuff when I actually need it.
//dot -Tps -Gpage="8.5,11" -Gsize="7.5,10" -Gratio=fill ID3.dot |lpr

   MLCOStream out("ID3.dot");
   DotGraphPref pref;
   inducer.display_struct(out,pref);

   
   return 0; // return success to shell
}   




