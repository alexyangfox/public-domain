// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : This function returns a pointer to an allocated inducer
                   according to the give enumerated value.  The intent
		   is to chain this to env_inducer.
  Assumptions  : We return NULL if we don't support any of the enumerated
                   values.
  Comments     : One reason to split this from env_inducer is to allow
                   a link without FSS which is huge.
  Complexity   :
  Enhancements : 
  History      : Ronny Kohavi                                       2/19/95
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <env_inducer.h>
#include <FSSInducer.h>
#include <DiscSearchInd.h>
#include <OrderFSSInd.h>
#include <C45APInducer.h>
#include <TableCasInd.h>
#include <WeightSearchInd.h>

RCSID("MLC++, $RCSfile: search_ind.c,v $ $Revision: 1.12 $")


/***************************************************************************
  Description : Setup extra enumerations we support
  Comments    :
***************************************************************************/

void setup_search_inducers(MEnum& envInducerEnum)
{
  envInducerEnum = envInducerEnum << 
    MEnum("FSS", fss) <<
    MEnum("disc-search", discSearch) <<
    MEnum("order-FSS", orderFSS) <<
    MEnum("C4.5-auto-parm", c45ap) <<
    MEnum("table-cascaded", tableCas) <<
    MEnum("weight-search", WeightSearch);
}
   
/***************************************************************************
  Description : Create an inducer from the given inducerType.
                Return NULL if none of the inducers are recognized.
  Comments    :
***************************************************************************/

BaseInducer *search_inducers(const MString& prefix, InducerType inducerType,
			  const MString& inducerName)
{
   if (inducerType == fss) {
      FSSInducer *inducer = new FSSInducer(inducerName);
      inducer->set_user_options(prefix + "FSS_");
      return inducer;
   } else if (inducerType == discSearch) {
      DiscSearchInducer *inducer = new DiscSearchInducer(inducerName);
      inducer->set_user_options(prefix + "DISC_");
      return inducer;
   } else if (inducerType == orderFSS) {
      OrderFSSInducer *inducer = new OrderFSSInducer(inducerName);
      inducer->set_user_options(prefix + "OFSS_");
      return inducer;
   } else if (inducerType == c45ap) {
      C45APInducer *inducer = new C45APInducer(inducerName);
      inducer->set_user_options(prefix + "AP_");
      return inducer;
   } else if (inducerType == tableCas) {
      TableCasInd *inducer = new TableCasInd(inducerName);
      inducer->set_user_options(prefix);
      return inducer;
   } else if (inducerType == WeightSearch) {
      WeightSearchInducer *inducer = new WeightSearchInducer(inducerName);
      inducer->set_user_options(prefix + "WEIGHT_");
      return inducer;
   } else
      return NULL;
}

