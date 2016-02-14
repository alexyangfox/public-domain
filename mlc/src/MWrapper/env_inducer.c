// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : This function returns a pointer to an allocated inducer
                   according to the environment variable given
                   (defaults to INDUCER).
                 The name of the inducer will be INDUCERNAME if defined,
                   or INDUCER if not.
  Assumptions  : 
  Comments     : Abort if the environment variable INDUCER is not defined.
                 We do not return a pointer to a static inducer because
                   the caller may want to set options to different
                   instances of this inducer.
                 Each inducer may have an environment variable to set
                    the log level.
  Complexity   :
  Enhancements : Support options log for all inducers.
  History      : Ronny Kohavi                                       1/22/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <env_inducer.h>
#include <BaseInducer.h>
#include <ConstInducer.h>
#include <ID3Inducer.h>
#include <HOODGInducer.h>
#include <TableInducer.h>
#include <NullInducer.h>
#include <IBInducer.h>
#include <C45Inducer.h>
#include <C45RInducer.h>
#include <NaiveBayesInd.h>
#include <OneRInducer.h>
#include <DFInducer.h>
#include <AccEstInducer.h>
#include <EntropyODGInducer.h>
#include <LazyDTInducer.h>
#include <BaggingInd.h>
#include <PeblsInducer.h>
#include <AhaIBInducer.h>
#include <PtronInducer.h>
#include <WinnowInducer.h>
#include <OC1Inducer.h>
#include <CatDTInducer.h>
#include <CFInducer.h>
#include <ListHOODGInd.h>
#include <CN2Inducer.h>
#include <COODGInducer.h>
#include <ListODGInducer.h>

RCSID("MLC++, $RCSfile: env_inducer.c,v $ $Revision: 1.58 $")

static MEnum envInducerEnum =
  MEnum("null", null) << MEnum("ID3", id3) << MEnum("HOODG", hoodg) <<
  MEnum("const", constInducer) << MEnum("table-majority", tablemaj) <<
  MEnum("table-no-majority", tablenomaj) <<
  MEnum("IB", ib) << 
  MEnum("c4.5", c45prune) << 
  MEnum("c4.5-no-pruning", c45noprune) <<
  MEnum("c4.5-rules", c45Rules) << 
  MEnum("naive-bayes", naiveBayes) <<
  MEnum("accuracy-estimator", accEst) << MEnum("oneR", oneR) <<
  MEnum("disc-filter", dfInducer) << MEnum("EODG", eodg) <<
  MEnum("lazyDT", lazyDT) <<
  MEnum("bagging", baggingInd) <<
  MEnum("pebls", peblsInd) <<
  MEnum("aha-ib", ahaIB) <<
  MEnum("perceptron", ptronInd) <<
  MEnum("winnow", winnowInd) <<
  MEnum("disc-naive-bayes", discNaiveBayes) <<
  MEnum("oc1", oc1) <<
  MEnum("CatDT", catDt) <<
  MEnum("cont-filter", cfInducer) <<
  MEnum("list-hoodg", listHOODG) <<
  MEnum("cn2", cn2) <<
  MEnum("COODG", COODG) <<
  MEnum("ListODG", ListODG);

const MString inducerTypeHelp = "Use this option to select the type of "
 "inducer to use.  Refer to inducer-specific documentation for more "
 "details on what each of these inducers do.";


/***************************************************************************
  Description : Construct a disc_filter_inducer wrapping around the given
                  inducer.
  Comments    : Static function.  This is used by the automatic disc_filter
                  modes in env_inducer.
***************************************************************************/
static BaseInducer *make_disc_filter(const MString prefix,
				     BaseInducer *& innerInducer)
{
   DiscFilterInducer *inducer = new DiscFilterInducer("automatic-filter");
   inducer->set_user_options_no_inducer(prefix);
   inducer->set_inducer(innerInducer);
   return inducer;
}


/***************************************************************************
  Description : Using the options INDUCER and INDUCER_NAME, construct an
                  inducer of the appropriate type.
  Comments    : 
***************************************************************************/
BaseInducer *env_inducer(const MString prefix)
{
   static Bool init = FALSE;
   BaseInducer *baseInd;

   if (!init) {
      setup_search_inducers(envInducerEnum);
      init = TRUE;
   }
	 
   MString envVarType = prefix + "INDUCER";
   MString envVarName = prefix + "INDUCER_NAME";
   
   InducerType inducerType;
   get_option_enum(envVarType, envInducerEnum, inducerTypeHelp, FALSE,
		   inducerType);
   MString defaultInducerName = envInducerEnum.name_from_value(inducerType);   
   MString inducerName = get_option_string(envVarName, defaultInducerName,
					   "", TRUE);
   
   if (inducerType == null)
      // create a null inducer that never aborts
      return new NullInducer(inducerName, FALSE);
   else if (inducerType == id3) {
      ID3Inducer *inducer = new ID3Inducer(inducerName);
      inducer->set_user_options(prefix + "ID3_");
      return inducer;
   } else if (inducerType == hoodg) {
      HOODGInducer *inducer = new HOODGInducer(inducerName);
      BaseInducer *inner = inducer;
      return make_disc_filter(prefix, inner);
   } else if (inducerType == constInducer) {
      ConstInducer *inducer = new ConstInducer(inducerName);
      return inducer;
   } else if (inducerType == tablemaj) {
      TableInducer *inducer = new TableInducer(inducerName, TRUE);
      return inducer;
   } else if (inducerType == tablenomaj) {
      TableInducer *inducer = new TableInducer(inducerName, FALSE);
      return inducer;
   } else if (inducerType == ib) {
      IBInducer *inducer = new IBInducer(inducerName);
      inducer->set_user_options(prefix);
      return inducer;
   } else if (inducerType == c45prune) {
      MString c45Flags = get_option_string(prefix + "C45_FLAGS",
					 C45Inducer::defaultPgmFlags);
      C45Inducer *inducer = new C45Inducer(inducerName, c45Flags, TRUE);
      return inducer;
   } else if (inducerType == c45Rules) {
      MString c45Flags1 = get_option_string(prefix + "C45R_FLAGS1",
					 C45RInducer::defaultPgmFlags1);
      MString c45Flags2 = get_option_string(prefix + "C45R_FLAGS2",
					 C45RInducer::defaultPgmFlags2);
      C45RInducer *inducer = new C45RInducer(inducerName, c45Flags1, c45Flags2);
      return inducer;
   } else if (inducerType == c45noprune) {
      MString c45Flags = get_option_string(prefix + "C45_FLAGS",
					 C45Inducer::defaultPgmFlags);
      C45Inducer *inducer = new C45Inducer(inducerName, c45Flags, FALSE);
      return inducer;
   } else if (inducerType == naiveBayes) {
      NaiveBayesInd *inducer = new NaiveBayesInd(inducerName);
      return inducer;
   } else if (inducerType == accEst) {
      AccEstInducer *inducer = new AccEstInducer(prefix, inducerName);
      return inducer;
   } else if (inducerType == oneR) {
      int minInstPerLabel = get_option_int(prefix + "MIN_INST", 6, 
					   "Small parameter", TRUE);
      OneRInducer *inducer = new OneRInducer(inducerName, minInstPerLabel);
      return inducer;
   } else if (inducerType == eodg) {
      EntropyODGInducer *inducer = new EntropyODGInducer(inducerName);
      inducer->set_user_options(prefix + "ODG_");
      return inducer;
   } else if (inducerType == dfInducer) {
      DiscFilterInducer *inducer = new DiscFilterInducer(inducerName);
      inducer->set_user_options(prefix);
      return inducer;
   } else if (inducerType == lazyDT) {
      LazyDTInducer *inducer = new LazyDTInducer(inducerName);
      inducer->set_user_options(prefix + "LAZYDT_");
      BaseInducer *inner = inducer;
      return make_disc_filter(prefix, inner);
   } else if (inducerType == baggingInd) {
      BaggingInd *inducer = new BaggingInd(inducerName);
      inducer->set_user_options(prefix);
      return inducer;
   } else if (inducerType == peblsInd) {
      PeblsInducer *inducer = new PeblsInducer(inducerName);
      inducer->set_user_options(prefix);
      return inducer;
   } else if (inducerType == ahaIB) {
      AhaIBInducer *inducer = new AhaIBInducer(inducerName);
      inducer->set_user_options(prefix);
      return inducer;
   } else if (inducerType == oc1) {
      OC1Inducer *inducer = new OC1Inducer(inducerName);
      inducer->set_user_options(prefix);
      return inducer;                        
   } else if (inducerType == ptronInd) {
      PerceptronInducer *inducer = new PerceptronInducer(inducerName);
      inducer->set_user_options(prefix);
      return inducer;
   } else if (inducerType == winnowInd) {
      WinnowInducer *inducer = new WinnowInducer(inducerName);
      inducer->set_user_options(prefix);
      return inducer;
   } else if (inducerType == discNaiveBayes) {
      NaiveBayesInd *inducer = new NaiveBayesInd(inducerName);
      BaseInducer *inner = inducer;
      return make_disc_filter(prefix, inner);
   } else if (inducerType == catDt) {
      CatDTInducer *inducer = new CatDTInducer(inducerName);
      inducer->set_user_options(prefix);
      return inducer;
   } else if (inducerType == cfInducer) {
      ContinFilterInducer *inducer = new ContinFilterInducer(inducerName);
      inducer->set_user_options(prefix);
      return inducer;
   } else if (inducerType == listHOODG) {
      ListHOODGInducer *inducer = new ListHOODGInducer(inducerName);
      inducer->set_user_options(prefix);
      return inducer;
   } else if (inducerType == cn2) {
      CN2Inducer *inducer = new CN2Inducer(inducerName);
      inducer->set_user_options(prefix);
      return inducer;
   } else if (inducerType == COODG) {
      COODGInducer *inducer = new COODGInducer(inducerName);
      inducer->set_user_options(prefix);
      return inducer;
   } else if (inducerType == ListODG) {
      ListODGInducer *inducer = new ListODGInducer(inducerName, NULL);
      inducer->set_user_options(prefix);
      return inducer;
   } else if (baseInd = search_inducers(prefix, inducerType, inducerName))
      return baseInd;
   else
      err << "env_inducer:: invalid inducer type " << inducerType
	  << fatal_error;
   return 0;
}

