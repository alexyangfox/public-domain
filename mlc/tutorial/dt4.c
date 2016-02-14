// Tutorial program.  Output to XStream

#include <basics.h> 
#include <MLCStream.h> 
#include <ID3Inducer.h> 

main()
{
   ID3Inducer inducer("Xema");
   inducer.set_unknown_edges(FALSE);
   inducer.read_data("tutorial");
   inducer.train();

   MLCOStream out(XStream);             // Declare out to be XStream
   DotGraphPref dotPref;
   inducer.display_struct(out,dotPref);// Display the structure to the XStream.

   return 0;
}   
