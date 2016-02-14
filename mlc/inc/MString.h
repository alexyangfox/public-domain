// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "MString.c".

#ifndef _MString_h
#define _MString_h 1

class MLCOStream;
class MLCIStream;

class MString {

   friend class MStringRC;
   char *str;
   int len;
   
   void construct_long(long source, int width);
   void copy(const MString&);
   void free();
   
   int refCount;
   
public:
   void OK(int level = 0) const; // has only one level of checking. 
   MString();
   MString(const MString&);
   MString(const char*);
   MString(const char*, int l); // copy only first l chars
   MString(char source);
   MString(Real, int precision, int width = 0);
   MString(long, int width);
   MString(int, int width);
   MString(short, int width);
   ~MString();                  // destructor is not virtual because
                                // there are no virtual members.
   
   int length() const { return len; }
   
   MString& operator=(const MString&);
   MString& operator+=(const MString&);
   
   //This is different from operator== in that it ignores case.
   Bool equal_ignore_case(const MString&) const;
   
   // Note that functions need be overriden three times since
   // the automatic conversions to the first one from the second
   // two are amiguous.
   // These are friend functions. Because there won't be any inheritance from
   //   MString, they are not virtual.
   friend MString operator+(const MString&, const MString&);
   friend Bool operator==(const MString&, const MString&);
   friend Bool operator==(const MString&, const char *);
   friend Bool operator==(const char *,   const MString&);
   friend Bool operator!=(const MString&, const MString&);
   friend Bool operator!=(const MString&, const char *);
   friend Bool operator!=(const char *,   const MString&);
   friend Bool operator< (const MString&, const MString&);
   friend Bool operator< (const MString&, const char *);
   friend Bool operator< (const char *,   const MString&);
   friend Bool operator<=(const MString&, const MString&);
   friend Bool operator<=(const MString&, const char *);
   friend Bool operator<=(const char *,   const MString&);
   friend Bool operator> (const MString&, const MString&);
   friend Bool operator> (const MString&, const char *);
   friend Bool operator> (const char *,   const MString&);
   friend Bool operator>=(const MString&, const MString&);
   friend Bool operator>=(const MString&, const char *);
   friend Bool operator>=(const char *,   const MString&);
   
   // Common conversions to other types
   Real real_value() const;
   long long_value() const;
   short short_value() const;
   char char_value() const;
   
   // This is the only conversion which is "automatic."  The above
   // conversions are explicit methods.
   operator const char*() const { return str; }
   
   MString& prepend(const MString&);
   // Fast z = x + y. Works for z = x or y too.
   friend MString& cat(const MString& x, const MString& y, MString& z);
   // Fast w = x + y + z. Works for w = x, y, or z too.
   friend MString& cat(const MString& x, const MString& y,
		       const MString& z, MString& w);
   // Returns the index of the leftmost or rightmost, occurrence of l
   //   in the string. 
   int index(const MString& l, int s = 0) const;
   // Returns TRUE if the string contains l at position s.
   Bool contains(const MString& l, int s) const;
   // Returns TRUE if the string contains l.
   Bool contains(const MString& l) const;
   // Returns TRUE if the string matches l starting at
   //   position s with no trailing characters, FALSE otherwise.
   Bool matches(const MString& l, int s) const;
   
   // Returns the substring starting at pos of lengh len.
   MString substring(int pos, int subLen) const;
   // Returns the substring starting at startPos until(including) endPos
   MString operator()(int startPos, int endPos) const;
   // Returns the pos'th char
   char operator[](int pos) const ; // see set_char for setting.

   // reads a line from a stream and sticks it in this MString
   void get_line(MLCIStream& stream, char delim = '\n');

   // Conversion functions which do not abort.  These are used by UI
   // routines which need to inform the user if input was invalid.
   // Both return TRUE if successful, FALSE if not.
   Bool convert_to_real(Real& r) const;
   Bool convert_to_long(long& l) const;

   // Get an all-caps copy of this string
   MString to_upper() const;
   void set_char(int pos, char c); // set character at position pos to be
                              // c.  c cannot be \0.
};

// Safe input. Makes sure the input does not overflow. The max num of
//   characters that could be read from a stream is MAX_INPUT_STRING
//   defined in "basics.h".
MLCIStream& operator>>(MLCIStream&,MString&); 
MLCOStream& operator<<(MLCOStream& out, const MString& dest);

// like sprintf with one argument
MString insert_arg(const MString& templateStr, const MString& argStr);

#endif









