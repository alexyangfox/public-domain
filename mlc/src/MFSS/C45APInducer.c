// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Wrapper inducer for automatic parameter selection in C4.5
  Assumptions  :
  Comments     : 
  Complexity   : Training is the number of states searched times the
                   estimation time per state.
  Enhancements :
  History      : Dan Sommerfield                                     6/10/95
                   Added a .c file
***************************************************************************/

#include <basics.h>
#include <C45APInducer.h>
#include <GetOption.h>

const MString varyMHelp = "Determines whether the search should vary the m"
  "flag to C4.5.  The m flag specifies the minimum weight for the split.";
const MString varyCHelp = "Determines whether the search should vary the c"
  "flag to C4.5.  The c flag specifies the certainty factor for pruning.";
const MString varyGHelp = "Determines whether the search should vary the g"
  "flag to C4.5.  The g flag specifies use of gain ratio vs information gain.";
const MString varySHelp = "Determines whether the search should vary the s"
  "flag to C4.5.  The s flag controls grouping/sub-grouping.";

/***************************************************************************
  Description : Get extra options from the user
  Comments    :
***************************************************************************/
void C45APInducer::set_user_options(const MString& prefix)
{
   SearchInducer::set_user_options(prefix);

   // determine which flags to modify
   Bool varyM = get_option_bool(prefix + "VARY_M", TRUE, varyMHelp, TRUE);
   Bool varyC = get_option_bool(prefix + "VARY_C", TRUE, varyCHelp, TRUE);
   Bool varyG = get_option_bool(prefix + "VARY_G", TRUE, varyGHelp, TRUE);
   Bool varyS = get_option_bool(prefix + "VARY_S", TRUE, varySHelp, TRUE);

   // set modification flags
   has_global_info();
   ASSERT(globalInfo->class_id() == C45AP_INFO);
   C45APInfo *info = (C45APInfo *)globalInfo;
   info->set_vary(0, varyM);
   info->set_vary(1, varyC);
   info->set_vary(2, varyG);
   info->set_vary(3, varyS);
}
