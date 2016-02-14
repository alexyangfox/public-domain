// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _checkstream_h
#define _checkstream_h 1

#include <iomanip.h>

void check_ostream(ostream& stream, const char* fileName);
void check_cout(ostream& stream);
void check_cin(istream& stream);
void check_istream(istream& stream, const char* fileName);

#endif
