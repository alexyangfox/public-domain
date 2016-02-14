// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/***************************************************************************
  Description  : Every file must include basics.h.  Here we define some
                   constants, an initialization function and
                   termination function.
                 There is an MLCInit class defined here, whose sole
                   purpose is to execute initialization code.  A static
                   class instance forces the constructor to be executed,
                   and hence this code will execute before main()
                   is called.
                 Note that since initialization order of static instances
                   is not defined within different files, it is important
                   that all initialization be done here, or at least that
                   any other initialization will not depend on anything
                   initialized here.  This is especially a problem with
                   constructors that call fatal_error and output to err,
                   since err may not be initialized at that stage.
  Assumptions  : Note the dependency between LONG_MAX and MAX_LONG_LEN.
  Comments     : Note that using "const int a=3" makes it a static member,
                   hence we preferred a global variable that can
                   easily be changed and avoids duplication of information.
  Complexity   :
  Enhancements : Should Bool be a class ?  It would be nice to have it
                   print True/False when it is sent to cout with <<.
                 Still looking for a better solution for the stupid
                     warning of not using rcsid in basics.h
  History      : Ronny Kohavi                                       7/13/93
                   Initial revision (.c)
                 Ronny Kohavi                                       8/26/93
                   Initial revision (.h)
***************************************************************************/

#include <basics.h>
#include <MLCStream.h>
#include <LogOptions.h>
#include <safe_new.h>
#include <sysent.h>    // needed for umask
#include <sys/stat.h>
#include <math.h>
#include <GetOption.h>

RCSID("MLC++, $RCSfile: basicCore.c,v $ $Revision: 1.10 $")

int debugLevel  = INT_MAX; // Have maximum int, but this is set in MLCInit
const char *ERROR_PREFIX = "Error -";
const char *WRAP_INDENT  = "   ";
const int bad_status = 2; // Bad exit status for program
MString fatal_expected = "";
const int max_error_message_size = 500;
char err_text[max_error_message_size];
MLCOStream err("err",err_text, max_error_message_size);

// Note that const usually has internal linkage UNLESS it has
// previously been given external linkage (Stroustrup page 517)

int defaultOpenProt;
MLCIStream Mcin(cin);
MLCOStream Mcout(cout, FileStream);
MLCOStream Mcerr(cerr, FileStream);
// currentMLCOStream is initialized here and reset again in check_ostream.
MLCOStream* currentMLCOStream = &Mcout;

const Real REAL_EPSILON = 1.0e-15; // Do 2^-FSIGNIF or DSIGNIF
const StoredReal STORED_REAL_EPSILON = 1.0e-6; // Do 2^-FSIGNIF or DSIGNIF
const int  DEFAULT_PRECISION = STORED_REAL_MANTISSA_LEN;
const MString WHITESPACE = " \n\t\v\r\f"; // see isspace()
const float  defaultPageX = 8.5;
const float  defaultPageY = 11.0;
const float  defaultGraphX = 7.5;
const float  defaultGraphY = 10.0;
// environment variable that holds the name of the directory
// to look in if a file is not found in ./ (current directory)
const MString MLCPATH = "MLCPATH";

// Note that we can't put the long_value with the same statement as
//   get_env_default because it's a temporary variable and is
//   apparently destroyed in ObjectCenter (we can MString assert
//   failure because this = &source in copy).
MString defaultLogLevel("0");
MString logLevelStr = get_env_default("LOGLEVEL", defaultLogLevel);
int globalLogLevel = logLevelStr.short_value();

MLCOStream*  globalLogStream = &Mcout;
const MString dotty = get_env_default("DOTTY", "dotty");
const MString dot = get_env_default("DOT", "dot");

MOption::PromptLevel MOption::promptLevel = requiredOnly;
MEnum MOption::promptLevelEnum;
MEnum MOption::boolEnum;

MString MOption::optionDump = "";
MLCOStream *MOption::optionDumpOut = NULL;
       

// Function tath takes a bool and returns a string.  This could
// eventually be moved to a small_function.c file.
static MString trueString("True");
static MString falseString("False");
const MString emptyString("");
const MString& bool_to_string(Bool boolValue)
  {return (boolValue ? trueString : falseString);} 

// The following function is a dummy function that just returns the
// input string.  This is "needed" for the RCSID macro
const char *usercs(const char *s) {return s;}

/***************************************************************************
  Description : Round a given number, so that there are only d digits
                  after the decimal point
  Comments    :
***************************************************************************/

Real Mround(Real x, int digits)
{
   Real scale = pow(10,digits);
   int  sign  = (x >= 0)*2 - 1;
   return floor(fabs(x) * scale + 0.5) / scale * sign;
}


// The following class is static so that code can be executed on MLC++
//   startup or before exiting.

class MLCInit {
public:
   MLCInit();
   ~MLCInit();
};

MLCInit::MLCInit()
{
   extern int errLineWidth;

   mode_t mask = umask(0);              // stores the old umask
   umask(mask);                         // restore mask
   defaultOpenProt = int(mask ^ 0666);  // bit-wise XOR to get protections
   
   init_safe_new();
   if (globalLogLevel < 0)
      err << "Negative global log level " << globalLogLevel << fatal_error;

   // if stdout refers to a terminal (tty), then let the LINE_WIDTH default
   // to 79.  Otherwise, make it MAX_INT to avoid auto-wrapping in redirected
   // output.  We don't set it to 0 so that SHOW_LOG_ORIGIN will work.
   MString defaultLineWidth(SHORT_MAX, 0);
   if(isatty(1))
      defaultLineWidth = "79";

   int lineWidth = get_env_default("LINE_WIDTH", defaultLineWidth).
      short_value();
   Mcout.set_width(lineWidth);
   errLineWidth = lineWidth; // for fatal_abort().
   
   
   Mcout.set_wrap_prefix(get_env_default("WRAP_PREFIX", "   "));
   Mcout.set_newline_prefix(get_env_default("NEWLINE_PREFIX", ""));
   //Tie operation. See Stroustrup 10.4.1.1 [P. 338] (Gray book)
   Mcin.get_stream().tie(&Mcout.get_stream());
   // Default to debugLevel 0
   debugLevel  = get_env_default("DEBUGLEVEL", "0").short_value(); 
   #ifdef FAST
      if (debugLevel > 0)
	 Mcerr << "MLC++ Warning: fast library ignores DEBUGLEVEL > 0" << endl;
   #endif
   
   GLOBLOG(1, "MLC++ Debug level is " << debugLevel
           << ", log level is " << globalLogLevel << endl);

   // Cannot use DBG in FAST mode.
   if (debugLevel >= 2 && globalLogLevel > 0)
   #ifdef FAST
	Mcout << "Fast MLC++ library compiled on "
	      << __DATE__ << " " << __TIME__ << endl;
   #else
     if (debugLevel >= 2 && globalLogLevel > 0)
       Mcout << "MLC++ library compiled on "
	     << __DATE__ << " " << __TIME__ << endl;
   #endif

  MOption::initialize();

}

MLCInit::~MLCInit()
{
   MOption::finish();
   end_safe_new();
}

static MLCInit mlcInit;



// Log base 2.  Solaris doesn't have log2().
double log_bin(double num) {return log(num) * M_LOG2E;}


/***************************************************************************
  Description : Lose the least significant digits of Reals in an array.
  Comments    :
***************************************************************************/
void lose_lsb_array(Array<Real>& a)
{
   for (int i = a.low(); i <= a.high(); i++) {
      float temp = a[i];
      a[i] = (double) temp;
   }
}
