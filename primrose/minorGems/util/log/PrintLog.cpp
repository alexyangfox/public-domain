/*
 * Modification History
 *
 * 2002-February-25    Jason Rohrer
 * Created.  
 *
 * 2002-March-11    Jason Rohrer
 * Added a missing include.  
 *
 * 2002-April-8    Jason Rohrer
 * Fixed a casting, dereferencing Win32 compile bug.  
 * Changed to be thread-safe.
 * Changed to use thread-safe printing function. 
 *
 * 2002-April-8    Jason Rohrer
 * Fixed a signed-unsigned mismatch.
 *
 * 2004-January-11    Jason Rohrer
 * Added lock around asctime call.
 * Increased scope of lock.
 *
 * 2004-January-29    Jason Rohrer
 * Changed to use ctime instead of localtime and asctime.
 * Improved locking scope.
 * Changed to use autoSprintf.
 */


#include "PrintLog.h"

#include "minorGems/system/Time.h"

#include "minorGems/util/printUtils.h"
#include "minorGems/util/stringUtils.h"


#include <time.h>
#include <stdio.h>
#include <string.h>



const char *PrintLog::mDefaultLoggerName = "general";



PrintLog::PrintLog()
    : mLoggingLevel( Log::TRACE_LEVEL ),
      mLock( new MutexLock() ) {
    
    }



PrintLog::~PrintLog() {
    delete mLock;
    }



void PrintLog::setLoggingLevel( int inLevel ) {
    mLock->lock();
    mLoggingLevel = inLevel;
    mLock->unlock();
    }



int PrintLog::getLoggingLevel() {
    mLock->lock();
    int level = mLoggingLevel;
    mLock->unlock();
    
    return level;
    }



void PrintLog::logString( char *inString, int inLevel ) {
    logString( (char *)mDefaultLoggerName, inString, inLevel ); 
    }


        
void PrintLog::logString( char *inLoggerName, char *inString,
                         int inLevel ) {


    // not thread-safe to read mLoggingLevel here
    // without synchronization.
    // However, we want logging calls that are above
    // our level to execute with nearly no overhead.
    // mutex might be too much overhead....
    // Besides, not being thread-safe in this case might
    // (worst case) result in a missed log entry or
    // an extra log entry... but setting the logging level should be rare.
    if( inLevel <= mLoggingLevel ) {

        
        char *message = generateLogMessage( inLoggerName,
                                            inString,
                                            inLevel );

        threadPrintF( "%s\n", message );
        
        delete [] message;

        }
    }



char *PrintLog::generateLogMessage( char *inLoggerName, char *inString,
                                    int inLevel ) {

    unsigned long seconds, milliseconds;
    
    Time::getCurrentTime( &seconds, &milliseconds );

    
    // lock around ctime call, since it returns a static buffer
    mLock->lock();

    char *dateString = stringDuplicate( ctime( (time_t *)( &seconds ) ) );
    
    // done with static buffer, since we made a copy
    mLock->unlock();

    
    // this date string ends with a newline...
	// get rid of it
	dateString[ strlen(dateString) - 1 ] = '\0';
    
    char *messageBuffer = autoSprintf( "L%d | %s (%ld ms) | %s | %s",
                                       inLevel, dateString, milliseconds,
                                       inLoggerName, inString );
    
    delete [] dateString;
    
    
    return messageBuffer;
    }


