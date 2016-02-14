// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : Computes order from user or based on entropy.
  Comments     :
  Enhancements :
  History      : Yeogirl Yun                                 July 15, 1995
                 Initial revision
***************************************************************************/

#include <basics.h>
#include <MEnum.h>
#include <AttrOrder.h>
#include <EntropyODGInducer.h>
#include <GetOption.h>
#include <BagSet.h>
#include <CtrInstList.h>

const MEnum orderTypeEnum =
  MEnum("sequential", AttrOrder::sequential) <<
  MEnum("user", AttrOrder::user) <<
  MEnum("mutualInfo", AttrOrder::mutualInfo);

const MString ORDER_TYPE_HELP =
  "Sets the option of what order scheme to use. sequential means from 0 to "
  "n-1 where n is the number of attributes. user means users set the order. "
  "mutualInfo means order is set based on mutual information.";



/***************************************************************************
  Description : Sets the order vector based on user options.
  Comments    : 
***************************************************************************/
void AttrOrder::set_order_from_user(const MString& /* prefix */,
				    const InstanceBag& bag)
{
   const SchemaRC& schema = bag.get_schema();
   int totalNumAttrs = schema.num_attr();

   DynamicArray<int> userOrder(0);
   BoolArray attr(0, totalNumAttrs, FALSE);

   Mcout << "Enter attribute.  End with -1" << endl;
   int attrNum;
   do {
      Mcin >> attrNum;
      if (attrNum >= 0  && attrNum < totalNumAttrs) {
	 if (attr[attrNum])
	    Mcout << "Duplicate attribute ignored." << endl;
         else {
	    MString name(schema.attr_info(attrNum).name());
	    Mcout << "Attribute " << attrNum << " is " << name << endl;
	    attr[attrNum] = TRUE;
	    userOrder[userOrder.size()] = attrNum;
	 }
      } else if (attrNum != -1)
	 Mcout << "Invalid attribute number.  Must be 0 to "
	       << totalNumAttrs - 1 << endl;
   } while (attrNum != -1);
      
   delete order;
   order = new Array<int>(userOrder, ctorDummy);
}


//  This is Dan's old code which requires filling in all attributes
//    and also requires the names, which usually makes it harder
//    to input, given that OFSS gives numbers.
//    const SchemaRC& schema = bag.get_schema();

//    // create an order vector of appropriate size
//    order = new Array<int>(schema.num_attr());
   
//    // create a map indicating which attributes have already been chosen
//    BoolArray map(0, schema.num_attr(), FALSE);

//    // ask for all but one attribute names from the user using options
//    for(int which=0; which<schema.num_attr() - 1; which++) {
//       MEnum orderEnum;

//       // set up for the option:  run through the map array, grab the
//       // first open entry, make that the default.  Add all open entries
//       // to the orderEnum with the correct numbers
//       int defaultVal = -1;
//       for(int i=map.low(); i<=map.high(); i++) {
// 	 if(!map[i] && defaultVal < 0)
// 	    defaultVal = i;
// 	 if(!map[i])
// 	    orderEnum << MEnum(schema.attr_name(i), i);
//       }

//       // get the option from the user.  Note that we're using
//       // get_option_enum templated on an int here!
//       (*order)[which] = get_option_enum(prefix + "ATTR_" +
// 					      MString(which+1, 0),
// 					orderEnum, defaultVal, "", FALSE);
//       map[(*order)[which]] = TRUE;
//    }

//    // set the last entry of the vector to the only map entry left
//    // (make sure there's only one)
//    ASSERT(map.num_false() == 1);
//    for(int i=map.low(); map[i]; i++) { }  // empty body intentional
//    (*order)[schema.num_attr()-1] = i;
// }



/***************************************************************************
  Description : Sets the order vector based on mutual info.
  Comments    : 
***************************************************************************/
void AttrOrder::set_order_from_minfo(const LogOptions& logOptions,
				     const InstanceBag& bag)
{
   EntropyODGInducer eodgInducer("dummy");
   eodgInducer.set_log_level(logOptions.get_log_level());
   eodgInducer.set_unknown_edges(TRUE); // to handle unknowns
   eodgInducer.set_post_proc(none);
   eodgInducer.set_cv_prune(TRUE);
   eodgInducer.set_user_options("AO_");
   CtrInstanceList listBag(bag.get_schema());
   for (Pix pix = bag.first(); pix; bag.next(pix))
      listBag.add_instance(bag.get_instance(pix));
   InstanceBag* bagPtr = &listBag;
   eodgInducer.assign_data(bagPtr);
   eodgInducer.train();
   order= new Array<int>(eodgInducer.get_order_vector(), ctorDummy);
   InstanceBag *newBag = eodgInducer.release_data();
   ASSERT(newBag == &listBag);
}



/*****************************************************************************
  Description : Copy. Helper function.
  Comments    :
*****************************************************************************/
void AttrOrder::copy(const AttrOrder& src)
{
   orderSet = src.orderSet;
   orderType = src.orderType;
   order = NULL;
   if (src.order)
      order = new Array<int>(*src.order, ctorDummy);
}


/*****************************************************************************
  Description : Deletes memory allocated.
  Comments    :
*****************************************************************************/
void AttrOrder::destroy()
{
   delete order;
   order = NULL;
}


/*****************************************************************************
  Description : Copy constructor.
  Comments    :
*****************************************************************************/
AttrOrder::AttrOrder(const AttrOrder& src, CtorDummy)
{
   copy(src);
}


/*****************************************************************************
  Description : Assignment operator.
  Comments    :
*****************************************************************************/
AttrOrder& AttrOrder::operator=(const AttrOrder& src)
{
   if (this != &src) {
      destroy();
      copy(src);
   }
   return *this;
}


/*****************************************************************************
  Description : Reset the data members.
  Comments    :
*****************************************************************************/
void AttrOrder::init()
{
   destroy();
   orderSet = FALSE;
}
   

/*****************************************************************************
  Description : Returns order array.
  Comments    :
*****************************************************************************/
const Array<int>& AttrOrder::get_order() const
{
   if (order == NULL)
      err << "AttrOrder::get_order() : order array is NULL " << fatal_error;

   return *order;
}


/*****************************************************************************
  Description : Sets the order array.
  Comments    :
*****************************************************************************/
void AttrOrder::set_order(const Array<int>& src)
{
   delete order;
   order = new Array<int>(src, ctorDummy);
   orderSet = TRUE;
}


/*****************************************************************************
  Description : Set user options for order type.
  Comments    :
*****************************************************************************/
void AttrOrder::set_user_options(const MString& preFix)
{
   set_order_type(get_option_enum(preFix + "ORDER_TYPE",
				  orderTypeEnum,
				  AttrOrder::mutualInfo,
				  ORDER_TYPE_HELP,
				  FALSE));
}


/*****************************************************************************
  Description : Computes order and returns order array.
  Comments    :
*****************************************************************************/
const Array<int>& AttrOrder::compute_order(const LogOptions& logOptions,
					   const InstanceBag& bag,
					   const MString preFix)
{
   if (orderSet)
      return *order;  // do nothing if somebody set order already.
   
   delete order;
   order = NULL;
   
   if (get_order_type() == sequential) {
      order = new Array<int>(bag.get_schema().num_attr());
      for (int i = 0; i < order->size(); i++)
	 (*order)[i] = i;
   }
   else if (get_order_type() == user) 
      set_order_from_user(preFix, bag);
   else if (get_order_type() == mutualInfo)
      set_order_from_minfo(logOptions, bag);
   else
      err << "AttrOrder::compute_order : unknown order type : " <<
	 get_order_type() << fatal_error;

   FLOG(1, "Attribute order is " << *order << endl);

   return *order;
}
      
