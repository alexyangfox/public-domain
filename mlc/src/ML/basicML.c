// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/***************************************************************************
  Description  : Global stuff for the machine learning subdirectory
  Assumptions  :
  Comments     :
  Complexity   :
  Enhancements :
  History      : Ronny Kohavi                                       1/8/95
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <InstanceHash.h>

RCSID("MLC++, $RCSfile: basicML.c,v $ $Revision: 1.3 $")

const Real confidenceIntervalProbability = 0.95;
const Real confidenceIntervalZ = 1.96;

const Real
   HashTable<InstanceBagSameAttr_,InstanceBagSameAttr_>::LOAD_FACTOR = 0.5;
SET_DLLPIX_CLEAR(InstanceBagSameAttr_,NULL);
