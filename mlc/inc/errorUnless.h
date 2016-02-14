// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name "error.c" suffix.

/***************************************************************************
  Description  : Provide primitive error catching capability.
  Comments     : To use this in testers,
                   use the macro TEST_ERROR(msg, stmts) where
                   msg is part of the expected error message, and stmt
                   is some set of statements that generate the expected
                   error.
  Enhancements : 
  History      : Ronny Kohavi                                       7/17/93
                   Initial revision

***************************************************************************/


#ifndef _errorUnless_h
#define _errorUnless_h 1

#include <error.h>
#include <setjmp.h>

extern MString fatal_expected;
extern jmp_buf backFromFatal;

#ifndef FAST
#define TEST_ERROR(msg,stmts) if (debugLevel) { \
   fatal_expected = msg; \
   if (setjmp(backFromFatal) == 0) \
      {stmts; err << "TEST_ERROR at " << __FILE__ << ':' << __LINE__ << \
              " did NOT generate expected fatal_error" << fatal_error;}}
#else
#define TEST_ERROR(msg,stmts)
#endif
#endif
