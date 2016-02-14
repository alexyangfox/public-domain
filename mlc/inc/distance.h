// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _DISTANCE_h
#define _DISTANCE_h 1

class InstanceRC;
class InstanceBag;

Real inst_dist(const InstanceRC inst1, const InstanceRC inst2);
Real bag_dist(const InstanceBag& lib, const InstanceRC inst, int& numMin);
Real avg_bag_dist(const InstanceBag& lib, const InstanceRC inst);
#endif
