// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/***************************************************************************
  Description  : Generate template files
***************************************************************************/

#include <basics.h>
#include <InstanceHash.h>
#include <CtrInstList.h>
#include <ConstInducer.h>
#include <CatTestResult.h>
#include <AttrCat.h>
#include <BadCat.h>
#include <NullInducer.h>
#include <ThresholdCat.h>
#include <entropy.h>
#include <DynamicArray.h>
#include <PartialOrder.h>

// Display using the call and then using "<<" so both will be generated
//   immediately.  If we just us "<<" we get a second phase of display()
//   template instantiations
#define DISP(str, var) \
   Mcout << str << " "; var.display(Mcout); \
   Mcout << endl << var << endl;


static void dllMString()
{
   DblLinkList<MString> dbl1;
   dbl1.append(MString("ronny"));
   DISP("dblLinkList MString", dbl1);
}

static void bags()
{
   PtrArray<InstanceBag*> foo(2);
   PtrArray<InstanceBag*> foo2(1,2,3);
   DblLinkList<AttrInfoPtr> aip;
   (void)(foo[2] == foo[1]);
   
}


TEMPL_GENERATOR(MLTemplates) 
{
   Array<edge_struct*> aes(2);
   Array<AugCategory*> aac(2);
   Array<RealAttrValue_> ara(2);
   Array<RealAttrValue_> ara2(ara,ctorDummy);
   DynamicArray<RealAttrValue_> dara(2);
   DynamicArray<RealAttrValue_> dara2(dara,ctorDummy);
   (void)dara.index(1);
   DynamicArray<attrLabelPair> alp(2);
   (void)alp.index(0);
   DynamicArray<InstanceBag*> daib(2);
   ASSERT(daib[2] == NULL);
   daib.truncate(2);
   daib.append(daib);
   PtrArray<Categorizer*> pac(2);
   PtrArray<Categorizer*> pac2(2,1);

   ASSERT(pac == pac2);
   LogOptions opt;
   // Instantiate the following, which causes problems in MTrans
   //   versus MWrapper duplicate generation.
   PtrArray<CtrInstanceBag*>  foo(1);
   DynamicArray<const CtrInstanceBag*> foo2(1);
   foo2 = foo2;
   ASSERT(foo2[2] == NULL);
   
   CtrInstanceList cil("cil");
   Mcout << cil << endl;
   Mcout << (InstanceBag&)cil << endl;
   ConstInducer ci("ci");
   AttrCategorizer ac(cil.get_schema(), 0, "ac");
   NullInducer ni("ni");
   ThresholdCategorizer tc(cil.get_schema(), 0, 0.0, "tc");

   Real threshold;
   Real minCondEntropy;
   real_cond_entropy(opt, cil, 0, threshold, minCondEntropy, 2);
   mutual_info(opt, cil, 0);

   dllMString();
   bags();

   CatTestResult result(badCategorizer, cil, cil);
   Mcout << result << endl;
   
   return 0; // return success to shell
}   
