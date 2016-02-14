// Tutorial program: preferences

#include <basics.h> 
#include <MLCStream.h>
#include <ID3Inducer.h> 

main()
{
   ID3Inducer inducer("Xema");
   inducer.set_unknown_edges(FALSE);
   inducer.read_data("tutorial");
   inducer.train(); 

   MLCOStream psOut("dt3.ps");
   DotPostscriptPref psPref;                // Declare Dot preference
   // Default orientation is LandScape.  Change to portrait
   psPref.set_orientation(DotPostscriptPref::DisplayPortrait);
   inducer.display_struct(psOut,psPref);    // Display to file using given
                                            //    preferences

   MLCOStream dotOut("dt3.dot");            // Now generate dot output
   DotGraphPref dotPref;
   inducer.display_struct(dotOut, dotPref); 

   return 0;
}   
