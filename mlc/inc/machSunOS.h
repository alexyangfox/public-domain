// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _MACHSUNOS_h
#define _MACHSUNOS_h 0x0FFF

//* SunOS does not have a memmove routine, we use bcopy() instead
#define MEMMOVE(ptr1,ptr2,len) bcopy((ptr1),(ptr2),(len))

#endif
