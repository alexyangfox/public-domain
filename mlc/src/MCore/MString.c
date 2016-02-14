// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : String class with some nice features. MString can
                   be initialized with a string (char*), the first l
 		   characters of a string, another MString or just a
		   single character.
		 The usual lexicographic relational operators are defined for
		   MStrings as well as the operators for string
		   concatenation, including operators + and +=.
		 There are also functions that search in different ways for
		   a substring of MString that matches a given string.
		 A conversion operator from MString to const char* is provided.
  Assumptions  : When MString is read from a stream its length should
                   be no bigger than MAX_INPUT_STRING.
  Comments     : The class is meant to mimic GNU's strings through a
                   nicer implementation. The implementation of the
		   searching functions differs from the GNU implementation
		   because substrings are not supported. Consequently, the
		   two private functions match and search that do all the
		   work in the GNU class are not used. Instead, the public
		   functions index and contains play the role of
		   search and match.
		 When empty strings are created through the
		   constructor with no arguments they are initialized
		   to point to the static nullChar. Further
		   manipulations that would still result in an empty string
		   are not guaranteed to leave the string pointing to
		   nullChar. For example:
		   MString e1, e2; // e1.str == e2.str == &nullChar
		   e1 += e2;       // e1.str may not be
				   // &nullChar.
		 When NULL is passed as char* to the constructor it
		   creates an empty string.
	 	 All searching functions will match the empty string
		   from any position in the object.		   
  Complexity   : All functions take O(total num-characters of their arguments)
                   except the following:
		   1) The lexicographic relational operators take O(biggest
		   matching num-characters of the two arguments) time.
		   2)The index() and contains(const MString&) take
		   O(sum of lengths of all matching substrings) time.
		 The destructor takes constant time.
  Enhancements : Implement operator>> to read a string of unlimited length.
                 Implement index(), and therefore contains(const MString&),
		   to work in O(num-char of source + num-char of object),
		   using a linear time algorithm. Reference:
		   'Introduction to Algorithms' by Cormen, Leiserson
		   and Rivest, pp.857-885. 
  History      : Svetlozar Nestorov                           11/26/93
                   Initial revision (.h,.c)
***************************************************************************/

static char nullChar = 0;

#include <basics.h>
#include <string.h>
#include <MString.h>
#include <MLCStream.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <std.h>    // gnu needs it for strcasecmp

RCSID("MLC++, $RCSfile: MString.c,v $ $Revision: 1.38 $")

/***************************************************************************
  Description : Allocates space for the new value of the string and
                  copies this new value there. 
  Comments    : Takes O(number of characters in the new value) time.
                Assumes that this != &source.
		Private function.
***************************************************************************/
void MString::copy(const MString& source)
{
   ASSERT(this != &source);
   str = new char[(len = source.length()) + 1];
   strcpy(str, source.str);
   refCount = 1;   
}


/***************************************************************************
  Description : Deallocates the space previously allocated for the value of
                  the string. 
  Comments    : Assumes that str != NULL.
                Private function.
***************************************************************************/
void MString::free()
{
   DBG(OK());
   ASSERT(str != NULL);
   if (str != &nullChar)
      delete[] str;
}


/***************************************************************************
  Description : This helper function is used by the constructors that
                  create a string from a long, short, or int.  These
		  constructors also takes a width (which is the number
		  of non-NULL characters).  This is also parameter is
		  also passed and used here.If the width is 0, the
		  minimum needed number of characters are used. 
  Comments    : As above, the width should not be given a default value of
                  zero.  To do so would ignore one of the original
		  motivations of having extra arguments--namely that
		  construction from a long cannot occur as an accident.
		The extra space in the string is filled with ' '
		  characters. 
***************************************************************************/
void MString::construct_long(long source, int width)
{
   // We need to create a temporary string so that we know how much
   // space needs to be allocated.
   static char tempString[MAX_LONG_LEN+1];
   ostrstream tempNumStream(tempString, MAX_LONG_LEN + 1);
   tempNumStream.seekp(0);
   tempNumStream << source << '\0';

   int numLength = strlen(tempString);
   if (width && numLength > width)
      err << "MString::MString(long,width): long (" << source <<
	 ") does not fit in the given width of (" << width << ")" <<
	 fatal_error;

   // if the user passes in (width == 0), the minumum space is used to
   // store the string.
   if (!width) 
      width = numLength;
   
   str = new char[(len = width) + 1];
   ostrstream numStream(str, width + 1);
   numStream << setfill(' ');
   numStream << setw(width);
   
   // note that we don't need to deal with the adjustment since the
   // default is right adjustment.
   numStream.seekp(0);
   numStream << source << '\0';
   ASSERT(!numStream.fail());
   refCount = 1;   
}


/***************************************************************************
  Description : Checks that str != NULL because empty strings should
                  either point to the static nullChar or have space
		  for one char allocated for them.
		Checks that the actual length of the string is the
		  same as the one stored in len.  
  Comments    :
***************************************************************************/
void MString::OK(int /*level*/) const
{
   if (str == NULL)
      err << "MString::OK: String is NULL" << fatal_error;
   for (int i=0; i<len; i++) 
      if (!str[i]) 
	 err << "MString::OK: NULL at pos " << i << ", while the recorded"
	 " length of the string  is " << len << fatal_error;
   if (str[len])
      err << "MString::OK: Non NULL character '" << str[len] << "' at pos "
	  << len << " while the recorded length of the string is " << len
	  << fatal_error;
}


/***************************************************************************
  Description : The constructor that takes no arguments
                  initializes the string to the empty string. In this
		  case str points to a static char nullChar.  
  Comments    : It is not guaranteed for an empty string that is not created
		  by the constructor that str will point to nullChar. 
***************************************************************************/
MString::MString()
{
   len = 0;
   str = &nullChar;
   refCount = 1;
}


/***************************************************************************
  Description : Copy Constructor.
  Comments    : Copy construction is allowed since Strings are fairly
                  small, and one often needs to copy strings for
		  simple manipulation. 
***************************************************************************/
MString::MString(const MString& source)
{	
   copy(source);
}


/***************************************************************************
  Description : Constructor from a c style string.  Also a constructor
                  that just uses the specified number of characters.	
  Comments    : copy cannot be used here because it takes MString and
                  this is the constructor that makes MString from char*.
***************************************************************************/
MString::MString(const char* source)
{
   if (source && strlen(source) > 0) {            // check if we are passed in
      str = new char[(len = strlen(source)) + 1]; // an empty string.
      strcpy(str, source);
   }
   else {
      len = 0;
      str = &nullChar; 
   }
   refCount = 1;   
}

MString::MString(const char* source, int first)
{
   if (first < 0)
      err << "MString::MString: Negative first (" << first
      << ") for first letters of a string" << fatal_error;
   if (first > strlen(source))
      err << "MString::MString: First (" << first
      << ") too big; string length is " << strlen(source) << fatal_error; 
   str = new char[(len = first)+1];
   strncpy(str, source, first);
   str[len] = 0;
   refCount = 1;      
}


/***************************************************************************
  Description : Construct an MString of a single character.
  Comments    : There was some debate whether this needed a dummy
                  argument or should even be allowed at all.  The
		  deciding factor was to allow it since there the user
		  is unlikely to be harmed by accidental construction
		  of a string from a character.
		Note that if (source == 0) we create a NULL string, because
		  the user's call is MString(char(0)). 
***************************************************************************/
MString::MString(char source)
{
   if (source) {
      len = 1;
      str =  new char[2];
      str[0] = source;
      str[1] = 0;
   }
   else {
      len = 0;
      str = &nullChar;
   }
   refCount = 1;      
}


/***************************************************************************
  Description : The MString constructor that takes a real also takes a
                  width (which is the number of non-NULL characters)
		  and a precision).  Note that if the width and/or
		  precisions are 0, the maximum values are used.
  Comments    : Note that width should not be given a default value of
                  zero.  To do so would ignore one of the original
		  motivations of having extra arguments--namely that
		  construction from a real cannot occur as an accident.
		The extra space in the string is filled with the ' '
		  character. 
***************************************************************************/
MString::MString(Real source, int precision, int width)
{
   // We need to create a temporary string so that we know how much
   // space needs to be allocated.
   static char tempString[MAX_REAL_LEN+1];
   ostrstream tempNumStream(tempString, MAX_REAL_LEN + 1);
   tempNumStream << setprecision(precision);
    
   if(precision)
      tempNumStream.setf(ios::floatfield, ios::fixed);
   else
      tempNumStream << setprecision(DEFAULT_PRECISION);
   tempNumStream.seekp(0);
   tempNumStream << source << '\0';
      
   int numLength = strlen(tempString);
   if (width && numLength > width)
      err << "MString::MString(real,precision, width): real (" << source <<
	 ") does not fit in the given width of (" << width << ")" <<
	 fatal_error;

   // if the user passes in (width == 0), the minumum space is used to
   // store the string.
   if (!width) 
      width = numLength;
   
   str = new char[(len = width) + 1];
   ostrstream numStream(str, width + 1);
   numStream << setprecision(precision);
   numStream << setfill(' ');
   numStream << setw(width);
    
   // force number of decimal places as specified by precision
   if(precision)
      numStream.setf(ios::floatfield, ios::fixed);
   else
      numStream << setprecision(DEFAULT_PRECISION);
   
   // note that we don't need to deal with the adjustment since the
   // default is right adjustment.
   numStream.seekp(0);
   numStream << source << '\0';

   ASSERT(!numStream.fail());
   refCount = 1;      
}


/***************************************************************************
  Description : The MString constructors which take a long, short, and
                  int also take a width (which is the number of non-NULL
		  characters). If the width is 0, the minimum needed value
		  is used. 
  Comments    : Since all of these constructors do the same thing, the
                  common code is factored out into a helper function.
		  While it would make sense for the short and int
		  constructors call the long constructor, this is not
		  possible in C++. 
***************************************************************************/
MString::MString(long source, int width)
{
   construct_long(source, width);
}

MString::MString(short source, int width)
{
   construct_long(source, width);
}


MString::MString(int source, int width)
{
   construct_long(source, width);
}


/***************************************************************************
  Description : Destructor. 
  Comments    :
***************************************************************************/
MString::~MString()
{
   DBG(OK());
   free();
}


/***************************************************************************
  Description : Operator=. Deallocates the space for the
                  previous value of the string and allocates space and
		  copies the new value there.   
  Comments    : Works for x = x.
***************************************************************************/
MString& MString::operator=(const MString& source)
{
   if (this != &source) {
      free();
      copy(source);
   }
   return (*this);
}


/***************************************************************************
  Description : Operator +=. Concatenates the source after the object
                  and stores it into the object.
  Comments    : Works for x += x.
***************************************************************************/
MString& MString::operator+=(const MString& source)
{
   return cat(*this, source, *this);
}


/***************************************************************************
  Description : Similar to operator== in that it ignores case.
  Comments    : This was made a method instead of a friend function as
                  operator== is.  The reasoning is that operator== is
		  useful because you write str1==str2.  On the other hand,
		  equal_ignore_case(str1, str2) is as better than
		  str1.equal_ignore_case(str2).  
***************************************************************************/
Bool MString::equal_ignore_case(const MString& str2) const
{
   return (strcasecmp(str, str2.str) == 0);
}


/***************************************************************************
  Description : Operator +. Constructs and returns a string which is the sum
                  of the two arguments.  Either can be a character.      
  Comments    : For w = x + y it will be faster to use
		  cat(x, y, w).
***************************************************************************/
MString operator+(const MString& str1, const MString& str2) 
{
   MString sum;
   return cat(str1, str2, sum);
}


/***************************************************************************
  Description : The usual lexicographic relational operators (==, !=,
                  <, <=, >, >=) are defined below. 
  Comments    : All operators run in O(biggest number of matching
                  chararcters).
		We implement many versions because otherwise conversions
		  are not uniquely determined.
***************************************************************************/
Bool operator==(const MString& str1, const MString& str2)
{
   return (strcmp(str1.str, str2.str) == 0);
}

Bool operator==(const MString& str1, const char* str2)
{
   return (strcmp(str1.str, str2) == 0);
}

Bool operator==(const char* str1, const MString& str2)
{
   return (strcmp(str1, str2.str) == 0);
}

Bool operator!=(const MString& str1, const MString& str2)
{
   return (strcmp(str1.str, str2.str) != 0);
}

Bool operator!=(const MString& str1, const char* str2)
{
   return (strcmp(str1.str, str2) != 0);
}

Bool operator!=(const char* str1, const MString& str2)
{
   return (strcmp(str1, str2.str) != 0);
}

Bool operator<(const MString& str1, const MString& str2)
{
   return (strcmp(str1.str, str2.str) < 0);
}

Bool operator<(const MString& str1, const char* str2)
{
   return (strcmp(str1.str, str2) < 0);
}

Bool operator<(const char* str1, const MString& str2)
{
   return (strcmp(str1, str2.str) < 0);
}

Bool operator<=(const MString& str1, const MString& str2)
{
   return (strcmp(str1.str, str2.str) <= 0);
} 

Bool operator<=(const MString& str1, const char* str2)
{
   return (strcmp(str1.str, str2) <= 0);
}

Bool operator<=(const char* str1, const MString& str2)
{
   return (strcmp(str1, str2.str) <= 0);
} 

Bool operator>(const MString& str1, const MString& str2)
{
   return (strcmp(str1.str, str2.str) > 0);
}

Bool operator>(const MString& str1, const char* str2)
{
   return (strcmp(str1.str, str2) > 0);
}

Bool operator>(const char*str1, const MString& str2)
{
   return (strcmp(str1, str2.str) > 0);
}

Bool operator>=(const MString& str1, const MString& str2)
{
   return (strcmp(str1.str, str2.str) >= 0);
} 

Bool operator>=(const MString& str1, const char* str2)
{
   return (strcmp(str1.str, str2) >= 0);
}

Bool operator>=(const char* str1, const MString& str2)
{
   return (strcmp(str1, str2.str) >= 0);
}


/***************************************************************************
  Description : Common conversions to other types.
  Comments    : These are not made conversion operators to prevent
                  accidental conversions.  Note, though, since a
		  asking for pointer to the characters is so common,
		  this is the only exception.
		Overflow error checking is performed.
***************************************************************************/
Real MString::real_value() const
{
   char *endptr;
   double realValue = strtod(str, &endptr);

   if ((endptr - str) != len)
      err << "MString::real_value: Not all " << len << " characters were "
	 " used in the conversion of " << str << " to a real value" <<
	 fatal_error;
      
   if (fabs(realValue) > FLT_MAX) 
      err << "MString::real_value: Overflow occured in "
	 "conversion of MString to a real value" <<
	 fatal_error;

   if ((fabs(realValue) < FLT_MIN) && (realValue != 0)) 
      err << "MString::real_value: Underflow occured in "
	 "conversion of MString to a real value" <<
	 fatal_error;

   return (realValue);
}


long MString::long_value() const
{
   if (*this == "")
      err << "MString::long_value(): empty string" << fatal_error;
   
   char *endptr;

   // the final argument to strtol (0) indicates that we should use the
   // default base unless the user specifies otherwise.
   long longValue = strtol(str, &endptr, 0);

   if ((endptr - str) != len)
      err << "MString::long_value: Not all " << len << " characters were "
	 " used in the conversion of " << str << fatal_error;

   // Note that the AT&T Sun compiler does not implement errno for
   // overflows on the strtol.  To quote from the man page on strtol():
   // "BUGS
   //    Overflow conditions are ignored."
   // As a result, there is a compensating code path that is taken
   // until this bug is fixed:  the number is simply converted back to
   // a string and checked for equality.  If the strings are equal,
   // then we know that there has been no overflow.

   #ifdef STR_TO_L_BUG_FIXED
   // errno is set on an overflow or an underflow
   if (errno == ERANGE) {
      // errno should be set to 0 in case we catch this error and
      // wish to continue.
      errno = 0;
      err << "MString::long_value: string value does not fit in a long"
	  << fatal_error;
   }

   #else
   // set errno to 0 just in case if the bug is fixed w/o the user
   // knowing. 
   errno = 0;

   // reconvert to a string.
   if (MString(longValue,0) != *this)
      err << "MString::long_value: string value does not fit in a long"
	  << fatal_error;
   
   return (longValue);
   #endif
}


short MString::short_value() const
{
   long longValue = long_value();
   if (longValue > SHORT_MAX || longValue < SHORT_MIN)
      err << "MString::short_value:String value of " << longValue <<
	 " overflows a short" << fatal_error;
   return((short)longValue);
}


char MString::char_value() const
{
   // Conversion to a character only makes sense if the string is of
   // length 1.
   if (len != 1)
      err << "MString::char_value(): attempt to convert a string with "
	 " a length of " << len << " to a character" << fatal_error;
   return str[0];
}



/***************************************************************************
  Description : Prepends the source to the object and stores the
                  result in the object. 
  Comments    : Works for source == object too.
***************************************************************************/
MString& MString::prepend(const MString& source)
{
    return cat(source, *this, *this);
 }


/***************************************************************************
  Description : Concatenates str1 and st2 and stores the result in dest.
  Comments    : Works for both str1 or str2 equal to dest too.
***************************************************************************/
MString& cat(const MString& str1, const MString& str2, MString& dest)
{
   DBG(str1.OK(); str2.OK(); dest.OK());
   char *sum = new char[str1.len + str2.len + 1];
   strcpy(sum, str1.str);
   strcpy(sum + str1.len, str2.str);    // faster than strcat(sum, str2.str)
   dest.free();
   dest.str = sum;
   dest.len = str1.len + str2.len;
   return dest;
}


/***************************************************************************
  Description : Concatenates str1, str2, and str3 and stores the
                  result in dest.
  Comments    : Works for str1, str2 or str3 equal to dest too.
***************************************************************************/
MString& cat(const MString& str1, const MString& str2,
		      const MString& str3, MString& dest)
{
   DBG(str1.OK(); str2.OK(); dest.OK());
   char *sum = new char[str1.len + str2.len + str3.len + 1];
   strcpy(sum, str1.str);
   strcpy(sum + str1.len, str2.str);   // faster than strcat(sum, str2.str)
   strcpy(sum + str1.len + str2.len, str3.str);  
   dest.free();
   dest.str = sum;
   dest.len = str1.len + str2.len + str3.len;
   return dest;
}


/***************************************************************************
  Description : Returns the zero-based index of str1 in string
                  in the following way. If startPos>= 0 returns the
		  index of the leftmost occurrence of str1 in string
		  starting at position startPos. If startPos< 0 returns
		  the index of the rightmost occurence of str1 in
		  string starting at len + startPos and searching
		  string from right to left. If the string is not
		  found returns -1.     
  Comments    : Takes O(sum of lengths of all matching substrings) time.
***************************************************************************/
int MString::index(const MString& str1, int startPos) const
{
   if (startPos < 0) {
      for(startPos += len; startPos >= 0; startPos--) 
	 if (contains(str1, startPos)) 
	    return startPos;
      return -1;
   }
   for (; startPos < len; startPos++) 
      if (contains(str1, startPos))
	 return startPos;
   return -1;
}


/***************************************************************************
  Description : Returns TRUE or FALSE depending on whether the str1
                  is contained in the string or not.
  Comments    : Takes  O(sum of lengths of all matching substrings) time.
***************************************************************************/
Bool MString::contains(const MString& str1) const
{
   return (index(str1, 0) != -1);
}


/***************************************************************************
  Description :  Attempts to match the str1 in the object only at the given
                  startPos. Returns TRUE if successful, FALSE otherwise. 
  Comments    :  Takes O(biggest number of matching characters + 1).
                   startPos should be positive.
***************************************************************************/
Bool MString::contains(const MString& str1, int startPos) const
{
   DBG(str1.OK());
   if (startPos < 0)
      err << "MString::contains: Negative start position" << fatal_error;
   if (len - startPos < str1.length())
      return FALSE;
   for (int i=0; i<str1.length(); i++)
      if (str[startPos+i] != str1.str[i])
       return FALSE;
   return TRUE;
}


/***************************************************************************
  Description : Returns TRUE if the string matches str1 starting at
                  startPos with no trailing characters, FALSE otherwise.
  Comments    : Takes O(biggest number of matching characters + 1).
                  startPos should be positive.
***************************************************************************/
Bool MString::matches(const MString& str1, int startPos) const
{
   if (startPos < 0)
      err << "MString::matches: Negative start position" << fatal_error;
   return (len==startPos+str1.length() && contains(str1, startPos));
}	



/***************************************************************************
  Description : Returns the substring starting at position pos of length len. 
  Comments    : Takes O(length of substring)
***************************************************************************/
MString MString::substring(int pos, int subLen) const
{
   if (subLen<0)
      err << "MString::substring: Negative length of substring (subLength=" <<
	 subLen << ")"	 << fatal_error;

   if ((pos < 0) || (pos >= len))
      err << "MString::substring: Negative or out-of-bound start position = "
	  << pos << fatal_error;

   if ((pos+subLen)>len)
      err << "MString::substring: Access out of bound.(String length = "
	 << len <<"Access to position " << pos+subLen << fatal_error;

   return MString(str + pos, subLen);
}



/***************************************************************************
  Description : Returns the substring starting at position startPos to endPos.
  Comments    : Takes O(length of substring).
***************************************************************************/
MString MString::operator()(int startPos, int endPos) const
{
   if (endPos-startPos < 0)
      err << "MString::substring: Negative length of substring. endPos="
	 << endPos << ", startPos="<< startPos << fatal_error;

   if (startPos < 0 || startPos >= len)
      err << "MString::substring: Negative or out-of-bound start position ("
	  << startPos << ")" << fatal_error;

   if ((endPos<0)||(endPos>=len))
      err << "MString::substring: Negative or out-of-bound end position ("
	 << endPos << ")" << fatal_error;
   
   return MString(str + startPos, endPos-startPos+1);
}

/***************************************************************************
  Description : Returns the pos'th character.
  Comments    : See set_char for changing characters in a string.
***************************************************************************/

char MString::operator[](int pos) const
{
   if (pos < 0 || pos >= len)
      err << "MString::operator[]: Negative or out-of-bound index (" 
	  << pos << ")" << fatal_error;

   return str[pos];
}



/***************************************************************************
  Description : Reads a line from an input stream.  The MString is filled
                  with the contents of this line (not including the
		  newline)
  Comments    :
***************************************************************************/

void MString::get_line(MLCIStream& stream, char delim)
{
   // 2 more than max.  1 is needed for holding the delimiter, and
   // 1 more for trailing newline.
   char buf[MAX_INPUT_STRING+2];

   // read into buffer.  Then force trailing newline
   stream.get_stream().getline(buf, MAX_INPUT_STRING, delim);
   if (stream.get_stream().eof()) {
      free();
      len = 0;
      str = &nullChar;
      ASSERT(refCount == 1);
   } else {
      buf[MAX_INPUT_STRING+1] = 0;

      // search the buffer for the delimiter character or the first
      // newline.  If we find the delimiter, replace it with a newline.
      // If not, abort.
      // Index function is not in libraries we're using, so I defined it
      // myself right here.
      char *pbuf = buf;
      while(*pbuf != '\n' && *pbuf != 0)
	 pbuf++;
      *pbuf = 0;
      
      if(!pbuf)
	 err << "MString.c::get_line: The line" << endl << buf << endl  <<
	    " read in has more chars then the max allowed which is "
	     << MAX_INPUT_STRING << fatal_error;

      // free memory.
      free();
      // copy into MString
      copy(MString(buf));
   }
}


/***************************************************************************
  Description : Common conversions to other types:  UI versions.  These
                  conversion functions return error codes rather than
		  abort if the input is in error.
		In case of overflow, realReturn is set to the max
		  allowed value FLT_MAX, and we return 0.  Underflow is
		  treated similarly.  If the conversion failed, realReturn
		  is set to 0 and we also return 0.
  Comments    : 
***************************************************************************/
Bool MString::convert_to_real(Real& realReturn) const
{
   char *endptr;
   double realValue = strtod(str, &endptr);

   if ((endptr - str) != len) {
      realReturn = 0.0;
      return FALSE;
   }
      
   if (fabs(realValue) > FLT_MAX) {
      realReturn = FLT_MAX;
      return FALSE;
   }

   if (fabs(realValue) < FLT_MIN && (realValue != 0)) {
      realReturn = FLT_MIN;
      return FALSE;
   } 

   realReturn = realValue;
   return TRUE;
}


Bool MString::convert_to_long(long& longReturn) const
{
   char *endptr;

   // the final argument to strtol (0) indicates that we should use the
   // default base unless the user specifies otherwise.
   long longValue = strtol(str, &endptr, 0);

   if ((endptr - str) != len) {
      longReturn = 0;
      return FALSE;
   }


   // Note that the AT&T Sun compiler does not implement errno for
   // overflows on the strtol.  To quote from the man page on strtol():
   // "BUGS
   //    Overflow conditions are ignored."
   // As a result, there is a compensating code path that is taken
   // until this bug is fixed:  the number is simply converted back to
   // a string and checked for equality.  If the strings are equal,
   // then we know that there has been no overflow.

   #ifdef STR_TO_L_BUG_FIXED
   // errno is set on an overflow or an underflow
   if (errno == ERANGE) {
      // errno should be set to 0 in case we catch this error and
      // wish to continue.
      errno = 0;
      longReturn = -1;
      return FALSE;
   }

   #else
   // set errno to 0 just in case if the bug is fixed w/o the user
   // knowing. 
   errno = 0;

   // reconvert to a string.
   if (MString(longValue,0) != *this) {
      longReturn = -1;
      return FALSE;
   }

   #endif
   longReturn = longValue;
   return TRUE;
}

   

/***************************************************************************
  Description : to_upper returns a copy of the string with all of its
                  characters converted to uppercase.
  Comments    : 
***************************************************************************/
MString MString::to_upper() const
{
   MString ret = *this;
   for(int i=0; i<len; i++)
      ret.str[i] = toupper(ret.str[i]);
   return ret;
}

   

/***************************************************************************
  Description : Operator >>. Reads in a char* from a stream and
                  initializes the MString with it. 
  Comments    : The number of characters that could be read in is
                  limited by MAX_INPUT_STRING.
		MStrings should be read from MLCIStreams. Otherwise,
		  due to the way ordinary streams are converted to MLC
		  streams, some information could be lost. Thus cin
		  should not be used to read in MString. Instead use
		  Mcin.
***************************************************************************/
MLCIStream& operator>>(MLCIStream& in, MString& dest)
{
   char source[MAX_INPUT_STRING+2];  // 2 more than max. 1 is needed for
                                     // NULL and 1 more to check for
                                     // overflow. 
   in >> setw(MAX_INPUT_STRING+2) >> source;
   if (strlen(source)> MAX_INPUT_STRING)
      err << "MString.c::operator>>: The string '" << source << "' read in"
	 " has more chars then the max allowed which is "
	    << MAX_INPUT_STRING << fatal_error;
   dest = source;
   return in;
}

MLCOStream& operator<<(MLCOStream& out, const MString& dest)
{
   out << (const char *)dest;
   return out;
}


/***************************************************************************
  Description : Inserts the string arg into the string template.  Uses
		sprintf to replace "%s" in template with arg.
  Comments    :
***************************************************************************/
// like sprintf with one argument
MString insert_arg(const MString& templateStr, const MString& argStr)
{
   char *str = new char[templateStr.length() + argStr.length() + 1];
   // cast template and arg because sprintf takes vargs
   // also, note that sprintf returns different values depending on
   // whether run under system V or BSD.  Since the return value
   // doesn't help us, this isn't a problem.
   (void)sprintf(str, (const char *) templateStr, (const char *) argStr); 
   MString result(str);
   delete[] str;
   return result;
}

/***************************************************************************
  Description : Set a given character of a string to character c.
                The character c must not be NULL (\0)
  Comments    :
***************************************************************************/

void MString::set_char(int pos, char c)
{
   if (pos < 0 || pos >= len)
      err << "MString::set_char: Negative or out-of-bound index (" << pos
	 << ")" << fatal_error;

   if (c == 0)
      err << "MString::set_char: trying to set character to NULL"
	  << fatal_error;

   str[pos] = c;
}



















