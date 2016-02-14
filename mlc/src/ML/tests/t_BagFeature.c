// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the feature construction functions. 
  Doesn't test :
  Enhancements :
  History      : Svetlozar Nestorov                               5/5/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <BagSet.h>
#include <InstList.h>

RCSID("MLC++, $RCSfile: t_BagFeature.c,v $ $Revision: 1.10 $")


/***************************************************************************
  Description : Used in the testing of the feature construction.
  Comments    : The address of the this function is passed as a
                  parameter to change_feature(). 
***************************************************************************/
static AttrValue_ cross_product(const AttrValue_& val1, const AttrInfo& ai1,
				const AttrValue_& val2, const AttrInfo& ai2,
				const AttrInfo& newai)
{
   int intResult  = (ai1.get_nominal_val(val1) - FIRST_NOMINAL_VAL) *
                    (ai2.cast_to_nominal().num_values())
	            + ai2.get_nominal_val(val2);
   AttrValue_ result;
   newai.set_nominal_val(result, intResult);
   return result;
}

// These two parameters should not be changed since the .out file
//   depends on them and for different values the diff between .out
//   and .exp will fail. 
const int ATTR_NUM1 = 0;
const int ATTR_NUM2 = 3;


main()
{
   Mcout << "t_BagFeature.c executing" << endl;
   InstanceList bag("t_BagFeature");
   InstanceBag *featureBag = bag.copy_with_blank_feature();
   Mcout << *featureBag;
   featureBag->display_names(Mcout, TRUE, "Names file for t_BagFeature");
   featureBag->change_feature(ATTR_NUM1, ATTR_NUM2, " * ", &cross_product);
   Mcout << *featureBag;
   delete featureBag;
   return 0; // return success to shell
}   
