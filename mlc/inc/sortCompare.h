// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _sortCompare_h
#define _sortCompare_h 1

#include <basics.h>

template <class Element>
int sort_compare(const Element* a, const Element* b);

template <class Element>
int sort_ptr_compare(const Element** a, const Element** b);

#endif
