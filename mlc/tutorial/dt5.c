// Tutorial program: log-level

#include <basics.h>   
#include <ID3Inducer.h>

main()
{
   ID3Inducer inducer("Xema");
   inducer.set_unknown_edges(FALSE);
   inducer.read_data("tutorial");

   inducer.set_log_level(2); // set log-level to 2

   inducer.train();

   return 0;
}   
