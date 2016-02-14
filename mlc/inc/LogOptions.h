// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _LogOptions_h
#define _LogOptions_h 1

class LogOptions {
   int log_level;      // higher is more log info.
   MLCOStream *stream;
   // default copy constructor OK
public:
   // Note that all functions are const because we do not want to disallow
   //   logging from const function members.
   //   However, callers should return the stream state to its original state.
   LogOptions();
   LogOptions(const MString& logOptionName);
   virtual ~LogOptions() {}
   void    set_log_options(const LogOptions& opt) const;
   virtual void set_log_level(int level) const;
   virtual int  get_log_level() const;
   virtual void set_log_stream(MLCOStream& strm) const;
   virtual MLCOStream& get_log_stream() const;
   virtual void set_log_prefixes(char *file, int line) const;
   virtual void set_log_prefixes(char *file, int line, 
				 int stmtLogLevel, int lgLevel) const;
};
   
// The following macro is meant to be put in the private section of a class.
// It provides a standard interface for set/get of log_level and stream
#define LOG_OPTIONS \
public: \
   void set_log_level(int level) const {logOptions.set_log_level(level);} \
   int  get_log_level() const {return logOptions.get_log_level();} \
   void set_log_stream(MLCOStream& strm) const \
      {logOptions.set_log_stream(strm);} \
   MLCOStream& get_log_stream() const {return logOptions.get_log_stream();} \
   const LogOptions& get_log_options() const {return logOptions;} \
   void set_log_options(const LogOptions& opt) const \
      {logOptions.set_log_options(opt);} \
   void set_log_prefixes(char *file, int line,int lvl1, int lvl2) const \
     {logOptions.set_log_prefixes(file, line, lvl1, lvl2);} \
private: \
   LogOptions logOptions

void global_set_log_prefixes(MLCOStream& stream, char *file, int line);

// The following assumes you have the get/set log functions
// as defined by the LOG_OPTIONS macro
#undef IFLOG
#undef LOG
#define IFLOG(level, stmt) if (get_log_level() >= level){\
   MStreamOptions saveOptions = get_log_stream().get_options();\
   set_log_prefixes(__FILE__, __LINE__, level, get_log_level()); stmt;\
   get_log_stream().set_options(saveOptions);} else
#define LOG(level,out) IFLOG(level, get_log_stream() << out)

// The following macros use the global log levels.
#define IFGLOBLOG(level, stmt) if (globalLogLevel >= level){\
   MStreamOptions saveOptions = (*globalLogStream).get_options();\
   global_set_log_prefixes(*globalLogStream, __FILE__, __LINE__); stmt;\
   (*globalLogStream).set_options(saveOptions);}else
#define GLOBLOG(level,out) IFGLOBLOG(level, *globalLogStream << out)

#define IFFLOG(level, stmt) if (logOptions.get_log_level() >= level) {\
   MStreamOptions saveOptions = logOptions.get_log_stream().get_options();\
   logOptions.set_log_prefixes(__FILE__, __LINE__, \
			       level, logOptions.get_log_level()); stmt;\
   logOptions.get_log_stream().set_options(saveOptions);} else
#define FLOG(level, out) IFFLOG(level, logOptions.get_log_stream() << out)   
									     
#endif













