/*
 * Modification History
 *
 * 2002-February-25    Jason Rohrer
 * Created.  
 *
 * 2002-March-30    Jason Rohrer
 * Wrapped our dynamically allocated static member in a statically
 * allocated class to avoid memory leaks at program termination.  
 */



#include "AppLog.h"
#include "Log.h"

#include <stdlib.h>



// wrap our static member in a statically allocated class
LogPointerWrapper AppLog::mLogPointerWrapper( new PrintLog );



LogPointerWrapper::LogPointerWrapper( Log *inLog )
    : mLog( inLog ) {

    }



LogPointerWrapper::~LogPointerWrapper() {
    if( mLog != NULL ) {
        delete mLog;
        }
    }



void AppLog::criticalError( char *inString ) {
    mLogPointerWrapper.mLog->logString(
        inString, Log::CRITICAL_ERROR_LEVEL );
    }



void AppLog::criticalError( char *inLoggerName, char *inString ) {
    mLogPointerWrapper.mLog->logString(
        inLoggerName, inString, Log::CRITICAL_ERROR_LEVEL );
    }



void AppLog::error( char *inString ) {
    mLogPointerWrapper.mLog->logString( inString, Log::ERROR_LEVEL );
    }



void AppLog::error( char *inLoggerName, char *inString ) {
    mLogPointerWrapper.mLog->logString(
        inLoggerName, inString, Log::ERROR_LEVEL );
    }



void AppLog::warning( char *inString ) {
    mLogPointerWrapper.mLog->logString( inString, Log::WARNING_LEVEL );
    }



void AppLog::warning( char *inLoggerName, char *inString ) {
    mLogPointerWrapper.mLog->logString(
        inLoggerName, inString, Log::WARNING_LEVEL );
    }



void AppLog::info( char *inString ) {
    mLogPointerWrapper.mLog->logString( inString, Log::INFO_LEVEL );
    }



void AppLog::info( char *inLoggerName, char *inString ) {
    mLogPointerWrapper.mLog->logString(
        inLoggerName, inString, Log::INFO_LEVEL );
    }



void AppLog::detail( char *inString ) {
    mLogPointerWrapper.mLog->logString( inString, Log::DETAIL_LEVEL );
    }



void AppLog::detail( char *inLoggerName, char *inString ) {
    mLogPointerWrapper.mLog->logString(
        inLoggerName, inString, Log::DETAIL_LEVEL );
    }



void AppLog::trace( char *inString ) {
    mLogPointerWrapper.mLog->logString( inString, Log::TRACE_LEVEL );
    }



void AppLog::trace( char *inLoggerName, char *inString ) {
    mLogPointerWrapper.mLog->logString(
        inLoggerName, inString, Log::TRACE_LEVEL );
    }



void AppLog::setLog( Log *inLog ) {
    int currentLoggingLevel = getLoggingLevel();
    
    if( inLog != mLogPointerWrapper.mLog ) {
        delete mLogPointerWrapper.mLog;
        }
    mLogPointerWrapper.mLog = inLog;
    
    setLoggingLevel( currentLoggingLevel );
    }



Log *AppLog::getLog() {
    return mLogPointerWrapper.mLog;
    }



void AppLog::setLoggingLevel( int inLevel ) {
    mLogPointerWrapper.mLog->setLoggingLevel( inLevel );
    }



int AppLog::getLoggingLevel() {
    return mLogPointerWrapper.mLog->getLoggingLevel();
    }






