// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _DestArray_h
#define _DestArray_h 1

#include <Attribute.h>
#include <Array.h>

class DestArray {
public:
   static NominalVal same_dest(const Array<NominalVal>& dests);
   static Bool consistent_dests(const Array<NominalVal>& dests1, 
                               const Array<NominalVal>& dests2);
   static Bool included_in_dest(const Array<NominalVal>& dests1, 
                               const Array<NominalVal>& dests2);
   static int  merge_dests(Array<NominalVal>& dests1, Array<Real>& destCount1,
               const Array<NominalVal>& dests2, const Array<Real>& destCount2);
   static int  num_dests(const Array<NominalVal>& dests);
   static int  num_different_dests(const Array<NominalVal>& dests);
   static int  num_new_dests_from_merge(const Array<NominalVal>& dests1,
                                        const Array<NominalVal>& dests2);
   static int  num_diff_dests_after_merge(const Array<NominalVal>& dests1,
                                          const Array<NominalVal>& dests2);
};


#endif
