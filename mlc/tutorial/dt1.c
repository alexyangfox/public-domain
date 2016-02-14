// Tutorial program 1
// Run the ID3 algorithm on a test set and display the tree in ASCII.

#include <basics.h>     // Must be included first in any program using MLC++
#include <ID3Inducer.h> // Include ID3Inducer class declarations

main()
{
   ID3Inducer inducer("Xema");      // "inducer" is an object of ID3Inducer.
                                    // The argument is the "description."
   inducer.set_unknown_edges(FALSE);// Avoid edges for "unknown" attributes.
   inducer.read_data("tutorial");   // Read data files tutorial.names and  
                                    //   tutorial.data 
   inducer.train();                 // Produce categorizer
   inducer.display_struct();        // Display structure of categorizer

   return 0;                        // Return success to shell
}   
