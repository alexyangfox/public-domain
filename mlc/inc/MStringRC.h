// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "MStringRC.c".

#ifndef _MStringRC_h
#define _MStringRC_h 1

#include <MString.h>

class MStringRC {

#define HANDLE_CLASS MStringRC
#define BODY_CLASS MString
#include <RefCount.h>

public:
   void OK(int level = 0) const {
      read_rep()->OK(level);
   }
   MStringRC() { set_rep(new MString); }
   MStringRC(const MString str) { set_rep(new MString((const char *)str)); }
   MStringRC(const char* str) { set_rep(new MString(str)); }
   MStringRC(const char* str, int l) { set_rep(new MString(str, l)); }
   MStringRC(char source) { set_rep(new MString(source)); }

   MStringRC(Real realNum, int precision, int width = 0) {
      set_rep(new MString(realNum, precision, width));
   }
   MStringRC(long longNum, int width) {
      set_rep(new MString(longNum, width));
   }
   MStringRC(int intNum, int width) {
      set_rep(new MString(intNum, width)); }
   MStringRC(short shortNum, int width) {
      set_rep(new MString(shortNum, width));
   }

   int length() const { return read_rep()->length(); }
   
   MStringRC& operator+=(const MStringRC& src) {
      write_rep()->operator+=(*src.read_rep());
      return *this;
   }

   //This is different from operator== in that it ignores case.
   Bool equal_ignore_case(const MStringRC& src) const {
      return read_rep()->equal_ignore_case(*src.read_rep());
   }

   // Note that functions need be overriden three times since
   // the automatic conversions to the first one from the second
   // two are amiguous.
   // These are friend functions. Because there won't be any inheritance from
   //   MStringRC, they are not virtual.
   friend MStringRC operator+(const MStringRC& s1, const MStringRC& s2) {
      return MStringRC(*s1.read_rep() + *s2.read_rep());
   }
   friend Bool operator==(const MStringRC& s1, const MStringRC& s2) {
      return (*s1.read_rep() == *s2.read_rep());
   }
   friend Bool operator==(const MStringRC& s1, const MString& s2) {
      return (*s1.read_rep() == s2);
   }
   friend Bool operator==(const MString& s1, const MStringRC& s2) {
      return (s1 == *s2.read_rep());
   }
   friend Bool operator==(const MStringRC& s1, const char * s2) {
      return (*s1.read_rep() == s2);
   }
   friend Bool operator==(const char * s1,   const MStringRC& s2) {
      return (s1 == *s2.read_rep());
   }
   friend Bool operator!=(const MStringRC& s1, const MStringRC& s2) {
      return (*s1.read_rep() != *s2.read_rep());
   }
   friend Bool operator!=(const MStringRC& s1, const MString& s2) {
      return (*s1.read_rep() != s2);
   }
   friend Bool operator!=(const MString& s1, const MStringRC& s2) {
      return (s1 != *s2.read_rep());
   }
   friend Bool operator!=(const MStringRC& s1, const char * s2) {
      return (*s1.read_rep() != s2);
   }
   friend Bool operator!=(const char * s1,   const MStringRC& s2) {
      return (s1 != *s2.read_rep());
   }
   friend Bool operator< (const MStringRC& s1, const MStringRC& s2) {
      return (*s1.read_rep() < *s2.read_rep());
   }
   friend Bool operator< (const MStringRC& s1, const char * s2) {
      return (*s1.read_rep() < s2);
   }
   friend Bool operator< (const char * s1,   const MStringRC& s2) {
      return (s1 < *s2.read_rep());
   }
   friend Bool operator<=(const MStringRC& s1, const MStringRC& s2) {
      return (*s1.read_rep() <= *s2.read_rep());
   }
   friend Bool operator<=(const MStringRC& s1, const char * s2) {
      return (*s1.read_rep() <= s2);
   }
   friend Bool operator<=(const char * s1,   const MStringRC& s2) {
      return (s1 <= *s2.read_rep());
   }
   friend Bool operator> (const MStringRC& s1, const MStringRC& s2) {
      return (*s1.read_rep() > *s2.read_rep());
   }
   friend Bool operator> (const MStringRC& s1, const char * s2) {
      return (*s1.read_rep() > s2);
   }
   friend Bool operator> (const char * s1,   const MStringRC& s2) {
      return (s1 > *s2.read_rep());
   }
   friend Bool operator>=(const MStringRC& s1, const MStringRC& s2) {
      return (*s1.read_rep() >= *s2.read_rep());
   }
   friend Bool operator>=(const MStringRC& s1, const char * s2) {
      return (*s1.read_rep() >= s2);
   }
   friend Bool operator>=(const char * s1,   const MStringRC& s2) {
      return (s1 >= *s2.read_rep());
   }
   
   // Common conversions to other types
   Real real_value() const { return read_rep()->real_value(); }
   long long_value() const { return read_rep()->long_value(); }
   short short_value() const { return read_rep()->short_value(); }
   char char_value() const { return read_rep()->char_value(); }
   
   // This is the only conversion which is "automatic."  The above
   // conversions are explicit methods.
   operator const char*() const { return (const char*)*read_rep(); }
   
   MStringRC& prepend(const MStringRC& src) {
      write_rep()->prepend(*src.read_rep());
      return *this;
   }
   // Fast z = x + y. Works for z = x or y too.
   friend MStringRC& cat(const MStringRC& x, const MStringRC& y,
			 MStringRC& z) {
      cat(*x.read_rep(), *y.read_rep(), *z.write_rep());
      return z;
   }
   // Fast w = x + y + z. Works for w = x, y, or z too.
   friend MStringRC& cat(const MStringRC& x, const MStringRC& y,
			 const MStringRC& z, MStringRC& w) {
      cat(*x.read_rep(), *y.read_rep(), *z.read_rep(), *w.write_rep());
      return w;
   }
   // Returns the index of the leftmost or rightmost, occurrence of l
   //   in the string. 
   int index(const MStringRC& l, int s = 0) const {
      return read_rep()->index(*l.read_rep(),s);
   }
   // Returns TRUE if the string contains l at position s.
   Bool contains(const MStringRC& l, int s) const {
      return read_rep()->contains(*l.read_rep(),s);
   }
   // Returns TRUE if the string contains l.
   Bool contains(const MStringRC& l) const {
      return read_rep()->contains(*l.read_rep());
   }
   // Returns TRUE if the string matches l starting at
   //   position s with no trailing characters, FALSE otherwise.
   Bool matches(const MStringRC& l, int s) const {
      return read_rep()->matches(*l.read_rep(),s);
   }

   // Returns the substring starting at pos of lengh len.
   MStringRC substring(int pos, int subLen) const {
      return MStringRC(read_rep()->substring(pos, subLen));
   }
   // Returns the substring starting at startPos until(including) endPos
   MStringRC operator()(int startPos, int endPos) const {
      return MStringRC(read_rep()->operator()(startPos, endPos));
   }
   // Returns the pos'th char
   char operator[](int pos) const {
      return read_rep()->operator[](pos);
   }

   // Safe input. Makes sure the input does not overflow. The max num of
   //   characters that could be read from a stream is MAX_INPUT_STRING
   //   defined in "basics.h".
   friend MLCIStream& operator>>(MLCIStream& stream, MStringRC& src) {
      stream >> *src.write_rep();
      return stream;
   }

   friend MLCOStream& operator<<(MLCOStream& stream, const MStringRC& src) {
      stream << *src.read_rep();
      return stream;
   }

   // like sprintf with one argument
   friend MStringRC insert_arg(const MStringRC& templateStr,
			const MStringRC& argStr) {
      return MStringRC(insert_arg(*templateStr.read_rep(),
				  *argStr.read_rep()));
   }   
};
#endif
