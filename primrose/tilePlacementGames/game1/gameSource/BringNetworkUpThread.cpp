#include "BringNetworkUpThread.h"

#include "networkInterface.h"


#include "minorGems/util/stringUtils.h"



BringNetworkUpThread::BringNetworkUpThread( char *inURL )
        : mURL( stringDuplicate( inURL ) ),
          mNetworkCheckDone( false ),
          mResult( false ) {

    start();
    }



BringNetworkUpThread::~BringNetworkUpThread() {
    join();
    
    delete [] mURL;
    }



char BringNetworkUpThread::isNetworkCheckDone() {
    mLock.lock();
    char done = mNetworkCheckDone;
    mLock.unlock();
    
    return done;
    }



char BringNetworkUpThread::isNetworkUp() {
    mLock.lock();
    char result = mResult;
    mLock.unlock();
    
    return result;
    }



void BringNetworkUpThread::run() {
    
    char result = makeSureNetworkIsUp( mURL );
    

    mLock.lock();        
    mNetworkCheckDone = true;
    mResult = result;
    mLock.unlock();

    setFinished();
    }
