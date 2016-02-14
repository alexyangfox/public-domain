// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The AFState class encaptulates the necessary state
                   information for the state space of some abstract
		   parameter setting.
  Assumptions  :
  Comments     :
  Complexity   : 
  Enhancements :
  History      : Dan Sommerfield                                      5/21/95
                   Refit into new search engine classes.
                 Ron Kohavi                                           9/18/94
                   Based on Brian Frasca's BFFSSState.  
***************************************************************************/

#include <basics.h>
#include <C45APState.h>
#include <string.h>
#include <C45Inducer.h>

/***************************************************************************
  Description : Basic functions for Program flags strings.
                You must call set_flag for each number from 0 to numFlags-1
		  exactly once.
		set_flag gets ownership of the flags strings in set_flag.
  Comments    :
***************************************************************************/
void PgmFlagsStrings::OK() const
{
   for (int i = 0; i < size(); i++)
      if (index(i) == NULL)
	 err << "PgmFlagsStrings::OK(): flag number " << i << " not set"
	     << fatal_error; 
}

PgmFlagsStrings::PgmFlagsStrings(int numFlags) 
   : PtrArray<Array<MString>*>(numFlags)
{}

PgmFlagsStrings::~PgmFlagsStrings() 
{
   DBG(OK());
}

void PgmFlagsStrings::set_flag(int num, Array<MString>* flagVals)
{
   if (flagVals->size() == 0)
      err << "PgmFlagsStrings::set_flag: size: " << flagVals->size()
	  << " must be > 0" << fatal_error;
   
   if (index(num) != NULL)
      err << "PgmFlagsStrings::setting same flag parameter: " << num
	 << "twice" << fatal_error;
   
   index(num) = flagVals;
}

/***************************************************************************
  Description : Returns a set of flags appropriate for a first state in
                  the search
  Comments    : static function
***************************************************************************/
Array<int>* C45APInfo::init_flags()
{
   Array<int>* flags = new Array<int>(4);
   flags->index(0) = 1;   /* -m2 */
   flags->index(1) = 8;   /* -c25% */
   flags->index(2) = 0;   /* no -g */
   flags->index(3) = 0;   /* no -s */

   return flags;
}

/***************************************************************************
  Description : Constructor.  Translates possible values for the flags in
                  the c4.5 program into a PgmFlagsStrings structure.
  Comments    :
***************************************************************************/
C45APInfo::C45APInfo()
   : flagStrings(4),
     varyArray(0, 4, TRUE),
     initFlags(4)
{
   // set initial flags
   Array<int> *fillInit = init_flags();
   ASSERT(fillInit->size() == initFlags.size());
   for(int i=0; i<fillInit->size(); i++)
      initFlags.index(i) = fillInit->index(i);
   
   Array<MString>&  mFlag = *new Array<MString>(17);
   mFlag[0] = "-m1";
   mFlag[1] = "-m2";
   mFlag[2] = "-m3";
   mFlag[3] = "-m4";
   mFlag[4] = "-m5";
   mFlag[5] = "-m10";
   mFlag[6] = "-m15";
   mFlag[7] = "-m20";
   mFlag[8] = "-m25";
   mFlag[9] = "-m30";
   mFlag[10] = "-m40";
   mFlag[11] = "-m50";
   mFlag[12] = "-m60";
   mFlag[13] = "-m70";
   mFlag[14] = "-m80";
   mFlag[15] = "-m90";
   mFlag[16] = "-m100";
   flagStrings.set_flag(0, &mFlag);

   Array<MString>& cFlag = *new Array<MString>(17);
   cFlag[0] = "-c1";
   cFlag[1] = "-c2";
   cFlag[2] = "-c3";
   cFlag[3] = "-c4";
   cFlag[4] = "-c5";
   cFlag[5] = "-c10";
   cFlag[6] = "-c15";
   cFlag[7] = "-c20";
   cFlag[8] = "-c25";
   cFlag[9] = "-c30";
   cFlag[10] = "-c40";
   cFlag[11] = "-c50";
   cFlag[12] = "-c60";
   cFlag[13] = "-c70";
   cFlag[14] = "-c80";
   cFlag[15] = "-c90";
   cFlag[16] = "-c100";
   flagStrings.set_flag(1, &cFlag);

   Array<MString>& gFlag = *new Array<MString>(2);
   gFlag[0] = "";
   gFlag[1] = "-g";
   flagStrings.set_flag(2, &gFlag);

   Array<MString>& sFlag = *new Array<MString>(2);
   sFlag[0] = "";
   sFlag[1] = "-s";
   flagStrings.set_flag(3, &sFlag);
}

/***************************************************************************
  Description : Displays the value of the local info of a state in terms
                  of the global info (this class).
  Comments    :
***************************************************************************/
void C45APInfo::display_values(const Array<int>& values, MLCOStream& out) const
{
   out << get_c45_flags(values);
}

/***************************************************************************
  Description : Returns a string describing the c4.5 program flags as
                  dictated by the local info of a state (pgmFlags).
  Comments    :
***************************************************************************/
MString C45APInfo::get_c45_flags(const Array<int>& pgmFlags) const
{
   MString flags = "";
   for (int i = 0; i < pgmFlags.size(); i++) {
      // Explanation: go to the i'th cell in flagStrings, which gives
      //   the array of strings.  Index that array by the flag index stored
      //   inf pgmFlags at the i'th position.
      if(i > 0 && (*flagStrings[i])[pgmFlags[i]] != "")
	 flags += " ";
      flags += (*flagStrings[i])[pgmFlags[i]];      
   }

   return flags;
}

/***************************************************************************
  Description : This function is called by the search engine immediately
                  before evaluation.  Here, we set the flags on the
		  C45 Inducer so that they will be used during the
		  evaluation process.
  Comments    :
***************************************************************************/
void C45APState::pre_eval(AccEstInfo *globalInfo)
{
   // case globaslInfo to C45APInfo
   ASSERT(globalInfo->class_id() == C45AP_INFO);
   C45APInfo *apSearchInfo = (C45APInfo *)globalInfo;

   MString c45flags = apSearchInfo->get_c45_flags(get_info());
   LOG(3, "using flags " << c45flags << endl);

   // set the flags.  Cast the base inducer stored in the global info
   // into C45Inducer then check class_id() to make sure the cast was
   // safe.
   C45Inducer *c45Inducer = (C45Inducer *)(globalInfo->inducer);
   ASSERT(c45Inducer->class_id() == C45_INDUCER);
   c45Inducer->set_pgm_flags(c45flags + " " + C45Inducer::defaultPgmFlags);
}

/***************************************************************************
  Description : Displays node number and identifies which flags are
                  active in the current state.
  Comments    :
***************************************************************************/
void C45APState::display_info(MLCOStream& stream) const
{
   static C45APInfo apSearchInfo;

   // use the static global info from this file to display the
   // flags, largely because the passed in globalInfo is often NULL
   // (not generally known at the time)
   if(get_eval_num() > NOT_EVALUATED)
      stream << "#" << get_eval_num();
   else
      stream << "#?";

   stream << " [";
   apSearchInfo.display_values(get_info(), stream);
   stream << "]";
}

DEF_DISPLAY(C45APState);
