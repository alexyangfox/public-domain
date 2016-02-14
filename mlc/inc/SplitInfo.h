// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _SplitInfo_h
#define _SplitInfo_h 1
#include <MEnum.h>

struct SplitInfo {
   int  attrNum;
   enum  SplitType {noReasonableSplit, realThresholdSplit, nominalSplit};
   SplitType splitType;
   Real mutualInfo;
   Real condEntropy;
   union {
      Real threshold;  // used in realThresholdSplit
      // any other info we need
   } typeInfo;
   // Make sure it defaults to unreasonable values.
   // Note that mutualInfo >= 0.
   SplitInfo()
   {
      attrNum = -1; splitType = noReasonableSplit; mutualInfo = -REAL_MAX;
      condEntropy = -1; typeInfo.threshold = -1;
   }
};

enum SplitByType {mutualInfo, normalizedMutualInfo};
const MEnum splitByEnum = MEnum("mutual_info", mutualInfo)
  << MEnum("normalized_mutual_info", normalizedMutualInfo);
const MString splitByHelp = "This option specifies what kind of mutual "
  "information we should use while picking up the best_split";
const SplitByType defaultSplitBy = normalizedMutualInfo;
#endif   
