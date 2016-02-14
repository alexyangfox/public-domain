// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Reads monk1 and ctr_split using AttrEqCategorizer to
                   verify that it produces two bags with the given
		   attribute value and with all the others.
  Doesn't test : 
  Enhancements : 
  History      : Yeogirl Yun                                    3/2/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <AttrEqCat.h>
#include <CtrBag.h>
#include <CtrInstList.h>
#include <MLCStream.h>

/*
Head shape :   round, square, octagon
Body shape :   round, square, octagon
Is smiling :   yes, no
Holding    :   sword, balloon, flag
Jacket color:   red, yellow, green, blue
Has tie :   yes, no
*/

/* const int HeadShape = 1;
const int BodyShape = 2;
const int IsSmiling = 3;
const int Holding = 4;
const int HasTie = 6;
*/ 
const int JacketColor = 5;

// for the JacketColor.
const int Red = FIRST_NOMINAL_VAL;

/* 
const int Yellow = FIRST_NOMINAL_VAL + 1;
const int Green = FIRST_NOMINAL_VAL + 2;
const int Blue =  FIRST_NOMINAL_VAL + 3;
*/

void test_split()
{

   CtrInstanceList il("monk1");
   AttrEqCategorizer aec(il.get_schema(), JacketColor, Red,
			 "Attribute EQ Cat", FALSE);

   CtrBagPtrArray* ctrArray = il.ctr_split(aec);
   ASSERT(ctrArray->size() == 3); // 2 + 1(UNKNOWN_NOMINAL_VAL)

   for (int i = FIRST_CATEGORY_VAL; i <= FIRST_CATEGORY_VAL + 1; i++) {
      const CtrInstanceBag& ctrBag = *(*ctrArray)[i];
      for (Pix pix = ctrBag.first(); pix; ctrBag.next(pix)) {
	 const InstanceRC& inst = ctrBag.get_instance(pix);
	 const NominalAttrInfo& nai =
	    inst.attr_info(JacketColor).cast_to_nominal();
	 if (i == FIRST_CATEGORY_VAL) // should have all the attribute values
	                              // except for Red.
	    ASSERT(Red != nai.get_nominal_val(inst[JacketColor]));
	 else // should have all Red attribute values.
	    ASSERT(Red == nai.get_nominal_val(inst[JacketColor]));
      }
   }
   delete ctrArray;
}
	 
   

int main()
{
   Mcout << "testing t_AttrEqCat.c" << endl;

   test_split();

   Mcout << "Success!" << endl;
   return 0;
}
