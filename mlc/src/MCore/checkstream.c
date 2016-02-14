// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Provide safe input/output routines.
                 Fatal_error is called in case the stream is not good.
  Assumptions  :
  Comments     : Usage: 
                   cout << ... << check_cout;
                   cin  >> ... >> check_cin;
                   ostream << ... << check_ostream(fileName);
                   ofstream << ... << check_ostream(fileName);
                   istream >> ... >> check_istream(fileName);
                   ifstream >> ... >> check_istream(fileName);
                 If you use other members, you should call
                   check_cout(cout); check_cin(cin);
                   check_ostream(stream, fileName);
                   check_istream(stream, fileName);

                 The code may not be very portable since it uses
                   CFRONT macros.  It is based on the Iostream
                   Examples on page 3-20 of CenterLine's AT&T C++
                   Language system library manual.
		 Note that stream.clear() is called after a bad
  		   operation,  because otherwise, if the program
		   exits through exit(), the destructors get called
		   and they call OK() which causes a fatal_error loop
		   (just a double message, but this is nicer).
		 
  Complexity   : 
  Enhancements : Provide more informative error reporting when there
                   is an error (e.g. what exactly happened).  This is
                   probably very hard since C++ doesn't tell us much.
                 The error reporting on cout may be meaningless if
                   cerr is corrupted too (unless fatal_error dumps the
                   messages to a file too).
  History      : Ronny Kohavi                                       7/28/93
                   Initial revision

***************************************************************************/

#include <basics.h>
#include <checkstream.h>
#include <MLCStream.h>

RCSID("MLC++, $RCSfile: checkstream.c,v $ $Revision: 1.14 $")


void check_ostream(ostream& stream, const char* fileName)
{
   if (!stream.good()) {
      if (stream == err.get_stream())
         cerr << "\n\ncheck_ostream: error buffer overflowed.  Possible cause "
	      << "is a message of the form err << msg << endl instead of "
	      << " err << msg << fatal_error." << endl
	      << "Outputing partial error message: " << endl;
      stream.clear();
      err << "check_ostream:Unable to write to file " << fileName 
	  << fatal_error;
   }
}

void check_cout(ostream& stream)
{
   if (stream != cout)
      err << "check_cout: stream != cout" << &fatal_error;
   
   if (!stream.good()) {
      int state = stream.rdstate();
      stream.clear();
      err << "check_cout: Bad state for cout (state=" << state
          << ')' << fatal_error;
   }
}


void check_cin(istream& stream)
{
   if (stream != cin)
      err << "check_cin: stream != cin" << fatal_error;
   
   if (!stream.good()) {
      int state = stream.rdstate();
      stream.clear();
      err << "Bad state for cin (state=" << state << ')' << fatal_error;
   }
}

void check_istream(istream& stream, const char* fileName)
{
   if (!stream.good())
      if (stream.eof()) {
	 stream.clear();
         err << "check_istream:Attempted read after EOF on file " <<
            fileName << fatal_error;
      } else {
	 stream.clear();
         err << "check_istream:Unable to read from file " << fileName <<
            fatal_error;
      }
}   
   



