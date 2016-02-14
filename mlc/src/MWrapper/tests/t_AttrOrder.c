// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : Tests AttrOrder methods.
  Comments     :
  Enhancements :
  History      : Yeogirl Yun                                  July 15, 95
                   Initial revision 
***************************************************************************/

// vote should have the following 3, 10, 11 at mutual info order type.

#include <AttrOrder.h>
#include <BagSet.h>
#include <CtrInstList.h>
#include <MOption.h>

const int MINFO_ATTR_1 = 3;


/*****************************************************************************
  Description : Checks the array size.
  Comments    :
*****************************************************************************/
void test_array_size(AttrOrder& ao,
		     const InstanceBag& bag)
{
   LogOptions lo;
   const Array<int>& array = ao.compute_order(lo, bag, "");

   if (array.size() != bag.get_schema().num_attr())
      err << "test_array_size: array size is not correct "
	 " got " << array.size() << " but should be " <<
	 bag.get_schema().num_attr() <<fatal_error;
}

/*****************************************************************************
  Description : Tests sequential option.
  Comments    :
*****************************************************************************/
void test_sequential_option(AttrOrder& ao,
			    const InstanceBag& bag)
{
   ao.set_order_type(AttrOrder::sequential);   
   test_array_size(ao, bag);

   const Array<int>& array = ao.get_order();
   for (int i = 0; i < array.size(); i++)
      if (array[i] != i)
	 err << "test_sequential_option: array contents are not "
	    "sequential. " << fatal_error;
}


/*****************************************************************************
  Description : Tests user option. Read from t_AttrOrder.in
  Comments    :
*****************************************************************************/
void test_user_option(AttrOrder& ao,
		      const InstanceBag& bag)
{
   MOption::set_prompt_level(MOption::promptBasic);

   ao.set_order_type(AttrOrder::user);
   LogOptions lo;
   const Array<int>& array = ao.compute_order(lo, bag, "");
   for (int i = 0; i < array.size(); i++)
      if (array[i] != i)
	 err << "test_user_option: array contents are different from what "
	    "user set" << fatal_error;

   MOption::set_prompt_level(MOption::requiredOnly);   
}   
   


/*****************************************************************************
  Description : test mutual info opiton.
  Comments    :
*****************************************************************************/
void test_minfo_option(AttrOrder& ao,
		       const InstanceBag& bag)
{
   LogOptions lo;
   ao.set_order_type(AttrOrder::mutualInfo);
   const Array<int>& array = ao.compute_order(lo, bag);

   if (array[0] != MINFO_ATTR_1 || array.size() != 1) {
      Mcerr << "User array is " << array << endl;
      err << "test_user_option: array contents are different from what "
	 "is expected" << fatal_error;
   }
      
   
}   


int main()
{
   AttrOrder ao;
   CtrInstanceList bag("vote", ".names", ".data");

   test_sequential_option(ao, bag);
   test_user_option(ao, bag);
   test_minfo_option(ao, bag);

   return 0;
}


