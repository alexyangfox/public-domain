// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Create stubs, i.e., functions that don't do anything.
                 This is to allow partial links.
  Assumptions  :
  Comments     :
  Complexity   :
  Enhancements :
  History      : Ronny Kohavi                                       2/19/95
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <env_inducer.h>

RCSID("MLC++, $RCSfile: stubs.c,v $ $Revision: 1.2 $")

/***************************************************************************
  Description : search_inducers stubs
  Comments    :
***************************************************************************/

void setup_search_inducers(MEnum& /* envInducerEnum */)
{}
   
BaseInducer *search_inducers(const MString& /* prefix */,
			     InducerType /* inducerType */,
			     const MString& /* inducerName */)
{
   return NULL;
}



