// Tutorial program: read and print dataset

#include <basics.h>   
#include <InstList.h>  // Required for InstanceList

main()
{
   // The default extensions are ".names" and ".data."  They may be
   //   given as the second and third arguments.
   InstanceList list("tutorial");

   Mcout << "Easy printing:\n" << list << endl;

   Mcout << "Same display but manually:" << endl;
   for (Pix pix = list.first(); pix; list.next(pix)) {
      const InstanceRC& instance = list.get_instance(pix);
      Mcout << instance;
   }

   return 0;
}
