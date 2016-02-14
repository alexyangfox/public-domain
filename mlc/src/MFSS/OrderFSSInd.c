// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Wrapper inducer for search for order over HOODGInducer.
  Assumptions  :
  Comments     : 
  Complexity   : Training is the number of states searched times the
                   estimation time per state.
  Enhancements :
  History      : Dan Sommerfield                                  (6/04/95)
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <OrderFSSInd.h>
#include <ListHOODGInd.h>
#include <COODGInducer.h>
#include <TableCasInd.h>
#include <ListODGInducer.h>
#include <CtrInstList.h>
#include <BFSearch.h>
#include <HCSearch.h>
#include <SASearch.h>

#include <env_inducer.h>
#include <GetOption.h>

RCSID("MLC++, $RCSfile: OrderFSSInd.c,v $ $Revision: 1.10 $")

const MEnum directionEnum =
  MEnum("forward", OrderFSSInducer::forward) <<
  MEnum("backward", OrderFSSInducer::backward);
const MString directionHelp = "This option chooses the direction in which to "
 "search.  Forward causes the search to begin with an empty subset of "
 "features, while backward causes the search to begin with a heuristic order.";
const OrderFSSInducer::Direction defaultDirection = OrderFSSInducer::forward;


MEnum innerEnum = MEnum("ListHOODG", OrderFSSInducer::listHOODG) <<
                  MEnum("COODG", OrderFSSInducer::COODG) <<
                  MEnum("table-cascade", OrderFSSInducer::tableCas) <<
                  MEnum("ListODG", OrderFSSInducer::listODG);
			
const MString innerHelp = "Specifies the inducer to run the ordering "
  "search over.  Choices are limited to inducers which make sense of "
  "different attribute orderings.";

OrderFSSInducer::OrderFSSInducer(const MString& description, 
   Inducer* ind)
   : SearchInducer(description, ind),
     direction(defaultDirection),
     innerInducer(ind)
{
   // must call create_global_info
   globalInfo = create_global_info();
}

void OrderFSSInducer::set_user_options(const MString& prefix)
{
   direction =
      get_option_enum(prefix + "DIRECTION", directionEnum,
		      defaultDirection,
		      directionHelp, TRUE);
   if (direction == backward)
      ao.set_user_options(prefix);

   if(innerInducer == NULL) {
      InnerType indType;
      get_option_enum(prefix + "INDUCER", innerEnum, innerHelp, FALSE,
		      indType);
      if(indType == OrderFSSInducer::listHOODG) {
	 ListHOODGInducer *lhi = new ListHOODGInducer("list-HOODG");
	 lhi->set_user_options(prefix);
	 innerInducer = lhi;
      } else if(indType == OrderFSSInducer::COODG) {
	 COODGInducer *ci = new COODGInducer("COODG");
	 ci->set_user_options(prefix);
	 innerInducer = ci;
      } else if(indType == OrderFSSInducer::tableCas) {
	 TableCasInd *tci = new TableCasInd("Table-Cascade");
	 tci->set_user_options(prefix);
	 innerInducer = tci;
      } else if(indType == OrderFSSInducer::listODG) {
	 ListODGInducer *loi = new ListODGInducer("ListODG", NULL);
	 loi->set_user_options(prefix);
	 innerInducer = loi;
      } else {
	 err << "OrderFSSInducer::set_user_options: illegal "
	    "type for inner inducer" << fatal_error;
      }
   }

   // set the baseInducer variable (of SearchInducer) before calling
   // SearchInducer's set_user_options so SearchInducer won't try
   // to set the induce itself
   baseInducer = innerInducer;
   SearchInducer::set_user_options(prefix);
}
   
Array<int>*
OrderFSSInducer::create_initial_info(InstanceBag* trainingSet)
{
   if (direction == forward)
      return new Array<int>(0);
   else if (direction == backward) {
      ASSERT(trainingSet != NULL);
      LogOptions lo;
      lo.set_log_options(get_log_options());
      lo.set_log_level(get_log_level()); // could be -1 here
      return new Array<int>(ao.compute_order(lo, *trainingSet), ctorDummy);
   } else {
      ASSERT(FALSE);
      return new Array<int>(0);
   }
} 


Categorizer *OrderFSSInducer::state_to_categorizer(
   const State<Array<int>, AccEstInfo>& state) const
{
   // Cast away constness:  this function is conceptually const, but
   // TS gets changed in the middle, then restored to normal
   OrderFSSInducer *thisNC = (OrderFSSInducer *)this;

   // set order for the inducer, then train
   has_global_info();
   Inducer& ind = globalInfo->inducer->cast_to_inducer();
   AttrOrder& ao = ind.get_attr_order_info();
   ao.set_order(state.get_info());

   // we need to cast the type of categorizer coming out of the
   // inducer so we can use the copy constructor properly
   ind.assign_data(thisNC->TS);
   ind.train();
   Categorizer *theCat = ind.get_categorizer().copy();
   thisNC->TS = ind.release_data();
   return theCat;
}



