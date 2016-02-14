// Tutorial program: output to a file

#include <basics.h> 
#include <ID3Inducer.h> 

const MString treeFile("tree.txt"); // Use MLC++ string class.

main()
{
   Mcout << "The tree will be stored in " << treeFile << endl;
   ID3Inducer inducer("Xema");
   inducer.set_unknown_edges(FALSE);
   inducer.read_data("tutorial");
   inducer.train(); 

   MLCOStream outfile(treeFile);
   outfile << "The tree produced by ID3 on the tutorial dataset:" << endl;
   inducer.display_struct(outfile);// Write structure to the file
                                   // instead of the standard output.

   return 0;
}   
