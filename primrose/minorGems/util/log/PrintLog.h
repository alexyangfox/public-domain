/*
 * Modification History
 *
 * 2002-February-25    Jason Rohrer
 * Created.  
 *
 * 2002-April-8    Jason Rohrer
 * Changed to be thread-safe.  
 */

#include "minorGems/common.h"



#ifndef PRINT_LOG_INCLUDED
#define PRINT_LOG_INCLUDED



#include "Log.h"

#include "minorGems/system/MutexLock.h"



/**
 * A console-based implementation of the Log interface.
 *
 * @author Jason Rohrer
 */
class PrintLog : public Log {



    public:


        
        /**
         * Constructs a print log.
         */
        PrintLog();


        
        virtual ~PrintLog();

        

        // implement the Log interface
        virtual void setLoggingLevel( int inLevel );

        virtual int getLoggingLevel();

        virtual void logString( char *inString, int inLevel ); 
        
        virtual void logString( char *inLoggerName, char *inString,
                                int inLevel );

        

    protected:

        int mLoggingLevel;

        static const char *mDefaultLoggerName;


        MutexLock *mLock;

        
        
        /**
         * Generates a string representation of a log message.
         *
         * @param inLoggerName the name of the logger
         *   as a \0-terminated string.
         *   Must be destroyed by caller.
         * @param inString the string to log as a \0-terminated string.
         *   Must be destroyed by caller.
         * @param inLevel the level to log inString at.
         *
         * @return the log message as a \0-terminated string.
         *   Must be destroyed by caller.
         */
        char *generateLogMessage( char *inLoggerName, char *inString,
                                  int inLevel );


        
    };



#endif
