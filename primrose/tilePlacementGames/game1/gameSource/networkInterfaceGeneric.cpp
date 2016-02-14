// Default for platforms that have always-on networking


char isNetworkAlwaysOn() {
    // for testing
    //return false;
    return true;
    }


#include "minorGems/system/Thread.h"

// does nothing
char makeSureNetworkIsUp( char *inTestURL ) {
    // for testing
    //Thread::staticSleep( 5000 );
    
    return true;
    }

