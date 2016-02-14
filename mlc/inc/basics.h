// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _basic_h
#define _basic_h 1

#ifdef IRIX
#define FASTINLINE
#endif

#include <stdlib.h>
// values.h is needed for MINDOUBLE, MAXDOUBLE, etc.
#include <values.h>

#define TRUE Bool(!0)
#define FALSE Bool(0)

// There's sometimes a need to give a reference to something that
//   is invalid.  the SGI compiler warns about NULL ref, so this
//   avoids the warning and is just as bad, i.e., an access will
//   cause program to abort.
#define NULL_REF 1
#if defined(GNU)
  typedef bool Bool;
#else
  typedef char Bool;  // After George Bool
#endif

typedef double Real;
typedef float StoredReal; // for arrays etc.

#define MAX_CHARS_PER_LINE 200

#include <MString.h>
#include <strstream.h>

extern int defaultOpenProt;
extern const char *ERROR_PREFIX;
extern const char *WRAP_INDENT;
extern const int bad_status; // Bad exit status for program
extern MString fatal_expected;

extern char err_text[];
extern MLCOStream err;

#include <error.h>
#include <MLCStream.h>
void verify_current_MLCOStream(ostream& s); //@@ get rid of this
extern MLCIStream Mcin;
extern MLCOStream Mcout;
extern MLCOStream Mcerr;
extern MLCOStream* currentMLCOStream; //@@ get rid of this?


// note that MString.c assumes that Real is typedef'd to a double.
#define REAL_MAX MAXDOUBLE
#define REAL_MIN MINDOUBLE
#define UNDEFINED_VARIANCE REAL_MAX

// This is needed to set the precision of the stream when doing Real I/O
// This values is trunc(DSIGNIF / log2(10)), where DSIGNIF is the
// number of significant bits for a double (defined in values.h)
#define REAL_MANTISSA_LEN 15
#define STORED_REAL_MANTISSA_LEN 7

// maximum exponent length + #significant digits + 2(for . and e)
// MAX_REAL_LEN is used to define array dimensions, so it cannot be
// extern const int (see Attribute.c::RealAttrInfo::attrValue_to_string())
#define MAX_REAL_LEN (_DEXPLEN + REAL_MANTISSA_LEN + 2)
#define MAX_STORED_REAL_LEN (_FEXPLEN + STORED_REAL_MANTISSA_LEN + 2)

// Default wrap width for an MStream, this guarantees that it is off initially
#define DEFAULT_WRAP_WIDTH  0
// Default wrap prefix for an MStream -3 spaces
#define DEFAULT_WRAP_PREFIX "   " 

// These are defined here because of a Bug in CFront that declared
//   INT_MIN wrong (without (int)).  What happens is that 2147483648
//   does not fit in int, so it is made unsigned, then unary minus subtracts
//   from max unsigned int to get the same number.
#define SHORT_MAX      32767 
#define LONG_MAX       2147483647
#define INT_MAX        LONG_MAX
// defining LONG_MIN as -2147483648 does not work because the minus is
//   a unary operator and the number causes it to be unsigned.  We can
//   cast to (long), but ObjectCenter gives a warning... This method
//   is adapted from GNU limits.h
#define SHORT_MIN      (-SHORT_MAX-1)
#define LONG_MIN       (-LONG_MAX-1) 
#define INT_MIN        LONG_MIN
// MAX_LONG_LEN is used to define array dimensions so it cannot be
//    extern const int. 
// LONG_MAX is +2147483647 which is 10 digits +1 for sign.
// Note that this is an upper bound and MAX_LONG can be lower.
#define MAX_LONG_LEN   11
// Similar to LONG_MAX, 5 digits +1 for sign.
#define MAX_SHORT_LEN 6  
#define	UCHAR_MAX     255  // max value of an "unsigned char"

// To prevent accidental copy construction, most copy constructors
// take a second "dummy" argument.  Initially this dummy argument was
// forced to be an integer and that integer was checked to be 0,
// The new standard is that it would be much simpler to have the dummy 
// argument be an actual enumerated type.
// Example usage:
//	Foo.h: Foo(const Foo& foo, CtorDummy); 
//	Client.h: Foo foo2(foo1, ctorDummy);
enum CtorDummy {ctorDummy=0}; 

// Also, there is an enumerated type for dummy arguments.
enum ArgDummy {argDummy=0};

// DBG*(code) needs a trailing semicolon outside, i.e. DBG(x=3);
// The last statement inside does not need a semicolon.  The semicolon
//   outside allows proper indentation in Emacs C++ mode.
//   and is also required inside IFs to generate an empty statement.

// The "else" is so DBG will work even if it is inside an if statement.
//   (the else in the definition assures proper matching of the else
//    in the code, and also makes use of the semicolon...):
//
//       if (foo)
//          DBG(bar);
//       else
//          kuku();

// debugLevel is initially INT_MAX because if something static is
//   declared, all debugs should work until the level is lowered in
//   "basics.c", where it is set to the environment variable DEBUGLEVEL
extern int debugLevel;

#ifndef FAST
#define DBG(stmts) if (debugLevel >= 1) {stmts;} else
#else
#define DBG(stmts) 
#endif

#ifdef TEMPL_MAIN 
   #define TEMPL_GENERATOR(name) main()
#else
   #define TEMPL_GENERATOR(name) name()
#endif


// DBGSLOW() should only be used for especially expensive code.
#ifndef FAST
#define DBGSLOW(stmts) if (debugLevel >= 2) {stmts;} else
#else
#define DBGSLOW(stmts) 
#endif

// DBG_DECLARE() is intended for use in class or function declarations,
//   where "if" statements are not allowed.
// Note that when using this macro, the semicolon must be INSIDE,
//   since empty declarations are not allowed (r9.2)
#ifndef FAST
#define DBG_DECLARE(stmts) stmts
#else
#define DBG_DECLARE(stmts) 
#endif

// This macro causes a fatal_error if the given condition is FALSE.
// The behavior is similar to the C/C++ assert (see include/assert.h).
// The macro is executed as a DBG statement.

// ASSERTs for debugging purposes should be in DBG().
#define ASSERT(stmt) \
      ((stmt)?((void)0): \
       (void)(err << "MLC++ internal error: assertion failed: " # stmt   \
	", file " << __FILE__ << ", line " << __LINE__ << fatal_error))

// Old version below caused compiler core dump on SGI and ConstCat
//      (void)((stmt) ||                                                     \
//      ((err << "MLC++ internal error: assertion failed: " # stmt   \
//	", file " << __FILE__ << ", line " << __LINE__ << fatal_error), 0))

// This is the size of the buffer used for reading and writing files.
// It must be defined "const" because it is used to dimension arrays.
const int IO_BUFFER_SIZE = 1024;

// This is the maximum length of a string read from a file
const int MAX_INPUT_STRING = 100;

extern const Real       REAL_EPSILON; 
extern const StoredReal STORED_REAL_EPSILON;
extern const int        DEFAULT_PRECISION;


// See basics.c for these values.
extern const MString WHITESPACE;
extern const int    defaultLineWidth;
extern const float  defaultPageX;
extern const float  defaultPageY;
extern const float  defaultGraphX;
extern const float  defaultGraphY;
extern const MString defaultFileName;
extern const MString MLCPATH;
extern       int    globalLogLevel;
extern MLCOStream*  globalLogStream;
extern const MString dotty;
// remember: mktemp requires a non-const char *   
extern const MString tmp_string;
extern const MString dot;
extern const MString emptyString;
extern const Real confidenceIntervalProbability;
extern const Real confidenceIntervalZ; // value such that area under standard
				       // normal curve going left & right Z,
				       // has confidenceIntervalProbability 

extern "C" { int centerline_true(void);}

// Declare a static string with the RCSID.  Use a function to 
// initialize so that the compiler will not warn about declare and not
// used.  A better solution would be nice.
#define RCSID(str) const char *usercs(const char *s); \
                   static const char* id = usercs(str);


// DECLARE_DISPLAY declares operator<< for the given class (.h file)
// DEF_DISPLAY defines it (for use in .c file)
// For templated classes do
//   template <class T> DEF_DISPLAY(DblLinkList<T>)
#define DECLARE_DISPLAY(class) \
    MLCOStream& operator<<(MLCOStream& s, const class& c)
// Note that there is NO SEMICOLON after DEF_DISPLAY
#define DEF_DISPLAY(class) \
   MLCOStream& operator<<(MLCOStream& s, const class& c) \
   {c.display(s); return s;} 

const MString& bool_to_string(Bool boolValue);
inline Real squareReal(Real x) {return x*x;}
inline double squareDouble(double x) {return x*x;}
Real Mround(Real x, int digits);

MString get_env_default(const MString& envVarName, const MString& defaultName);
MString get_env(const MString& envVarName);
// min/max templates prefer first argument on tie-breakers
template<class T> inline T max(T a, T b) {return (a >= b) ? a : b;}
template<class T> inline T min(T a, T b) {return (a <= b) ? a : b;}

template<class T> inline void swap(T& a, T& b)
   {T temp; temp = a; a = b; b = temp;}

// Log base 2.  Solaris doesn't have log2().
double log_bin(double num);

// Lose the least significant digits of Reals in an array.  Sometimes things
// just don't add up right...
template <class Element> class Array;
extern void lose_lsb_array(Array<Real>& a);

#endif







