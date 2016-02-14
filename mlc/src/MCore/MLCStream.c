// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  :  MLCIStream is an ifstream which will do various safety
                    checks as well as provide several useful operations which
		    regular C++ streams do not. MLCIStreams can inform you of
		    their current position in the stream, tell you their
		    current line number, and provide a full set of regular
		    C++ operations through the use of manipulators. Whenever
		    any operation is done on the underlying ifstream which
		    (under the regular C++ streams) could yield unpredictable
		    results, MLCIStreams will abort and state the problem.

		  MLCIStream(istream&) works only for cin.  Note that
                    it is not recommended for Mcin and cin to be used
		    concurrently since input could be lost (Mcin is attached
		    to cin). This is especially confusing if you do
		    MLCIStream(cin) and redirect the input because cin will
		    be at EOF after the first read from the MLCIStream.
		    In these cases, just use Mcin to read from cin. If all
		    reads are done through this stream, it will do all the
		    buffering.
		  Input is assumed not to overflow the given type, i.e.,
                    if you do MLCIStream >> long, then there is no
		    check for overflow.  To make safe input, you can
		    input to an Mstring and convert to the desired type.		    
		  MLCOStream is a safe output stream which supports the notion
		    of several different output types. Currently supported are
		    the regular C++  ofstream and strstream, an xstream type,
		    and partial support for PrinterStreams.
		      Note that while most MLCOStreams work with out
		    restrictions, Xstream is a special case.  It currently
		    does not have the capability to associate a stream with it
		    and works only with the DotGraphDisplay.  Finally, note
		    that the default stream is File Stream throughout the code.


		  In addition to being able to maintain the current line
		    number and position in the stream, MLCOStreams perform
		    output formatting based on a) the line width,b) the newline
		    prefix, and c) the wrap prefix.

		  The trick to performing the wrapping is to dump all output
		    to an strstream when the line width is not zero. The
		    strstream holds all of the users output in a char* buffer
		    and compares the current width of the output data to the
		    line width set via set_width(). Input is then processed
		    in two stages: a) Newlines ('\n') are printed from the
		    stream and b) individual space-delimited tokens are put on
		    the stream until the the width of the current line is
		    greater than that set via set_width(). Output stream
		    wrapping is then accomplished according to the following
		    rules:

		      In all of the below, "regular C++ streams" means that if
		    we were to replace MLC++ streams with C++ streams, the
		    positions would be those defined by those of C++ streams.

		    1) If a string of length > 0 (including newline) is being
		    printed at the leftmost position in regular C++ streams,
		    it will be preceded by the newline prefix defined at this
		    time.
		    2) If a string of length l is to be printed and the corre-
		    sponding position of the regular C++ streams is not 0 and
		    the (position + l) is greater than the wrapwidth,
		    a newline is printed, followed by the wrap prefix,
		    followed by the string; otherwise the string is printed.
		  The use of the strstream has greatly facilitated the support
		    for an strstream-based output stream which can buffer all
		    output as a string and return it to the user via the
		    mem_buf() method.
		  Remember that if  wrapping is on and you don't output spaces
		    or newlines in your code, the MLCOStream will buffer your
		    output to perform the wrapping correctly. This implies
		    several things:
		    1) Outputting a string ending with space(s) which would 
		     cause the current line to wrap will a) generate a newline
		     and b) print the wrap prefix followed by the string which
		     caused overflow on the previous line. In addition to this,
		     trailing whitespace on a string is "eaten" off or removed
		     from the end of the line which wrapped.
		    2) If you have some long string which is not *space* (" ")
		     delimited, it will be buffered until output on that line
		     has exceeed the wrap-width. If you want to see the output
		     before the string has wrapped, insert the manipulator
		     "flush" on the stream.
		  Always remember that it is an error if the wrap prefix or
		   newline prefix is set to a string with length > the current
		   wrap width. You're program will abort if you try to do
		   something equivalent to this:

		     Mcout.set_width(3);
		     Mcout.set_newline_prefix("A newline prefix...");

		  Wrapping is on if the width is set to be greater than 0.
		    (e.g. Mcout.set_width(80);)
		  Wrapping is turned off is the width is set to 0.
		    (e.g. Mcout.set_width(0);)
		  Wrap prefixes are set via the set_wrap_prefix() method.
		  Newline prefixes are set via the set_newline_prefix() method.
		  If a user would like to maintain a set of "canned"
		    formatting options, they may perform any of the above
		    three operations on the stream and keep a set of those
		    options for future use via the use of the MStreamOptions
		    structure.
		  MStreamOptions is a struct which stores all of the formatting
		   options for the stream. When a user wants to change the
		   format options for this class, they should perform
		   something similar to the following:

		   // get a copy of the current state
		   MStreamOptions saveOptions = Mcout.get_options();

		   // set the newline prefix, wrap prefix, line width
		   // to your needs

		   // restore them
		   Mcout.set_options(saveOptions);
		   In this way, different formatting styles can be
		   implemented throughout the code.
		 Formatting options may be saved via get_options().
		 Formatting options may be restored via set_options().

		 One last important point regarding wrapping is that
		   pos_in_line() is unsupported if wrapping is not on.
		   You're program will abort if you do not set the
		   wrapping on first *before* calling pos_in_line().
		   Note that pos_in_line() flushs output when called.
		 The use of Mcout in conjunction with the regular C++ cout is
		   not recommended. The reason for this is that Mcout is
		   attached to the cout descriptor and that some output may be
		   lost on cout if the user attempts to use both streams
		   concurrently. In all cases, it is recommended that either
		   Mcout *or* cout be used and that both are not used
		   simultaneously. If you would like Mcout to behave in the
		   same fashion as cout, simply set the wrap width to be zero
		   and use get_fstream() where required.
		 MLCOStream(ostream&) works only for ofstreams: cout, cerr,
		   and clog.
		 The destructors do not close the underlying file descriptor
		   for streams which were passed as parameters(e.g. cout, cin,
		   cerr) but they close streams that the constructors create.
		 Problem: When there is user input, the user types in a string
		   and ends with a CR. The user input is echoed, but the
		   output stream doesn't know this so it wraps in bizarre
		   places. Since input currently confuses the output routines,
		   reset_pos_in_line() should be called.
		   
  Assumptions  : Output which is to wrap correctly is assumed to be space
                   delimited; tabs are currently not handled as whitespace.
		   
  Comments     : If a stream is passed to a MLCStream constructor, that
                   stream should not be closed before the the last
		   operation using the MLCStream is called.
		 Using "" as a description (or file name) will cause an abort.
		 MLCIStream's are tied (via. tie() ) to MLCOStream so that
		   flushing occurs after input.
		 The constructor which Attaches a given file to an MLCOStream
		   ( MLCOStream(const MString&, ofstream&, const OutputType) )
		   requires the user to close the stream that was passed AFTER
		   the final write to it.
		 The macros VERIFY_OSTREAM() and VERIFY_ISTREAM() must be
		   enclosed in brackets {} in order for them to work.
		 
  Complexity   : Overall, a multiplicative constant of C++ streams excpet for
                   the pos_in_line() operations which take O(Pos) - O(current)
		   when no random IO is done and O(StreamLength) when random IO
		   is performed.

  Enhancements : Tied input/output streams should update linePos, currently
                   they don't, this is what prompted the need for
		   reset_pos_in_line().
                 Support manipulators that don't take its, such as
		     setiosflags(ios::fixed).
                 Need to support the Output Types Default and PrinterStream.
		 Want to think about what happens if conversion methods
		   don't make sense, i.e. for XWindowDisplay.
		 Want to extend support of the Output Type for X-Windows
		   to draw the graph to the same X-Window.  Currently,
		   whenever the user displays a graph to a MLCOStream
		   which is of Output Type XWindowDisplay, a new Window
		   is popped up.  Alternatively, the user could request
		   that all graphs be entered into the same (scrollable)
		   window.  This could be done with some kind of unique
		   identifier.
		 Support ios::prot for void MLCIStream::open_read().
		 Support tabs as well as spaces in
		   output_format_stream_no_newline() .
		 
  History      :   James Dougherty                                 10/09/94
                   Manipulators, wrapping, line counting, line-pos tracking,
		   extended error-checking, strstream support.
                   Richard Long                                    09/01/93
                   Initial revision
		   
 ***************************************************************************/

#include <basics.h>
#include <machine.h>
#include <MLCStream.h>
#include <ctype.h>
#include <string.h>
#include <checkstream.h>
#include <stdio.h>
#include <math.h>
#include <mlcIO.h>

RCSID("MLC++, $RCSfile: MLCStream.c,v $ $Revision: 1.73 $")

const char* TAB = "	";
const int CheckPrefixes = 1;

/***************************************************************************
  Description : Checks to see that the stream didn't change when stmt
                  is being executed, and returns the MLCOStream / MLCIStream.
  Comments    : In non-fast mode, we ignore this check.
                In both cases, we call check_ostream to make sure
		  that the state is OK.
***************************************************************************/
#ifdef FAST
#define VERIFY_OSTREAM(MLCOStream, stmt) \
      (MLCOStream).get_stream() << stmt;  \
      check_ostream((MLCOStream).get_stream(), (MLCOStream).description()) 
#else
#define VERIFY_OSTREAM(MLCOStream, stmt) \
   ostream& strm = (MLCOStream).get_stream() << stmt;\
   check_ostream((MLCOStream).get_stream(), (MLCOStream).description());\
   ASSERT(strm == (MLCOStream).get_stream())
#endif

#ifdef FAST
#define VERIFY_ISTREAM(MLCIStream, stmt) \
      (MLCIStream).get_stream() >> stmt;  \
      check_istream((MLCIStream).get_stream(), (MLCIStream).description()); \
      return MLCIStream
#else
#define VERIFY_ISTREAM(MLCIStream, stmt) \
  istream& strm = (MLCIStream).get_stream() >> stmt;\
  check_istream((MLCIStream).get_stream(), (MLCIStream).description());\
  ASSERT(strm == (MLCIStream).get_stream());\
     return MLCIStream
#endif

		       
/***************************************************************************
  Description : Defines the >>,<< operators for an MLCIStream,MLCOStream
                  object for the given type.
		If the line width has been set by a call to set_wrap_width,
		  it is assumed that stream output wrapping is on and the
		  stream is formatted according to the values of the
		  new_line_prefix and wrap_prefix. Note how all output
		  is dumped to the strstream named formatStream. This
		  is the first step to performing wrapping.
  Comments    :
***************************************************************************/  
#define DEF_OPERATOR_GG(type) MLCIStream& MLCIStream::operator>>(type t){\
   check_stream_is_valid();\
   stream >> t; check_istream(stream, description());\
   return *this;\
}

#define DEF_OPERATOR_LL(type) MLCOStream& MLCOStream::operator<<(type t){\
  check_stream_is_valid();\
  if(wrap_width()){\
      formatStream << t;\
      output_format_stream(FALSE);\
   }\
   else\
      stream << t;\
  return *this;\
}
	       
/***************************************************************************
  Description : Defines the read_bin/write_bin methods to stream a value
                  of type T in or out as a sequence of sizeof(T) bytes.
  Comments    : The warning message given under OC exists since we are
                  coercing a type t to be treated as an unsigned char.
***************************************************************************/
#define DEF_READ_BIN(type) void MLCIStream::read_bin(type& value) {\
		      get_fstream().read((char *)&value, sizeof(type));\
  		      check_istream(get_fstream(), description());}

#define DEF_WRITE_BIN(type) void MLCOStream::write_bin(type& value) {\
		      get_fstream().write((char *)&value, sizeof(type));\
  		      check_ostream(get_fstream(), description());}						   
/***************************************************************************
  Description : Checks the description to make sure that the name is
                  not an empty string.
  Comments    :
***************************************************************************/
static void check_description(const MString& descr)
{
   if (descr == "")
      err << "MLCStream.c ::check_description() : Empty string is not a valid "
	 "description" << fatal_error;
}


/***************************************************************************
  Description : Helper function for constructing the MLCIStream.
  Comments    : Factored code from c-tors. Note that this code changes
                  the important constants in the code (i.e. a 1-based
		  position in line, amd 1 based line count).
  
***************************************************************************/
void MLCIStream::init()
{
   posInLine = lineCount = 1;
   closed = FALSE;
   tellpLast = 0;
}

/***************************************************************************
  Description : Attach
  Comments    : Compiler dependent
***************************************************************************/

void MLCIStream::attach(int fd)
{
   get_fstream().rdbuf()->attach(fd);
}


/***************************************************************************
  Description : Tries to open a file in the current directory.  If open is
                  unsuccessful, checks are performed on the
		  environment variable whose name is MLCPATH for an
		  alternative directory.
		  Reports  an error iff both attempts are unsuccesful.
  Comments    : private method.
***************************************************************************/
void MLCIStream::open_read()
{
   MString fileName = file_exists(description(), TRUE);

   get_fstream().open(fileName);
   if (!get_fstream().good())
      err << "MLCIStream::open_read: Could not open file '"
	  << fileName << fatal_error;
}


/***************************************************************************
  Description : Updates the line count.
  Comments    : Does the dirty work of counting the lines. private method.
***************************************************************************/
void MLCIStream::update_line_count()
{
   long tellpCurrent = get_fstream().tellg();
   if(tellpCurrent < tellpLast)
      tellpLast = 0;
   get_fstream().seekg(tellpLast);
   while(tellpLast < tellpCurrent && !get_fstream().eof()) {
      if(get_fstream().get() == '\n'){
	 lineCount++;
	 //must be 1, since the first character we get is numbered 1
	 posInLine = 1;
      }
      tellpLast++;
      posInLine++;
   }
}


/***************************************************************************
  Description : Calls check_istream(), verifies that one of the references
                  to a stream is valid.
  Comments    : At least one stream must be NULL, but it may be that
                  both are NULL (i.e. XStream).
***************************************************************************/
void MLCIStream::OK(int /*level*/) const
{
   DBG(ASSERT(strStream == NULL  ||  fileStream == NULL));
   DBG(ASSERT(fileStream == NULL ||  &stream == fileStream));
   DBG(ASSERT(strStream  == NULL ||  &stream == strStream));
   check_istream(stream, description());
}


/***************************************************************************
  Description : Opens a new file with the given name.  
  Comments    : Stream is closed by the destructor.
***************************************************************************/
MLCIStream::MLCIStream(const MString& fileName)
   : descr(fileName),
     strStream(NULL),
     fileStream(new ifstream()),
     stream(*fileStream)
{
   init();
   check_description(fileName);
   open_read();
   setbuf(buf, INPUT_BUFFER_SIZE);
}


/***************************************************************************
  Description : Attaches the given stream to the MLCIStream.
  Comments    : The user must close the stream that was passed *after* the
                  final read from the MLCIStream.
**************************************************************************/
MLCIStream::MLCIStream(const MString& dscr, ifstream& strm)
   : descr(dscr),
     strStream(NULL),
     fileStream(new ifstream()),
     stream(*fileStream)
{
   init();
   check_description(dscr);
   attach(strm.rdbuf()->fd());
   OK();
}


/***************************************************************************
  Description : Attaches the given stream to the MLCIStream.
  Comments    : This works only for cin (standard input).
                Gives the description "standard input (cin)"
                The destructor will NOT close cin.
***************************************************************************/
MLCIStream::MLCIStream(istream& strm)
   : descr("standard input (cin)"),
     strStream(NULL),
     fileStream(new ifstream()),
     stream(*fileStream)
{
   init();
   if (strm != cin)
      err << "MLCIStream::MLCIStream(istream&): Unknown istream "
	  << description() << fatal_error;
   attach(0);
   OK();
}


/***************************************************************************
  Description : Attaches the stream with the given file descriptor to the 
                  MLCIStream.
  Comments    : The destructor will NOT close the associated stream.
                If in doubt about a file descriptor, use one of the other 
		  constructors.
***************************************************************************/
MLCIStream::MLCIStream(int fd, const MString& dscr)
   : descr(dscr),
     strStream(NULL),
     fileStream(new ifstream()),
     stream(*fileStream)
{
   init();
   check_description(dscr);
   attach(fd);
   OK();
}


/***************************************************************************
  Description : Closes the stream; checks that stream is still good.
                Does not close the underlying file descriptor
                  if it was supplied with attach()
  Comments    : The USER must still close stream which were passed using
                  constructors other than MLCIStream::MLCIStream(MString&)
***************************************************************************/
MLCIStream::~MLCIStream()
{
   if (stream.eof())  // allow closing when at EOF
     stream.clear();
   OK();
   if(fileStream && !closed)
      close();
   delete &stream;
}


/***************************************************************************
  Description : Closes the file.
  Comments    : Does not close the underlying file descriptor if it was
                  supplied with attach(), currently close is defined for
		  file streams.
***************************************************************************/
void MLCIStream::close()
{
   if(closed)
      err << "MLCIStream::close: Stream "<< description()
	  << " has already been closed " << fatal_error;
   get_fstream().close();  
   if (!get_fstream().good())
      err << "MLCIStream::close: Error occured closing stream " 
	  << description() << fatal_error;
   closed =TRUE;
}


/***************************************************************************
  Description :  Checks that the stream is not closed, aborts if so.
  Comments    :
***************************************************************************/
void MLCIStream::check_stream_is_valid() const
{
   if (closed)
      err << "MLCIStream::check_close: attempted operation on closed stream: "
	  << description() << fatal_error;
}


/***************************************************************************
  Description : Returns the line number in the file
  Comments    : 
***************************************************************************/
int MLCIStream::line_count()
{
   update_line_count();
   check_istream(stream, description());
   return lineCount;
}


/***************************************************************************
  Description : Returns the character position of the line in the file,
                  -1 if unsuccesful.
  Comments    : 
***************************************************************************/
int MLCIStream::pos_in_line()
{
   update_line_count();
   check_istream(stream, description());
   return posInLine;
}


/***************************************************************************
  Description : Returns the fstream associated with the MLCIStream. If this
                  pointer has not been set, (i.e. it is NULL) then it aborts.
  Comments    : Calling this would abort if the user has a stream of
                  type strstream or of type XStream.
***************************************************************************/
ifstream& MLCIStream::get_fstream() 
{
   if(fileStream==NULL)
      err << "MLCIStream::get_fstream: fileStream has not been set "
	  << description() << fatal_error;
   return *fileStream;
}


/***************************************************************************
  Description : Set the buffer for the stream; this should be the first operation
                  on the stream.
  Comments    : fstream.rdbuf() does not return anything. rdbuf->setbuf()
                  returns 0 on failure, which means that a buffer has already
		  been allocated, this is why the check is done below.
		  See main() in the tester.
***************************************************************************/
void MLCIStream::setbuf(char *p, int l)
{
   if (get_fstream().rdbuf()->setbuf(p, l) == 0) {
      err << "MLCIStream::setbuf: buffer already allocated. "
	     " setbuf() cannot execute after an output statement "
          << description() << fatal_error;
   }
  OK();
}

/***************************************************************************
  Description : Extracts a character from the stream and stores in
                  the characterand returns the stream.
  Comments    : Aborts if extraction encounters end of file.
***************************************************************************/
MLCIStream& MLCIStream::get(char& c)
{
   get_stream().get(c);      // OC bug,if changed to stream.get() -fails
   if (get_stream().eof())   // OC bug,if changed to stream.eof() -fails
      err << "MLCIStream::get: Tried to get EOF in stream "
	  << description() << fatal_error;
   if (get_stream().fail())
      err << "MLCIStream::get: Error reading char from stream " 
	  << description() << fatal_error;
   OK();
   return *this;
}


/***************************************************************************
  Description : Extracts and throws away up to len characters.  Extraction
                  stops prematurely if delim is extracted or end of file
		  is reached.
  Comments    : Do NOT use this for len=1 because it's rather slow.
                Use get() instead.
***************************************************************************/
void MLCIStream::ignore(int len, int delim)
{
   get_stream().ignore(len, delim);
   OK();
}




/***************************************************************************
  Description : Returns the description of the stream.
  Comments    :
***************************************************************************/
const MString& MLCIStream::description() const
{
   return descr;
}


/***************************************************************************
  Description : Extracts the white space on a stream. Terminates when anything
                  other than whitespace is encountered.
  Comments    :
***************************************************************************/
void MLCIStream::skip_white()
{
   char ch = get_fstream().peek(); // char in order for isspace() to work
   while( isspace(ch) && !get_fstream().eof() ) {
      get_fstream().get(ch);
      if( !isspace(ch) ) { 
	 get_fstream().putback(ch);
	 break;
      }
   }
}


/***************************************************************************
  Description : Reads in a type saved as a sequence of n byte values
  Comments    : See Macro definition at the header of this file.
               
***************************************************************************/
DEF_READ_BIN(int);
DEF_READ_BIN(char);
DEF_READ_BIN(unsigned char);
DEF_READ_BIN(long);
DEF_READ_BIN(float);
DEF_READ_BIN(short);
DEF_READ_BIN(double);


/***************************************************************************
  Description : The following methods provide extraction from an MLCIStream
                  analogous to the operator >>'s defined for istream in 
		  iostream.h.
  Comments    : The extractor for char* is handled by conversion to 
                  MString.  ( see MString.h for the definition of the 
		  apropriate operator>> )
***************************************************************************/
DEF_OPERATOR_GG(int&);
DEF_OPERATOR_GG(char*);
DEF_OPERATOR_GG(char&);
DEF_OPERATOR_GG(long&);
DEF_OPERATOR_GG(short&);
DEF_OPERATOR_GG(float&);
DEF_OPERATOR_GG(unsigned char&);
DEF_OPERATOR_GG(unsigned short&);
DEF_OPERATOR_GG(unsigned int&);
DEF_OPERATOR_GG(unsigned long&);


/***************************************************************************
  Description : Returns TRUE iff the given character is a legal first
                  character for a real number.
  Comments    :
***************************************************************************/
static Bool valid_real_prefix(char ch)
{
   switch (ch) {
   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':
   case '+':
   case '-':
   case '.':
      return TRUE;
   default:
      return FALSE;
   }
}


/***************************************************************************
  Description : Safely reads a Real from an MLCIStream.
  Comments    : Note that it must skip over white space in order for
                  this to work. 
***************************************************************************/
MLCIStream& MLCIStream::operator>>(Real& num)
{
   skip_white();                   //skip the white space, if it exists.
   int ch = get_fstream().peek();   

   if (ch == EOF)
      err << "MLCIStream::operator>>(Real&): unexpected EOF" << fatal_error;
   
   if ( !valid_real_prefix(ch) )   //and then check the prefix.
      err << "MLCIStream::operator>>(Real&): '" << ch
	  << "' is not a valid real prefix.  File: " 
	  << description() << fatal_error;
   
   stream >> setprecision(REAL_MANTISSA_LEN) >> num;
   if (fabs(num) < REAL_EPSILON)
      num = 0; // Sometimes we have -0.00 (as in waveform.data) which causes
	       // problems in matches with 0 (t_TableCasInd fails
               // to diff randomly because of this).
   return *this;
}


/******************************************************************************
  Description : operator>> for standard manipulators.  These permit the
                 manipulators defined for istream to be passed
		 to MLCIStreams.  
  Comments:
******************************************************************************/
MLCIStream& MLCIStream::operator>>(istream& (*manipulator)(istream&))
{
   VERIFY_ISTREAM(*this, manipulator);
}


/***************************************************************************
  Description : Extraction operator for int valued (SMANIP) manipulators.
  Comments    : friend.
***************************************************************************/
#if defined(CFRONT) || defined(IRIX)
MLCIStream& operator>>(MLCIStream& s, const SMANIP(int)& m)
#elif defined(GNU)
MLCIStream& operator>>(MLCIStream& s, const smanip<int>& m)
#else
#error "Compiler not supported"
#endif
{
   VERIFY_ISTREAM(s, m);
}

/***************************************************************************
  Description : Helper function for constructing the MLCIStream.
  Comments    : Factored code from c-tors. Note that this code changes
                  the important constants in the code (i.e. to a 1-based
		  position in line, 1 based line count).
***************************************************************************/
void MLCOStream::init()
{
   set_width(DEFAULT_WRAP_WIDTH);
   atLeftmostPosition = TRUE;
   closed = FALSE;
   lineCount = linePos = 1;
}

/***************************************************************************
  Description : Attach
  Comments    : Compiler dependent
***************************************************************************/

void MLCOStream::attach(int fd)
{
   get_fstream().rdbuf()->attach(fd);
}


/***************************************************************************
  Description : This method is called by output_format_stream() when a
                  space is to be put on the stream.
  Comments    : Eats space when the stream wraps.
***************************************************************************/
void MLCOStream::print_trailing_space()
{
   if (++linePos <= wrap_width())
      stream << ' '; 
   else {                         // wrap, but eat space
      gen_newline(); 
      stream << get_wrap_prefix();
      linePos = wrap_prefix_length() + 1;
   }
}


/***************************************************************************
  Description : This method checks to see whether there is a '\n' embedded
                  in a string to be output on the stream. It handles the
		  first half of the wrapping algorithm by removing
		  newlines in the stream and calling
		  output_format_stream_no_newline() for each of the
		  substrings which it preprocesses.
		  
  Comments    : It relies upon the helper function
                  output_format_stream_no_newline().
***************************************************************************/
void MLCOStream::output_format_stream(Bool flushOutput)
{
   if (!formatStream.good()) {
      formatStreamBuf[MAX_BUFFER_SIZE - 1] = 0;
      Mcerr << "MLCOstream::output_format_stream: buffer overflow on "
	   << description() << ".\nContinuing but some output was "
	      " probably lost. Increase MLCOStream::MAX_BUFFER_SIZE."<< endl
	   << formatStreamBuf << endl;
      formatStream.clear();
      formatStream.seekp(0);
      return;
   }
      
   long outputStringLength = formatStream.tellp();
   if(outputStringLength == 0)
      return;

   // By default, the formatStream does not have a terminal NULL.
   // Note: Do not use formatStream << '\0' because it will reset
   //       the previous manipulator.
   formatStreamBuf[outputStringLength]= '\0';
   if (outputStringLength != strlen(formatStreamBuf))
      err << "MLCOStream::output_format_stream: Output string contains "
	     " null characters '" << formatStreamBuf << "'"
	 << fatal_error;
   
   char* str = formatStreamBuf;
   char* newLinePtr;
   while( (newLinePtr = strchr(str,'\n')) != NULL) {
      *newLinePtr = '\0'; 
      int printLen = newLinePtr - str; 
      output_format_stream_no_newline(TRUE, printLen);
      // For example, "test\n\n\ntest" should output newlines with appropriate
      // prefixes.
      if(atLeftmostPosition)
	    stream << get_newline_prefix();
      gen_newline();
      //use overlap_byte_copy since the src and dest overlap
      outputStringLength -= printLen + 1;
      overlap_byte_copy(formatStreamBuf, newLinePtr + 1,
			(int)outputStringLength + 1);
   }

   // If output_format_stream_no_newline() returned TRUE,
   // we have to seek to zero (otherwise it did this for us).
   if(output_format_stream_no_newline(flushOutput, outputStringLength))
      formatStream.seekp(0);
}


/***************************************************************************
  Description : This method is called by output_format_stream() above.
                It first calculates the width of the stream to
  		  be output; if the position of the stream is leftmost in the
		  stream, it outputs the newline prefix, otherwise it wraps
		  according to the rules at the header of this file. Note that
		  the input which it receives is actually a substring of the
		  entire output which was put on the stream.
		If flushOutput is true, we do not buffer the input.
  Comments    : Assumes that there are no newlines in the formatStream.
                If this method returned false, then it already seeked and
		  flushed the output. 
***************************************************************************/
Bool MLCOStream::output_format_stream_no_newline(Bool flushOutput,
						 long outputStringLength)
{
   if (outputStringLength == 0)
      return TRUE;

   // pos = 0 in regular C++ streams, put out the newline prefix
   if (atLeftmostPosition) {
      ASSERT(1 == linePos);
      stream << get_newline_prefix();
      linePos = newline_prefix_length() + 1;
      atLeftmostPosition = FALSE;
   }

   Bool printSpace;
   char *str = formatStreamBuf;
   do {
      char *spacePtr = str;
      while (*spacePtr == ' ' && *(spacePtr + 1) != '\0'){
	 spacePtr++;
	 ASSERT(spacePtr != TAB);        //@@ May cause some testers to fail.
      }
      spacePtr = strchr(spacePtr, ' ');   // space or terminating NULL
      int printLen;
      if (spacePtr == NULL) {
	 printLen = strlen(str);
	 if (!flushOutput) {              // Buffer input.  Do NOT print
	    if (str != formatStreamBuf) { // Shift input left
	       overlap_byte_copy(formatStreamBuf, str, printLen + 1);
	       ASSERT(strlen(formatStreamBuf) == printLen);
	    }
	    formatStream.seekp(printLen); // Valid even for else part because
					  // it could be "foo\nbar"
	    return FALSE;
	 }
	 printSpace = FALSE;
      } else {
	 *spacePtr = NULL; 	          // point to terminating NULL
	 printSpace = TRUE;
      	 printLen = spacePtr - str;
      }
      
      ASSERT(strlen(str) == printLen);
      linePos += printLen;                // New linePos if we can fit it.
      
      // See if the text (without trailing space) fits on the line
      if (linePos <= wrap_width() + 1) {  
	 stream << str;
	 if (printSpace)                  // See if the trailing space fits 
	   print_trailing_space();
      } else {                            // We need to wrap

         // If we're at leftmost, or linePos == 1, don't generate a newline,
	 //   because it's clearly superfluous
	 if (!atLeftmostPosition && linePos > 1) {
	    gen_newline();
	    stream << get_wrap_prefix();
	    linePos = wrap_prefix_length() + printLen + 1;
	 } // Else it's too big and we just print it no matter what.
	    stream << str;
	    if (printSpace)
	       print_trailing_space();
      }
      atLeftmostPosition = FALSE;
      str += printLen + printSpace;
   } while (str < formatStreamBuf + outputStringLength);
   atLeftmostPosition = FALSE;
   return TRUE;
}



/***************************************************************************
  Description : Sets the line width for output.
  Comments    :
***************************************************************************/
void MLCOStream::set_width(int newWidth)
{
   output_format_stream(TRUE);
   width = newWidth;
   OK(CheckPrefixes);
}


/***************************************************************************
  Description : Sets the prefix which prints after every automatic line
                  wrap.
  Comments    :Aborts if the wrap prefix is larger than the current line
                  width set by set_width(). 
***************************************************************************/
void MLCOStream::set_wrap_prefix(const MString& str)
{
   wrapPrefix = str;
   OK(CheckPrefixes);
}


/***************************************************************************
  Description : Sets the prefix which prints before every newline.
  Comments    : Aborts if the newline prefix is larger than the current
                  line width set by set_width(). 
***************************************************************************/
void MLCOStream::set_newline_prefix(const MString& str)
{
   newLinePrefix = str;
   OK(CheckPrefixes);
}

/***************************************************************************
  Description : Sets the wrap width, newline prefix and wrap prefix for the
                  stream using the MStreamOptions structure; flushes output.
  Comments    : 
***************************************************************************/
void MLCOStream::set_options(const MStreamOptions& options)
{
   output_format_stream(TRUE); //flush, dumping current output
   wrapPrefix = options.wrapPrefix;
   newLinePrefix = options.newLinePrefix;
   width = options.width;
}


/***************************************************************************
  Description : Makes an options structure to store the current wrap width,
                  wrap prefix and newline prefix for the user.
  Comments    : 
***************************************************************************/
MStreamOptions MLCOStream::get_options() const
{
   MStreamOptions currentOptions;
   currentOptions.wrapPrefix = wrapPrefix;
   currentOptions.newLinePrefix = newLinePrefix;
   currentOptions.width = width;
   return currentOptions;
}


/***************************************************************************
  Description : Calls check_ostream() for level 0 check, checks prefixes
                  under level 1 check.
  Comments    : At least one stream must be NULL, but it may be that
                  both are NULL.
		Setting wrap prefixes greater than the width is an error.
		Setting the width less than the wrap prefix is an error.
***************************************************************************/
void MLCOStream::OK(int level) const
{
   if (level == 0) {
      ASSERT(strStream == NULL  ||  fileStream == NULL);
      ASSERT(fileStream == NULL || &stream == fileStream);
      ASSERT(strStream  == NULL || &stream == strStream);

      if (outputType != XStream && !closed) 
	 check_ostream(stream, description());
   }
   if (level == 1) {
      if(width != 0)
	 if(width < get_wrap_prefix().length()    ||\
	    width < get_newline_prefix().length() ||\
	    get_newline_prefix().length() > width ||\
	    get_wrap_prefix().length() > width)
	    err << "MLCOStream::OK() : " << description()
		<< " now has invalid wrap options since the width is " << width
		<< " and the prefixes are <" << get_wrap_prefix() <<","
		<< get_newline_prefix() << "> " << fatal_error;
   }
}


/***************************************************************************
  Description : Inserts a newline on the stream, updates the line-count and
                  resets the position in line.
  Comments    : Callers should update atLeftmostPosition if this function has
                  been called because of a C++ stream newline.
		  See description in the header at the top of this file.
***************************************************************************/
void MLCOStream::gen_newline()
{
   stream << endl; 
   lineCount++;
   linePos = 1;
   atLeftmostPosition = TRUE;
}


/***************************************************************************
  Description : Constructor. Specifies an OutputType
  Comments    : This currently is only valid for XStream
  ***************************************************************************/
MLCOStream::MLCOStream(const OutputType otype)
    : outputType(otype),
      descr("XStream"),
      strStream(NULL),
      fileStream(NULL),
      stream(*(ostream *)NULL_REF),
      formatStream(formatStreamBuf, sizeof(formatStreamBuf)),
      memBuf(0),
      wrapPrefix(DEFAULT_WRAP_PREFIX)
{
   init();
   if (otype != XStream)
       err << "MLCOStream::MLCOStream(OutputType): invalid OutputType of "
           << otype << " , must be XStream" << fatal_error;
}


/***************************************************************************
  Description : Constructs an MLCOStream to dump all output to an strstream.
                This is used so that all IO goes into the users char* buffer.
                Note that char* buff is NOT NULL terminated, and the
	          user must append a NULL before using it as a string, i.e.,
		  stream << '\0'
  Comments    : See get_strstream(),mem_buf()
  ***************************************************************************/
MLCOStream:: MLCOStream(const MString& description, char* buf, int nBytes)
    : outputType(MemStream),
      descr(description),
      strStream(new ostrstream(buf, nBytes)),
      fileStream(NULL),
      stream(*strStream),
      formatStream(formatStreamBuf, sizeof(formatStreamBuf)),
      memBuf(buf),
      wrapPrefix(DEFAULT_WRAP_PREFIX)
{
   init();
}


/***************************************************************************
  Description : Opens an ofstream with the given file name.  If append is
                  true, opens in the append mode; otherwise opens in the
		  out mode which overwrites existing files with the same
		  name.  See ios for details on prot.
  Comments    : Stream is closed by the destructor.
***************************************************************************/
MLCOStream::MLCOStream(const MString& fileName, const OutputType otype,
		       Bool append, int prot) 
    : outputType(otype),
      descr(fileName),
      strStream(NULL),
      fileStream(new ofstream()),
      stream(*fileStream),
      formatStream(formatStreamBuf, sizeof(formatStreamBuf)),
      memBuf(0),
      wrapPrefix(DEFAULT_WRAP_PREFIX)
{ 
   init();
   if (otype == XStream)
      err << "MLCOStream::MLCOStream(const MString& const OutputType,"
	     "const Bool, int prot): Xstream is invalid OutputType"
             " for this constructor [" << description() <<  "]"<< fatal_error;
  
  check_description(fileName);
  if (append) 
    get_fstream().open(fileName, ios::app, prot);
  else 
    get_fstream().open(fileName, ios::out, prot);
  if (!get_fstream().good())
    err << "MLCOStream::MLCOStream(const MString&, const Bool, int): Could "
           "not open file " << fileName << fatal_error;

}


/***************************************************************************
  Description : Attaches the given file to the MLCOStream.
  Comments    : User must close the stream that was passed AFTER the
                  final write to MLCOStream.
***************************************************************************/
MLCOStream::MLCOStream(const MString& dscr, ofstream& strm,
		       const OutputType otype)
   : outputType(otype),
     descr(dscr),
     strStream(NULL),
     fileStream(new ofstream()),
     stream(*fileStream),
     formatStream(formatStreamBuf, sizeof(formatStreamBuf)),
     memBuf(0),
     wrapPrefix(DEFAULT_WRAP_PREFIX)
{
   init();
   if (otype == XStream)
       err << "MLCOStream::MLCOStream(const MString&, ofstream&,"
	      "const OutputType): Xstream is invalid OutputType "
           << description() << fatal_error;
   check_description(dscr);
   attach(strm.rdbuf()->fd());
   OK();
}


/***************************************************************************
  Description : Attaches the given stream to the MLCOStream.
  Comments    : This works only for cout (standard output), 
                  cerr (standard error, unit buffered), and 
		  clog (standard error, buffered).
		  The destructor will NOT close these streams.
***************************************************************************/
MLCOStream::MLCOStream(ostream& strm, const OutputType otype)
    : outputType(otype),
      descr((strm == cout) ? "standard output (cout)":"standard error"),
      strStream(NULL),
      fileStream(new ofstream()),
      stream(*fileStream),
      formatStream(formatStreamBuf, sizeof(formatStreamBuf)),
      memBuf(0),
      wrapPrefix(DEFAULT_WRAP_PREFIX)
{
   init();
   if (otype == XStream)
      err << "MLCOStream::MLCOStream(ostream&, const OutputType):"
	     "Xstream is invalid OutputType " << description() << fatal_error;
   
   if (strm == cout)
      attach(1);
   else if (strm == cerr || strm == clog)
      attach(2);
   else
    err << "MLCOStream::MLCOStream(ostream&, const OutputType): Unknown"
           "ostream " << description() << fatal_error;
   OK();
}


/***************************************************************************
  Description : Attaches the stream with the given file descriptor to the 
                  MLCOStream.
  Comments    : The destructor will NOT close the associated stream.
                If in doubt about a file descriptor, use one of the other 
		  constructors.
***************************************************************************/
MLCOStream::MLCOStream(int fd, const MString& dscr, const
		       OutputType otype)
    : outputType(otype),
      descr(dscr),
      strStream(NULL),
      fileStream(new ofstream()),
      stream(*fileStream),
      formatStream(formatStreamBuf, sizeof(formatStreamBuf)),
      memBuf(0),
      wrapPrefix(DEFAULT_WRAP_PREFIX)
{
   init();
   if (otype == XStream)
      err << "MLCOStream::MLCOStream(int, const MString&,"
	  "const OutPutType): Xstream is invalid OutputType"
	  << description() <<  fatal_error;
   check_description(dscr);
   attach(fd);
   OK();
}



/***************************************************************************
  Description : Closes the stream; checks that stream is still good.
  Comments    : The user must still close the stream which was passed using
                  constructors other than 
		  MLCOStream::MLCOStream(MString&, Bool&, int)
***************************************************************************/
MLCOStream::~MLCOStream()
{
   if (wrap_width() != 0) 
      output_format_stream(TRUE); // Flush
   OK();                      
   if (this != &err && this != &Mcerr && this != &Mcout) {
      if (outputType != XStream && fileStream && !closed) 
         close();
      if (outputType != XStream)
         delete &stream;
   }
}


/***************************************************************************
  Description : Closes the stream.
  Comments    : Does not close the underlying file descriptor if it was
                  supplied with attach(), currently close is defined
		  only for file based streams.
***************************************************************************/
void MLCOStream::close()
{
   if(closed)
      err << "MLCIStream::close: Stream "<< description()
	  << " has already been closed " << fatal_error;
   if(fileStream){
      get_fstream().close();  
      if (!get_fstream().good())
	 err << "MLCOStream::close: Error occured closing stream" 
	     << fatal_error;
      closed =TRUE;
   }
   else
      err << " MLCOStream::close: undefined operation for this stream"
	  << fatal_error;
}


/***************************************************************************
  Description :  Checks that the stream is not closed, aborts if so.
  Comments    :
***************************************************************************/
void MLCOStream::check_stream_is_valid() const
{
   if(closed)
      err << "MLCOStream::check_close: attempted operation on a closed stream"
	  << fatal_error;
}


/***************************************************************************
  Description :  Returns the fstream associated with a given MLCOStream.
  Comments    :  This is used by member functions which operate on the
                   stream as if it were a disk-file object.
***************************************************************************/
ofstream& MLCOStream::get_fstream()
{
   if( fileStream == NULL)
      err << "MLCOStream::get_fstream -fileStream has not been set ... "
	  << fatal_error;
   return *fileStream;
}


/***************************************************************************
  Description : Returns the strstream associated with a given MLCOStream.
  Comments    : This is used by the member functions which operate on
                  the object so as to do IO in memory.
***************************************************************************/
ostrstream& MLCOStream::get_strstream()
{
   if (strStream == NULL)
      err << "MLCOStream::get_strStream - strStream has not been set ..."
	  << fatal_error;
   return *strStream;
}

/***************************************************************************
  Description : Returns the line count in the file.
  Comments    : Assumes that wrapping is on, aborts if it is not.
***************************************************************************/
int MLCOStream::line_count() const
{
   if (width == 0)
      err << "MLCOStream::line_count(): Wrapping is not on for stream "
	  << description() << " please set the wrap width > 0 before invoking"
	  " this method." << fatal_error;
   return lineCount;
}


/***************************************************************************
  Description : Flushes output, returns the position of the character about to
                  print in the file. For example
		  " stream << "xy" << stream.pos_in_line() "
	          will print "xy3"
  Comments    : If wrapping is not on, a call to this will abort.
***************************************************************************/
int MLCOStream::pos_in_line()
{
   if(!wrap_width())
      err << "MLCOStream::pos_in_line : wrapping is not on." << fatal_error;
   get_stream() << flush;
   return linePos;
}

/***************************************************************************
  Description : Set the buffer for the stream.  See setbuf() in fstream
                   class.
  Comments    : fstream.rdbuf() does not return anything. rdbuf->setbuf()
                  returns 0 on failure, which means that a buffer has already
		  been allocated, this is why the check is done below.
***************************************************************************/
void MLCOStream::setbuf(char *p, int l)
{
   if (get_fstream().rdbuf()->setbuf(p, l) == 0) {
      err << "MLCOStream::setbuf: buffer already allocated. "
	 " setbuf() cannot execute after an output statement"
          << fatal_error;
   }
   OK();
}


/***************************************************************************
  Description : Returns the description of the stream.
  Comments    :
***************************************************************************/
const MString& MLCOStream::description() const
{
   return descr;
}


/***************************************************************************
  Description : Returns an MString constructed from the current state of the
                  buffer which is associated with an MLCOStream memory stream.
		  i.e. an output stream constructed as an strstream.
  Comments    : This operation is UNDEFINED and will abort if the output_type
                  of the stream is not a MemStream.
***************************************************************************/
MString MLCOStream::mem_buf()
{
   if(output_type() != MemStream)
      err << "MLCOStream::mem_buf -undefined operation for stream "
	  << description() << fatal_error;
   return MString(memBuf,get_strstream().pcount());
}


/***************************************************************************
  Description : Returns the output type of the stream.
  Comments    :
***************************************************************************/
OutputType MLCOStream::output_type() const
{
   return outputType;
}


/***************************************************************************
  Description : Take the file passed in as a parameter and appends it to
                  the receiver's stream.
  Comments    : Buffer size is defined in basics.h
***************************************************************************/
void MLCOStream::include_file(const MString& filename)
{
   MLCIStream readStream(filename);
   char buf[IO_BUFFER_SIZE];       // Defined in basics.h
   
   while (!readStream.get_stream().fail()) { 
      readStream.get_stream().read(buf, IO_BUFFER_SIZE);
      stream.write(buf, readStream.get_stream().gcount());
   }
   readStream.get_stream().clear();
}


/***************************************************************************
  Description : Writes out a value as a sequence of bytes
  Comments    : See macro at top of this source file.
**************************************************************************/
DEF_WRITE_BIN(int);
DEF_WRITE_BIN(char);
DEF_WRITE_BIN(unsigned char);
DEF_WRITE_BIN(long);
DEF_WRITE_BIN(float);
DEF_WRITE_BIN(short);
DEF_WRITE_BIN(double);


/***************************************************************************
  Description : The following methods provide insertion to the MLCOStream
                  analogous to the operator <<'s defined for ostream in 
	          iostream.h
  Comments    : See macro at the header of this source file.
***************************************************************************/
DEF_OPERATOR_LL(int);
DEF_OPERATOR_LL(char);
DEF_OPERATOR_LL(long);
DEF_OPERATOR_LL(short);
DEF_OPERATOR_LL(float);
DEF_OPERATOR_LL(void *);
DEF_OPERATOR_LL(double);
DEF_OPERATOR_LL(const char*);
DEF_OPERATOR_LL(unsigned int);
DEF_OPERATOR_LL(unsigned char);
DEF_OPERATOR_LL(unsigned short);
DEF_OPERATOR_LL(unsigned long);


/******************************************************************************
  Description : operator<< for standard manipulators.  These permit the
                  manipulators defined for ostream to be passed
		  to MLCOStreams.  
  Comments:     If wrapping is on, and the manipulator is endl, it resets
                  the line count to be 0.
******************************************************************************/
MLCOStream& MLCOStream::operator<<(ostream& (*manipulator)(ostream&))
{
   if (wrap_width() == 0) {
      VERIFY_OSTREAM(*this, manipulator);
   }
   else {
      if(manipulator == endl){
	 output_format_stream(TRUE); // Flush our internal buffer
	 if(atLeftmostPosition)
	    stream << get_newline_prefix();
	 check_ostream(stream,description());
	 gen_newline();
      }
      else if (manipulator == ends || manipulator == flush) {
	 output_format_stream(TRUE); // Flush our internal buffer
	 stream << manipulator;
	 check_ostream(stream,description());
      }
      else {
	 formatStream << manipulator;
	 if (!formatStream.good())
	    err << "MLCOStream::operator << (manip) : bad formatStream"
	        << " status" << fatal_error;
      }
   }
   return *this;
}

/***************************************************************************
  Description : Operator << fro MLCOStream manipulators
  Comments    :
***************************************************************************/
MLCOStream& MLCOStream::operator<<(MLCOStream& (*manipulator)(MLCOStream&))
{
   return manipulator(*this);
}


/***************************************************************************
  Description : Defines the insertion operator for manipulators with
                  int parameters (i.e. .. << setwidth(10) << .. )
  Comments    : friend.
***************************************************************************/
#if defined(CFRONT) || defined(IRIX)
MLCOStream& operator<<(MLCOStream& stream, const SMANIP(int)& manip)
#elif defined(GNU)
MLCOStream& operator<<(MLCOStream& stream, const smanip<int>& manip)
#else
#error "Compiler not supported"
#endif
{
   // This takes care of applying the manipulator to the formatStream
   if(stream.wrap_width()){
         stream.formatStream << manip;
	 if (!stream.formatStream.good())
	    err << "MLCOStream:: operator<<() : bad status"
		<< " on stream " << stream.description() << fatal_error;
   } else {
      VERIFY_OSTREAM(stream, manip);
   }
   return stream;
}


/***************************************************************************
  Description : Manipulator for reset_pos_in_line.
  Comments    : 
***************************************************************************/
MLCOStream& reset_pos_in_line(MLCOStream& stream)
{
   return stream.reset_pos_in_line(), stream;
}











