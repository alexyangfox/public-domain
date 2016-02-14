// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test HOODGCIH.
  Doesn't test :
  Enhancements :
  History      : Ronny Kohavi                                       1/10/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <HOODGCIH.h>
#include <InstList.h>
#include <ProjBag.h>
#include <DestArray.h>

RCSID("MLC++, $RCSfile: t_HOODGCIH.c,v $ $Revision: 1.10 $")

const int attrNum = 0;
Bool disp = TRUE;
CIHMinDests cih; 

   
static void cover_instance(ProjInfoPtrList& pipl,
                           InstanceProjection& ip,
                           const NominalAttrInfo& deletedAttrInfo)
{
   static Bool tested_already = FALSE;

   // Just a convenience place to test since we can generate piwb
   if (!tested_already && !pipl.empty()) {
      tested_already = TRUE;
      TEST_ERROR("CoverInstanceHeuristic::init_instance_cover: NULL ",
                  cih.init_instance_cover(NULL));
      ProjInfoWithBag& piwb = *pipl(ProjInfoPtrPix(pipl, 1));
      cih.end_instance_cover();
      TEST_ERROR("is_better: init_instance_cover not called",
                 cih.is_better(piwb));
   }
      
   cih.init_instance_cover(&ip);

   if (disp) {
      Mcout << "\nCovering instance ";
      ip.display(deletedAttrInfo, Mcout);
   }
   ProjInfoPtrPix bestPix(pipl);
   int bestBagNum = -1;

   
   
   int bagNum = FIRST_CATEGORY_VAL;
   for (ProjInfoPtrPix pix(pipl,1); pix; ++pix, ++bagNum) {
      ProjInfoWithBag& piwb = *pipl(pix);
      if (cih.is_better(piwb)) {
         if (disp) 
	    Mcout << "Bag " << bagNum << " is best" << endl;
         bestBagNum = bagNum;
         bestPix = pix;
      }
   }

   if (!bestPix) {
      ProjInfoWithBag* piwb = new ProjInfoWithBag(ip);
      if (disp) {
         Mcout << "New (forced) projection.  Bag number is " <<
             FIRST_NOMINAL_VAL + pipl.length() << "\n   ";
         ip.display(deletedAttrInfo, Mcout);
      }
      pipl.append(piwb);
   }
   else {                       // cover it
      ProjInfoWithBag& piwb = *pipl(bestPix);
      if (DestArray::included_in_dest(ip.dest_bag(), piwb.dest_bag()))
         if (disp)
	    Mcout << "Projection included in bag "; else;
      else
         if (disp) 
	    Mcout << "Projection merged with bag "; else;

      if (disp)
	 Mcout << bestBagNum << " result is \n   ";
      piwb.merge_dests(ip.dest_bag(), ip.dest_counts());
      piwb.add_projection(ip);
      // Now display the merged projection info.
      if (disp) piwb.display(deletedAttrInfo, Mcout);
   }
}


void test(const InstanceList& TS, Bool stats)
{
   BagPtrArray* bpa = TS.split_by_label();
   if (disp) {
      for (int i = bpa->low(); i <= bpa->high(); i++)
         Mcout << "Bag " << i << '\n' << *(*bpa)[i] << endl;
   }
   
   const NominalAttrInfo &attrInfo = bpa->index(0)->attr_info(attrNum).
                                                        cast_to_nominal();

   // Two levels
   for (int level = 0; level < 2; level++) {
      cih.init_level_cover();
                                       
      ProjBag projBag(*bpa, attrNum);
      if (disp) 
	 Mcout << "Displaying ProjLIBag\n" << projBag;
      ProjInfoPtrList& pipl = *new ProjInfoPtrList;

      ProjListPix pix(projBag.projList,1);
      while (pix) {
         InstanceProjection& ip = *projBag.projList(pix);
         cover_instance(pipl, ip, projBag.deleted_attr_info());
         // Check the idempotency of end_instance_cover;
         if (level == 1) cih.end_instance_cover();
         projBag.del(pix, 1);
      }
      if (stats) {
         if (level == 2) cih.end_level_cover();
         cih.level_stats(Mcout);
         Mcout << "The same statistics are" << endl;
         cih.level_stats(Mcout);
      }

      // Clear pipl
      // @@ DblLinkList should provide clear_ptr(), not just free()
      // @@ (free being a bad name too)
      while (!pipl.empty())
	 delete pipl.remove_front(); 

      delete &pipl;
   }
   cih.total_stats(Mcout);
   delete bpa;
}
            
main()
{
   cout << "t_HOODGCIH executing" << endl;

   // we have to set the log level because this is a static object
   // that gets initialized before globalLogLevel is set.
   cih.set_log_level(globalLogLevel);

   InstanceList TS("t_HOODGCIH");  // golf
   test(TS, TRUE);
   disp = FALSE; 
   cih.init();
   test(TS, TRUE);  // should be exactly the same as the first
   cih.init();
   test(TS, FALSE);  // should be exactly the same as the first


   return 0; // return success to shell
}   
