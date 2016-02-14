// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Provide support for MLC++ logging.
  Comments     : Use the macro LOG_OPTIONS to introduce all the appropriate
                   functions for your class.
  Assumptions  : 
  Comments     : Note that all functions are const because we do not want
                   to disallow logging from const function members.
                   However, callers should return stream state to its
		   original state.
  Complexity   :
  Enhancements :

  History      :   Chia-Hsin Li                                      12/27/94
                      Added FLOG.
                   James Dougherty                                   10/10/94
                   Ronny Kohavi                                      12/21/93
                      Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <LogOptions.h>

RCSID("MLC++, $RCSfile: LogOptions.c,v $ $Revision: 1.15 $")

const int defaultLineNumWidth = 4;


/***************************************************************************
  Description : Constructors
  Comments    :
***************************************************************************/

LogOptions::LogOptions()
{
   log_level = globalLogLevel;
   stream    = globalLogStream;
}

LogOptions::LogOptions(const MString& logOptionName)
{
   log_level = get_env_default(logOptionName + "_LOGLEVEL", "0").short_value();
   stream    = globalLogStream;
}


/***************************************************************************
  Description : Set LogOptions based on another instance as prototype.
  Comments    : This is useful because many classes do not have constructors
                  taking LogOptions.
***************************************************************************/

void LogOptions::set_log_options(const LogOptions& opt) const
{
   set_log_level(opt.get_log_level());
   set_log_stream(opt.get_log_stream());
}


/***************************************************************************
  Description : Get/set for log level and streams.
                Negative log levels are turned into 0.  This allows
		  calls such as set_log_level(get_log_level() - 3);
  Comments    : All functions are const because these options do not
                  effect the behavior of the class they are used in,
                  but cause side effects.
                  This forces us to cast away constness in some cases.
***************************************************************************/

void LogOptions::set_log_level(int level) const
{
   ((LogOptions *)this)->log_level = max(0, level);
}

int  LogOptions::get_log_level() const
{
   return log_level;
}

void LogOptions::set_log_stream(MLCOStream& strm) const
{
   ((LogOptions *)this)->stream = &strm;
}

MLCOStream& LogOptions::get_log_stream() const
{
   return *((LogOptions *)this)->stream;
}

/***************************************************************************
  Description : This method will show the origin of log statements if an
                  environment variable "SHOWORIGIN" is set to yes.
  Comments    : Sets the wrap characters to be the text so that we can grep
                  for specific stuff.
***************************************************************************/
void LogOptions::set_log_prefixes(char *file, int line) const
{
   MString envSpec = get_env_default("SHOW_LOG_ORIGIN", "no");
   if (envSpec.equal_ignore_case("yes") ||
       envSpec.equal_ignore_case("true")) 
      stream->set_newline_prefix(MString(file) + "::" 
				 + MString(line,defaultLineNumWidth)+ ": ");
   else
      stream->set_newline_prefix("");
   stream->set_wrap_prefix(stream->get_newline_prefix() + WRAP_INDENT);
}

void LogOptions::set_log_prefixes(char *file, int line,
				  int stmtLogLevel, int logLevel) const
{
   MString envSpec = get_env_default("SHOW_LOG_ORIGIN", "no");
   if (envSpec.equal_ignore_case("yes")) 
      stream->set_newline_prefix(MString('[') + MString(stmtLogLevel,0) + "<="
				 + MString(logLevel,0) + "] " + 
				 MString(file) + "::" 
				 + MString(line,defaultLineNumWidth)+ ": ");
   else
      stream->set_newline_prefix("");
   stream->set_wrap_prefix(stream->get_newline_prefix() + WRAP_INDENT);
}



/***************************************************************************
  Description : Sets the prefixes for the globalLogStream.
  Comments    : See LogOptions.h
***************************************************************************/
void global_set_log_prefixes(MLCOStream& stream, char *file, int line) 
{
   MString envSpec = get_env_default("SHOW_LOG_ORIGIN", "no");
   if (envSpec.equal_ignore_case("yes")) 
      stream.set_newline_prefix(MString(file) + "::" \
				+ MString(line, defaultLineNumWidth)+ ": ");
   else
      stream.set_newline_prefix("");
   stream.set_wrap_prefix(stream.get_newline_prefix() + WRAP_INDENT);
}



   
