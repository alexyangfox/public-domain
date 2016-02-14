// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test ProjBag
  Doesn't test : ProjBag::del and many other things.
  Enhancements :
  History      : Ronny Kohavi                                       9/23/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <InstList.h>
#include <ProjBag.h>
#include <AttrCat.h>

RCSID("MLC++, $RCSfile: t_ProjBag.c,v $ $Revision: 1.8 $")


/***************************************************************************
  Description : Test the LIB constructor for ProjBag
  Comments    : 
***************************************************************************/
static void check_proj1(InstanceList& list)
{
   MLCOStream file("t_ProjBag.out1");

   file << "Bags by label" << endl;
   BagPtrArray& bpa = *list.split_by_label();
   for (int i = bpa.low(); i <= bpa.high(); i++) {
      file << "Bag " << i << endl;
      bpa[i]->display(file);
   }

   file << "\nProjection on outlook" << endl;
   ProjBag pBag(bpa, 0); pBag.OK();
   pBag.display(file);

   file <<  "\nProjection on windy" << endl;
   ProjBag pBag2(bpa, 2); pBag2.OK();
   pBag2.display(file);

   delete &bpa;

}

static void check_proj2(InstanceList& list)
{
   MLCOStream file("t_ProjBag.out1", FileStream, TRUE);
   AttrCategorizer ac(list.get_schema(), 2, "AttrCat on windy");
   BagPtrArray& bpa = *list.split(ac);
   file << "\nNow splitting on windy.  List are:" << endl;
   for (int i = bpa.low(); i <= bpa.high(); i++) {
      file << "Bag " << i << endl;
      bpa[i]->display(file);
   }

   ProjBag pBag(bpa, 0); pBag.OK();
   pBag.display(file);
   delete &bpa;
}


main()
{
   cout << "t_ProjBag executing" << endl;

   InstanceList list("t_ProjBag");

   check_proj1(list);
   check_proj2(list);
   
   return 0; // return success to shell
}   
