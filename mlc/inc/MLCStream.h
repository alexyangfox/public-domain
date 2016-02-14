// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _MLCStream_h
#define _MLCStream_h 1

#include <MString.h>
#include <error.h>
#include <fstream.h>
#include <strstream.h>
#include <checkstream.h>

// For saving, restoring options
struct MStreamOptions{
   int width;
   MString wrapPrefix;
   MString newLinePrefix;
};

enum OutputType {FileStream, MemStream, XStream, PrinterStream};

class MLCIStream {
protected:
   enum {INPUT_BUFFER_SIZE = 50000}; // this is like a #define for the class
   char buf[INPUT_BUFFER_SIZE];
   const MString descr;
   // Note order. strStream & fileStream need to be
   //   declared before stream in order for c-tor's to work.
   istrstream* strStream;
   ifstream *fileStream;
   istream& stream;         // Refers to either an strStream or an ifstream
   void open_read();
   void update_line_count();
   Bool closed;             // Flag for safety
   long tellpLast; 
   int lineCount;
   int posInLine;
   void init();
public:
   void attach(int fd);
   inline istream& get_stream();
   void OK(int level = 0) const;
   MLCIStream(const MString& fileName);
   MLCIStream(const MString& dscr, ifstream& strm);
   MLCIStream(istream& strm);
   MLCIStream(int fd, const MString& dscr);
   ~MLCIStream();
   void close();
   void check_stream_is_valid() const;
   int line_count();
   int pos_in_line();
   ifstream& get_fstream();
   // The function below is *ONLY* used in OK() because of the oc ref. bug.
   istrstream& get_strstream() { return *strStream;}  
   void setbuf(char *p, int l);
   Bool eof(){ return get_stream().eof(); }
   inline char get();
   MLCIStream& get(char&);
   void ignore(int len = 1, int delim = EOF);
   inline int peek();
   const MString& description() const;
   void skip_white();
   void read_bin(int&);
   void read_bin(unsigned char&);
   void read_bin(char&);
   void read_bin(long&);
   void read_bin(float&);
   void read_bin(short&);
   void read_bin(double&);
   MLCIStream& operator>>(int&);
   MLCIStream& operator>>(char*);
   MLCIStream& operator>>(char&);
   MLCIStream& operator>>(long&);
   MLCIStream& operator>>(Real&);
   MLCIStream& operator>>(short&);
   MLCIStream& operator>>(float&); 
   MLCIStream& operator>>(unsigned int&);
   MLCIStream& operator>>(unsigned char&);
   MLCIStream& operator>>(unsigned long&);
   MLCIStream& operator>>(unsigned short&);
   MLCIStream& operator>>(istream& (*)(istream&));
#if defined(CFRONT) || defined(IRIX)
   friend MLCIStream& operator>>(MLCIStream& s, const SMANIP(int)& m);
#elif defined(GNU)
   friend MLCIStream& operator>>(MLCIStream& s, const smanip<int>& m);
#else
#error "compiler not supported"
#endif

};

class MLCOStream {
protected:
   enum {MAX_BUFFER_SIZE = 500}; // this is like a #define for the class
   const OutputType outputType;
   MString descr;
   // Note order. strStream & fileStream need to be
   //   declared before stream in order for c-tor's to work.
   ostrstream* strStream;
   ofstream* fileStream;
   ostream& stream;         // Can refer to a string stream or an ofstream.
   ostrstream formatStream; // strstream used for incore formatting.
   char formatStreamBuf[MAX_BUFFER_SIZE];
   long lineCount, linePos; 
   Bool atLeftmostPosition; // For checking to see if we are at the
			    // left most position on output.
   char *memBuf;            // Returned from an strsrtream-based MLCOStream.
   int width;               // Line width.
   MString wrapPrefix;      // The string that prints when the output wraps.
   MString newLinePrefix;   // The string that prints when a newline is printed.
   Bool closed;
   void init();
   void print_trailing_space();
   void output_format_stream(Bool flushOutput);
   Bool output_format_stream_no_newline(Bool outputFlush, long lenny);
   void gen_newline(); 
   
public:
   void attach(int fd);
   inline ostream& get_stream();
   // Wrapping methods.
   void set_width(int newWidth);
   void set_wrap_prefix(const MString& str);
   void set_newline_prefix(const MString& str);
   const MString& get_wrap_prefix() const { return wrapPrefix;}
   const MString& get_newline_prefix() const { return newLinePrefix;}
   int wrap_width() const { return width;}
   int wrap_prefix_length() const { return wrapPrefix.length();}
   int newline_prefix_length() const{ return newLinePrefix.length();}
   void reset_pos_in_line() { output_format_stream(TRUE); linePos = 1;}
   void set_options(const MStreamOptions& options);
   MStreamOptions get_options() const;
   
   // Everything else.
   void OK(int level = 0) const;
   MLCOStream(const OutputType otype);
   // For memory streams (strstream based MLCOStream).
   MLCOStream(const MString& description, char *buf, int nBytes); 
   MLCOStream(const MString& fileName, const OutputType otype = FileStream,
	      Bool append = FALSE, int prot = defaultOpenProt);
   MLCOStream(const MString& dscr, ofstream& strm,
	      const OutputType otype = FileStream);
   MLCOStream(ostream& strm, const OutputType otype);
   MLCOStream(int fd, const MString& dscr,
	      const OutputType otype = FileStream);
   ~MLCOStream();
   void close();
   void check_stream_is_valid() const;
   ofstream& get_fstream();
   ostrstream& get_strstream();
   int line_count() const;
   int pos_in_line();
   void setbuf(char *p, int l);
   const MString& description() const;
   MString mem_buf();
   OutputType output_type() const;
   void include_file(const MString& filename);
   void write_bin(int&);
   void write_bin(char&);
   void write_bin(unsigned char&);
   void write_bin(long&);
   void write_bin(float&);
   void write_bin(short&);
   void write_bin(double&);
   MLCOStream& operator<<(int);
   MLCOStream& operator<<(char);
   MLCOStream& operator<<(long);
   MLCOStream& operator<<(short);
   MLCOStream& operator<<(float);
   MLCOStream& operator<<(double);
   MLCOStream& operator<<(const char*);
   MLCOStream& operator<<(unsigned int);
   MLCOStream& operator<<(unsigned char);
   MLCOStream& operator<<(unsigned long);
   MLCOStream& operator<<(unsigned short);
   MLCOStream& operator<<(void*);
   MLCOStream& operator<<(ostream& (*)(ostream&));
   MLCOStream& operator<<(MLCOStream& (*)(MLCOStream&));
#if defined(CFRONT) || defined(IRIX)
   friend MLCOStream& operator<<(MLCOStream& s, const SMANIP(int)& m);
#elif defined(GNU)
   friend MLCOStream& operator<<(MLCOStream& s, const smanip<int>& m);
#else
#error "compiler not supported"
#endif
};

MLCOStream& reset_pos_in_line(MLCOStream&);


/***************************************************************************
  Description : Returns the istream associated with the stream.
  Comments    : Currently all constructors set it so it would never be the
                  case that this returns an invalid stream.
***************************************************************************/
istream& MLCIStream::get_stream()
{
   return stream;
} 



/***************************************************************************
  Description : Extracts a character from the stream and returns it.
  Comments    : Aborts if extraction encounters end of file.
***************************************************************************/
char MLCIStream::get()
{
   int ch = stream.get();

   if (ch == EOF) 
      err << "MLCIStream::get: Tried to get EOF in stream " << description()
	  << fatal_error;
   OK();
   return ch;
}


/***************************************************************************
  Description : If stream is at end of file, returns EOF.  Otherwise,
                  returns next character without extracting it.
  Comments    :
***************************************************************************/
int MLCIStream::peek()
{
   // Notice that there's no OK() check here because we want the
   //   caller to check EOF.
   // OK() here is also a time killer (10% of time in mushroom/ID3).
   return get_stream().peek();
}   


/***************************************************************************
  Description : Checks that the stream is valid, returns the ostream associated
                  with the stream.
  Comments    : 
***************************************************************************/
ostream& MLCOStream::get_stream()
{
  if (outputType == XStream)
      err << "MLCOStream::get_stream(): operation undefined for XStream "
	  << description() << fatal_error;
  if (wrap_width())
     return ((MLCOStream*)this)->formatStream; // cast away constness
   else
     return stream;
}



#endif









