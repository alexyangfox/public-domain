// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _error_h
#define _error_h 1

#include <strstream.h>

MLCOStream& fatal_error(MLCOStream&);
ostream& fatal_abort(ostream&);
Bool is_valid_pointer(void *ptr, Bool fatalOnFalse = FALSE);


// leave it undefined to get link error.
#define NO_COPY_CTOR(X) X(const X&)
// for templates, use the format below with X not containing <T>
// otherwise g++ complains
#define NO_COPY_CTOR_TEMP(X,Y) X(const Y&)

// The second argument is to handle base class constructors we must specify
// the argument is always called x
// The statement "(void)x;" prevents warnings about x not being used
// This problem was originally solved using the comma operator, but that
// became a problem in CatGraph.h, where the arguments were too complex
#define NO_COPY_CTOR2(X,ctors) X(const X& x) : ctors \
    {(void)x; err << "No copy constructor for " #X << fatal_error;}

#define NO_COPY_CTOR3(X, ctor1, ctor2) X(const X& x) : ctor1, ctor2 \
    {(void)x; err << "No copy constructor for " #X << fatal_error;}

#define NO_COPY_CTOR4(X, ctor1, ctor2, ctor3) \
    X(const X& x) : ctor1, ctor2 , ctor3 \
    {(void)x; err << "No copy constructor for " #X << fatal_error;}

#define NO_COPY_CTOR5(X, ctor1, ctor2, ctor3, ctor4) \
    X(const X& x) : ctor1, ctor2 , ctor3, ctor4 \
    {(void)x; err << "No copy constructor for " #X << fatal_error;}


#define NOT_IMPLEMENTED \
    {err << "Unimplemented function called " << __FILE__ << " at line " \
         << __LINE__ << fatal_error;}
     
#endif
