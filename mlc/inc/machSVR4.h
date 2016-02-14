// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _MACHSVR4_h
#define _MACHSVR4_h 0x0FFC

//* SVR4 does not have a bcopy(), we use memmove() instead
#define MEMMOVE(ptr1,ptr2,len) memmove((ptr1),(ptr2),(len))

#endif
