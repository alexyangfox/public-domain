// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _env_inducer_h
#define _env_inducer_h 1

// enum for inducers.  All inducers must be in here, even if
//   supported by other inducer routines, not env_inducer specifically.

enum InducerType { null, id3, hoodg, constInducer, tablemaj, tablenomaj,
		   ib, c45prune, c45noprune, naiveBayes,
		   fss, accEst, oneR ,discSearch, dfInducer, eodg, c45Rules,
                   lazyDT, baggingInd, orderFSS, c45ap, peblsInd, ahaIB,
                   ptronInd, winnowInd, discNaiveBayes, oc1, catDt,
                   cfInducer, tableCas, listHOODG, cn2, COODG, ListODG,
                   WeightSearch};

#include <BaseInducer.h>
#include <GetOption.h>

BaseInducer *env_inducer(const MString prefix = emptyString);
void setup_search_inducers(MEnum& envInducerEnum);
BaseInducer *search_inducers(const MString& prefix, InducerType inducerType,
			     const MString& inducerName);
#endif




