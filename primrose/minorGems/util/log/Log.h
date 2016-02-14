/*
 * Modification History
 *
 * 2002-February-25    Jason Rohrer
 * Created.  
 *
 * 2002-March-29    Jason Rohrer
 * Added Fortify inclusion.
 */

#include "minorGems/common.h"



#ifndef LOG_INCLUDED
#define LOG_INCLUDED



#ifdef FORTIFY
#include "minorGems/util/development/fortify/fortify.h"
#endif



/**
 * An interface for a class that can perform logging functions.
 *
 * @author Jason Rohrer
 */
class Log {



    public:
        
        // These levels were copied from the JLog framework
        // by Todd Lauinger
        
        static const int DEACTIVATE_LEVEL;

        static const int CRITICAL_ERROR_LEVEL;

        static const int ERROR_LEVEL;

        static const int WARNING_LEVEL;

        static const int INFO_LEVEL;

        static const int DETAIL_LEVEL;

        static const int TRACE_LEVEL;


        
        // provided so that virtual deletes work properly
        virtual ~Log();


        
        /**
         * Sets the logging level of the current log.
         *
         * Messages with levels above the current level will not be logged.
         *
         * @param inLevel one of the defined logging levels.
         */
        virtual void setLoggingLevel( int inLevel ) = 0;


        
        /**
         * Gets the logging level of the current log.
         *
         * Messages with levels above the current level will not be logged.
         *
         * @return one of the defined logging levels.
         */
        virtual int getLoggingLevel() = 0;



        /**
         * Logs a string in this log under the default logger name.
         *
         * @param inString the string to log as a \0-terminated string.
         *   Must be destroyed by caller.
         * @param inLevel the level to log inString at.
         */
        virtual void logString( char *inString, int inLevel ) = 0; 
        
        

        /**
         * Logs a string in this log, specifying a logger name.
         *
         * @param inLoggerName the name of the logger
         *   as a \0-terminated string.
         *   Must be destroyed by caller.
         * @param inString the string to log as a \0-terminated string.
         *   Must be destroyed by caller.
         * @param inLevel the level to log inString at.
         */
        virtual void logString( char *inLoggerName, char *inString,
                                int inLevel ) = 0;


        
    };



#endif
