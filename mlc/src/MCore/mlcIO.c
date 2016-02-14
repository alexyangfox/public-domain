// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Provides input/ouput routines that are used by the classes
                   of the MLC++ library.
  Assumptions  : 
  Comments     : 
  Complexity   : skip_white_comments() takes time proportional to the amount 
                   of whitespace and comments that it skips.
		 read_word() and read_word_same_line() take time proportional
		    to the number of chars in the word + the amount of 
		    whitespace skipped.
  Enhancements : Write t_mlcIO.c (assuming it works because it was copied
                   from BagSet.c, where it was tested)
                 Have file_exists() that searches MLCPATH and returns
		   where it was found.  Currently file_exists() can be false,
		   but opening it will work.
  History      : Richard Long                                       1/21/94
                   Added operator>>(MLCIstream&, Real)
                 Richard Long                                       8/06/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <stdio.h>
#include <mlcIO.h>
#include <ctype.h>
#include <string.h>
#include <iomanip.h>
#include <errno.h>
#include <sysent.h>    // Required for unlink.
#include <MStringRC.h>

RCSID("MLC++, $RCSfile: mlcIO.c,v $ $Revision: 1.36 $")

/***************************************************************************
  Description : Causes a fatal error if the character is EOF.
                Returns TRUE if the characters is legal in a name,
                  which is if the character is not a , . | : '\n'
		A character prefixed by \ is always valid.
		Returns FALSE otherwise.
  Comments    :
***************************************************************************/
Bool legal_attr_char(MLCIStream& stream, int line, char& c, 
		     Bool periodAllowed)
{
   int ch = stream.peek();
   c = ch; // may lose EOF here (actually happens on SGI!)
   switch (ch) {
   case '\\':
      return TRUE;
   case EOF:
      check_istream(stream.get_stream(), stream.description());
      err << "mlcIO.c::legal_attr_char: Unexpected end of file on line "
  	  << line << " of " << stream.description() << fatal_error;
   case ',':
   case ':':
   case '|':
   case '\n': 
     return FALSE;
   case '.':
     return (periodAllowed);	 
   default:
     return TRUE;
  }
}


/***************************************************************************
  Description : Skips past whitespace and the remaining characters on a line
                  following a '|', which indicates the start of a comment.
		Updates line counter.
  Comments    : Does nothing if EOF is encountered.
***************************************************************************/
void skip_white_comments(MLCIStream& stream, int& line) 
{
   char c = stream.peek();
   while (isspace(c) || c == '|') {
    if (c == '|') {
      stream.ignore(INT_MAX, '\n');       // ignore rest of line
      line++;
    } else {
      if (c == '\n')
	line++;
      stream.ignore(1);                   // ignore white_space
    }
    c = stream.peek();
  }
}


/***************************************************************************
  Description : Performs skip_white_comments() and returns TRUE if no
                  no newline was encountered, and FALSE otherwise.
  Comments    : Depends on updating of line by skip_white_comments.
		Takes time proportional to the amount of whitespace and 
		  comments that it skips.
***************************************************************************/
Bool skip_white_comments_same_line(MLCIStream& file, int& line)
{
  int startLine = line;
  skip_white_comments(file, line);
  return (line == startLine);
}


/***************************************************************************
  Description : Returns the next "word" in the stream.  A "word" consists of 
                   a sequence of "legal" (given by legal_attr_char) 
		   characters.  A word also cannot be "?" if
		   qMark is FALSE.
		PeriodAllowed is to allow reading continuous vars,
		  in which case empty strings are OK, since they're zero.
  Comments    : Removes trailing white space, and compresses included
                   whitespace to a single space.
                This function uses a static array of characters
                  instead of MString, because profiling discovered
                  that allocating/deallocating strings every time a
                  new characters is added was too expensive.
***************************************************************************/

// inc_word_len increments the wordLen and checks for overflow.
static void inc_word_len(char *word, int& wordLen)
{
   if (++wordLen > MAX_INPUT_STRING)
      err << "mlcIO::read_word: word overflow.  More than "
          << MAX_INPUT_STRING << " characters for word "
          << MString(word, MAX_INPUT_STRING) << fatal_error;
}
         

MStringRC read_word (MLCIStream& stream, int& line, Bool qMark,
		     Bool periodAllowed)
{
   static char word[MAX_INPUT_STRING + 1];
   int wordLen = 0;
   skip_white_comments(stream, line);
   Bool whitespace = FALSE;
   char c;

   while (legal_attr_char(stream, line, c, periodAllowed)) {
      // already checked for EOF
      if (c == ' ' || c == '\t') {
         whitespace = TRUE;
         stream.ignore(1);      // ignore white space
      } else {
         if (whitespace) {
            word[wordLen] = ' ';
            inc_word_len(word, wordLen);
            whitespace = FALSE;
         }
         word[wordLen] = stream.get();
	 if (word[wordLen] == '\\')
	    word[wordLen] = stream.get(); // get next character
         inc_word_len(word, wordLen);
      }
      // indicates problem w/legal_attr_char()
      check_istream(stream.get_stream(), stream.description());
   }
   if (wordLen < 1 && !periodAllowed)
      err << "mlcIO.c::read_word: unable to read word.  Perhaps you "
	 " forgot to supply it on line " << line << " of "
	 << stream.description() << fatal_error;
   if (!qMark && wordLen == 1 && word[0] == '?')
      err << "mlcIO.c::read_word: illegal name '?' on line " 
      << line << " of " << stream.description() << fatal_error;
   return MStringRC(word, wordLen);
}


/***************************************************************************
  Description : Returns next word on same line or causes fatal_error
                  if no more words exist
  Comments    : See read_word() for description of a "word".
                Depends on updating of line by read_word.
		Necessary to ensure proper file format in read_data_line.
		Takes time proportional to the number of chars in the word.
***************************************************************************/
MStringRC read_word_on_same_line(MLCIStream& stream, int& line, Bool qMark,
				 Bool periodAllowed)
{
  int startLine = line;
  MStringRC word = read_word(stream, line, qMark, periodAllowed);
  if (startLine != line)
    err << "mlcIO.c:: read_word_on_same_line: Another word expected on line "
	<< startLine << " of " << stream.description() << fatal_error;
  return word;
}


/***************************************************************************
  Description : Returns the name of a unique temporary file.
  Comments    : 
  ***************************************************************************/
MString get_temp_file_name() {
   return MString(tmpnam(NULL)) + ".MLC";
}

/***************************************************************************
  Description : Remove a file (safe)
  Comments    :
***************************************************************************/
void remove_file(const MString& file)
{
   if (file == "")
      err << "remove_file: empty file name" << fatal_error;

   errno = 0;
   if (unlink(file) != 0) {
      cerr << "Warning: remove_file: error in removing file " << file
          << " errno=" << errno << ". Continuing" << endl;
      perror("Explanation of errno");
   }
}


/***************************************************************************
  Description : Return the full filename that can be used to open
                   the file for reading.  We attempt to prepend the
                   colon-separated paths in MLCPATH in the given order
                   until the file is found.  If it is not found, we
                   return the empty string.
		If the given file does not exist, it returns "";
  Comments    :  
***************************************************************************/
MString file_exists(const MString& str, Bool fatalOnFalse)
{
   fstream stream;

   if (str == "")
      if (fatalOnFalse)
	 err << "mlcIO::file_exists: empty file name" << fatal_error;
      else
	 return "";

   if (str[0] == '/') {
      stream.open(str, ios::in);
      if (!stream.good()) {
	 if (fatalOnFalse)
	    err << "mlcIO::file_exists: Could not open file '"
		<< str << "' (absolute path)" << fatal_error;
	 else
	    return "";
      }
      else
	 return str;
   }

   MString dir = get_env_default("MLCPATH",".");
   if (dir.contains("~"))
      err << "mlcIO::file_exists: ~ expansion not supported in MLCPATH"
	 << fatal_error;
   if (dir.contains("::"))
      err << "mlcIO::file_exists: MLCPATH is '" << dir
	  << "' and contains ::" << fatal_error;
   if (dir[dir.length()-1] != ':') // Make sure mlcpath is colon terminated
      dir += ":";

   MString fileName;
   // Loop until last colon
   int currentPos = -1;
   do {
      int startPos = currentPos + 1;
      currentPos = dir.index(":",startPos);
      MString pathName = dir(startPos,currentPos-1);
      if (pathName[pathName.length()-1] != '/')
	 pathName += "/";
      fileName = pathName + str ;
      stream.open(fileName, ios::in);
   } while (!stream.good() && currentPos < dir.length()-1);
   
   if (!stream.good()) {
      if (fatalOnFalse)
	 err << "mlcIO::file_exists: File '" << str << "' does not exist "
	     << "in colon separated paths '" << dir << "'" << fatal_error;
      return "";
   }

   return fileName;
} 


/***************************************************************************
  Description : Truncate a file to the current position.
                  The MLCOStream must be a file stream.
  Comments    : This must be a very efficient operation, i.e., it should not
                  be implemented via open/close.  See C45Interface.c
***************************************************************************/

void truncate_file(MLCOStream& stream)
{
#if defined(GNU)
   err << "truncate_file not implemented in GNU" << fatal_error;
#else
   ftruncate(stream.get_fstream().rdbuf()->fd(), stream.get_stream().tellp());
#endif
}
